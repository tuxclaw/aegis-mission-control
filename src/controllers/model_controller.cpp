#include "controllers/model_controller.h"
#include "services/model_service.h"
namespace aegis {
ModelController::ModelController(ModelService*s,QObject*p):QObject(p),service_(s),models_(this){connect(s,&ModelService::activeModelChanged,this,[this](const QString&id){activeModel_=id;emit activeModelChanged();});}
ModelListModel*ModelController::models(){return &models_;} QString ModelController::activeModel()const{return activeModel_;} bool ModelController::loading()const{return loading_;}
void ModelController::refresh(){if(loading_)return;loading_=true;emit loadingChanged();service_->list().then(this,[this](const Result<QVector<dto::ModelDto>>&r){loading_=false;emit loadingChanged();if(!r){emit errorRaised(r.error().userMessage,r.error().retryable);return;}models_.setItems(r.value());for(const auto&m:r.value())if(m.isActive){activeModel_=m.id;emit activeModelChanged();break;}});}
void ModelController::setActiveModel(QString id){service_->setActive(std::move(id)).then(this,[this](const Result<dto::ModelDto>&r){if(!r)emit errorRaised(r.error().userMessage,r.error().retryable);else{emit toast(QStringLiteral("Active model updated"),1);refresh();}});}
}
