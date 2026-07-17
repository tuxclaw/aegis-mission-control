#include "services/model_service.h"

#include "core/async.h"
#include "services/openclaw_cli.h"

namespace aegis {

ModelService::ModelService(OpenClawCli* cli, QObject* parent)
    : QObject(parent), cli_(cli) {}

QFuture<Result<QVector<dto::ModelDto>>> ModelService::list() {
  return cli_->listModels();
}

QFuture<Result<dto::ModelDto>> ModelService::setActive(QString modelId) {
  if (modelId.trimmed().isEmpty() || modelId.size() > 256 ||
      modelId.contains(QChar::Null)) {
    return async::run([] {
      return Result<dto::ModelDto>(tl::unexpected(makeError(
          ErrorCode::ValidationFailed, QStringLiteral("invalid model id"))));
    });
  }
  return async::flatten(this, cli_->listModels()
      .then(this, [this, modelId](const Result<QVector<dto::ModelDto>>& models)
                       -> QFuture<Result<dto::ModelDto>> {
        if (!models) {
          return async::run([error = models.error()] {
            return Result<dto::ModelDto>(tl::unexpected(error));
          });
        }
        const auto found = std::find_if(models->begin(), models->end(),
                                        [&modelId](const auto& model) {
                                          return model.id == modelId;
                                        });
        if (found == models->end()) {
          return async::run([] {
            return Result<dto::ModelDto>(tl::unexpected(makeError(
                ErrorCode::ValidationFailed,
                QStringLiteral("model is not in live inventory"))));
          });
        }
        auto selected = *found;
        return cli_->run({QStringLiteral("models"), QStringLiteral("set"),
                          modelId, QStringLiteral("--json")})
            .then(this, [this, selected](
                            const Result<OpenClawCli::CliResult>& result) mutable {
              if (!result) {
                return Result<dto::ModelDto>(tl::unexpected(result.error()));
              }
              selected.isActive = true;
              emit activeModelChanged(selected.id);
              return Result<dto::ModelDto>(selected);
            });
      }));
}

}  // namespace aegis
