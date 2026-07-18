#pragma once

#include <QObject>

#include "models/process_list_model.h"

namespace aegis {

class ProcessService;

class ProcessController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(ProcessListModel* processes READ processes CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

 public:
  explicit ProcessController(ProcessService* service,
                             QObject* parent = nullptr);
  [[nodiscard]] ProcessListModel* processes();
  [[nodiscard]] bool loading() const;
  Q_INVOKABLE void refresh();

 signals:
  void loadingChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);

 private:
  void setLoading(bool loading);

  ProcessService* service_;
  ProcessListModel processes_;
  bool loading_ = false;
};

}  // namespace aegis
