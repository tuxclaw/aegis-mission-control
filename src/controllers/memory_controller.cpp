#include "controllers/memory_controller.h"
#include "services/memory_service.h"
namespace aegis {
MemoryController::MemoryController(MemoryService*s,QObject*p):QObject(p),service_(s),files_(this){const auto ids=rootIds();if(!ids.isEmpty())currentRoot_=ids.first();}
MemoryFileModel*MemoryController::files(){return &files_;} QString MemoryController::currentContent()const{return currentContent_;} QString MemoryController::currentRoot()const{return currentRoot_;} QStringList MemoryController::rootIds()const{return service_->rootIds();}
void MemoryController::setRoot(QString id){if(!rootIds().contains(id)){const auto e=makeError(ErrorCode::PathOutsideSandbox);emit errorRaised(e.userMessage,e.retryable);return;}currentRoot_=std::move(id);currentContent_.clear();emit currentRootChanged();emit currentContentChanged();refresh();}
void MemoryController::selectFile(QString p){service_->read(currentRoot_,std::move(p)).then(this,[this](const Result<QString>&r){if(!r)emit errorRaised(r.error().userMessage,r.error().retryable);else{currentContent_=r.value();emit currentContentChanged();}});}
void MemoryController::refresh(){if(currentRoot_.isEmpty())return;service_->list(currentRoot_).then(this,[this](const Result<QVector<dto::MemoryFileDto>>&r){if(!r)emit errorRaised(r.error().userMessage,r.error().retryable);else files_.setItems(r.value());});}
}
