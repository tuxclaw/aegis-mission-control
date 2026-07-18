#pragma once

#include <QObject>

#include "models/container_list_model.h"

namespace aegis {

class ContainerService;

class ContainerController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(ContainerListModel* containers READ containers CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(bool available READ available NOTIFY availableChanged)

 public:
  explicit ContainerController(ContainerService* service,
                               QObject* parent = nullptr);
  [[nodiscard]] ContainerListModel* containers();
  [[nodiscard]] bool loading() const;
  [[nodiscard]] bool available() const;
  Q_INVOKABLE void refresh();

 signals:
  void loadingChanged();
  void availableChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);

 private:
  void setLoading(bool loading);

  ContainerService* service_;
  ContainerListModel containers_;
  bool loading_ = false;
  bool available_ = false;
};

}  // namespace aegis
