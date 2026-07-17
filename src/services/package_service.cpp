#include "services/package_service.h"

#include <QProcess>

#include "config/config_service.h"
#include "core/async.h"

namespace aegis {

PackageService::PackageService(ConfigService* config, QObject* parent)
    : QObject(parent), config_(config) {}

QFuture<Result<QVector<dto::PackageDto>>> PackageService::list() {
  const auto command = config_->packageQueryCommand();
  return async::run([command]() -> Result<QVector<dto::PackageDto>> {
    if (!command) return tl::unexpected(command.error());
    QProcess process;
    process.start(command->first(), command->mid(1));
    if (!process.waitForStarted(5000)) {
      return tl::unexpected(makeError(ErrorCode::ProcessSpawnFailed,
                                      QStringLiteral("package query failed to start")));
    }
    if (!process.waitForFinished(30000)) {
      process.kill();
      process.waitForFinished(1000);
      return tl::unexpected(makeError(ErrorCode::ProcessTimeout,
                                      QStringLiteral("package query timed out")));
    }
    const auto output = process.readAllStandardOutput();
    if (output.size() > 8 * 1024 * 1024) {
      return tl::unexpected(makeError(ErrorCode::ResponseTooLarge,
                                      QStringLiteral("package output cap exceeded")));
    }
    if (process.exitCode() != 0) {
      return tl::unexpected(makeError(ErrorCode::ProcessNonZeroExit,
                                      QStringLiteral("package query failed")));
    }
    const auto source = command->first().contains(QStringLiteral("dpkg"))
                            ? QStringLiteral("dpkg")
                            : QStringLiteral("rpm");
    QVector<dto::PackageDto> packages;
    const auto lines = output.split('\n');
    constexpr int kMaxLines = 100000;
    if (lines.size() > kMaxLines) {
      return tl::unexpected(makeError(ErrorCode::ResponseTooLarge,
                                      QStringLiteral("package output line count exceeded")));
    }
    packages.reserve(lines.size());
    for (const auto& rawLine : lines) {
      const auto line = QString::fromUtf8(rawLine).trimmed();
      if (line.isEmpty()) continue;
      QString name;
      QString version;
      QString summary;
      const auto fields = line.split(QChar::Tabulation);
      if (fields.size() >= 2) {
        name = fields.at(0).trimmed();
        version = fields.at(1).trimmed();
        if (fields.size() >= 3) summary = fields.at(2).trimmed().left(200);
      } else {
        const auto dash = line.lastIndexOf(QLatin1Char('-'));
        if (dash <= 0 || dash == line.size() - 1) {
          return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                          QStringLiteral("package line invalid")));
        }
        name = line.left(dash);
        version = line.mid(dash + 1);
      }
      const auto package = dto::PackageDto::fromJson(
          QJsonObject{{QStringLiteral("name"), name},
                      {QStringLiteral("version"), version},
                      {QStringLiteral("source"), source},
                      {QStringLiteral("summary"), summary}});
      if (!package) {
        return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                        package.error().debugContext));
      }
      packages.append(package.value());
    }
    return packages;
  });
}

}  // namespace aegis
