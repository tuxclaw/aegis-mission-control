#pragma once

#include <QObject>

#include "models/model_list_model.h"

namespace aegis {
class ModelService;

class ModelController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(ModelListModel* models READ models CONSTANT)
  Q_PROPERTY(QString activeModel READ activeModel NOTIFY activeModelChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

 public:
  // Creates a live model-selection controller.
  explicit ModelController(ModelService* service, QObject* parent = nullptr);
  [[nodiscard]] ModelListModel* models();
  [[nodiscard]] QString activeModel() const;
  [[nodiscard]] bool loading() const;
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void setActiveModel(QString modelId);

 signals:
  void activeModelChanged();
  void loadingChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);

 private:
  ModelService* service_;
  ModelListModel models_;
  QString activeModel_;
  bool loading_ = false;
};
}  // namespace aegis
