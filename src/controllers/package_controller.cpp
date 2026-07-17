#include "controllers/package_controller.h"
#include "services/package_service.h"
namespace aegis {
PackageController::PackageController(PackageService*s,QObject*p):QObject(p),service_(s),packages_(this){} PackageListModel*PackageController::packages(){return &packages_;} bool PackageController::loading()const{return loading_;} int PackageController::count()const{return packages_.rowCount();}
void PackageController::refresh(){if(loading_)return;loading_=true;emit loadingChanged();service_->list().then(this,[this](const Result<QVector<dto::PackageDto>>&r){loading_=false;emit loadingChanged();if(!r){emit errorRaised(r.error().userMessage,r.error().retryable);return;}all_=r.value();packages_.setItems(all_);emit countChanged();});}
void PackageController::filterBy(QString q){q=q.trimmed();if(q.isEmpty())packages_.setItems(all_);else{QVector<dto::PackageDto>v;for(const auto&p:all_)if(p.name.contains(q,Qt::CaseInsensitive)||p.summary.contains(q,Qt::CaseInsensitive))v.append(p);packages_.setItems(std::move(v));}emit countChanged();}
}
