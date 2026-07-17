#pragma once

#include <QObject>

#include "models/package_list_model.h"

namespace aegis {
class PackageService;

class PackageController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(PackageListModel* packages READ packages CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(int count READ count NOTIFY countChanged)

 public:
  // Creates a read-only package-inventory controller.
  explicit PackageController(PackageService* service, QObject* parent = nullptr);
  [[nodiscard]] PackageListModel* packages();
  [[nodiscard]] bool loading() const;
  [[nodiscard]] int count() const;
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void filterBy(QString query);

 signals:
  void loadingChanged();
  void countChanged();
  void errorRaised(QString message, bool retryable);

 private:
  PackageService* service_;
  PackageListModel packages_;
  QVector<dto::PackageDto> all_;
  bool loading_ = false;
};
}  // namespace aegis
