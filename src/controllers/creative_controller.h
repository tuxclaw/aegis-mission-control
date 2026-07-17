#pragma once

#include <QHash>
#include <QObject>

namespace aegis {
class CreativeService;

class CreativeController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString output READ output NOTIFY outputChanged)
  Q_PROPERTY(QString finishReason READ finishReason NOTIFY resultChanged)
  Q_PROPERTY(quint64 outputBytes READ outputBytes NOTIFY resultChanged)

 public:
  // Creates a single-request streaming presentation adapter.
  explicit CreativeController(CreativeService* service,
                              QObject* parent = nullptr);
  [[nodiscard]] bool busy() const;
  [[nodiscard]] QString output() const;
  [[nodiscard]] QString finishReason() const;
  [[nodiscard]] quint64 outputBytes() const;
  Q_INVOKABLE QString generate(int backend, QString model, QString prompt,
                               double temperature = 0.7,
                               int maxTokens = 1024);
  Q_INVOKABLE void cancel();

 signals:
  void busyChanged();
  void outputChanged();
  void resultChanged();
  void chunk(QString requestId, QString delta);
  void finished(QString requestId, QString resultText, QString finishReason);
  void failed(QString requestId, QString message);

 private:
  CreativeService* service_;
  bool busy_ = false;
  QString output_;
  QString finishReason_;
  quint64 outputBytes_ = 0;
  QString requestId_;
  QHash<QString, QString> outputBuffers_;
};
}  // namespace aegis
