#include "controllers/cron_controller.h"
#include "services/cron_service.h"
namespace aegis {
CronController::CronController(CronService*s,QObject*p):QObject(p),service_(s),jobs_(this){} CronJobModel*CronController::jobs(){return &jobs_;} bool CronController::loading()const{return loading_;}
void CronController::refresh(){if(loading_)return;loading_=true;emit loadingChanged();service_->list().then(this,[this](const Result<QVector<dto::CronJobDto>>&r){loading_=false;emit loadingChanged();if(!r)emit errorRaised(r.error().userMessage,r.error().retryable);else jobs_.setItems(r.value());});}
void CronController::toggleJob(QString id,bool e){service_->toggle(std::move(id),e).then(this,[this](const Result<dto::CronJobDto>&r){if(!r)emit errorRaised(r.error().userMessage,r.error().retryable);else refresh();});}
void CronController::runJob(QString id){service_->runNow(std::move(id)).then(this,[this](const Result<QString>&r){if(!r)emit errorRaised(r.error().userMessage,r.error().retryable);else emit toast(QStringLiteral("Cron run started"),1);});}
}
