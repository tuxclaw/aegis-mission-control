#include "controllers/app_controller.h"
#include "services/gateway_service.h"
namespace aegis {
AppController::AppController(GatewayService* g,QObject*p):QObject(p),gateway_(g){connect(g,&GatewayService::connectionStateChanged,this,[this](ConnectionState){emit connectionStateChanged();});}
int AppController::connectionState()const{return static_cast<int>(gateway_->connectionState());}
int AppController::activeView()const{return activeView_;}
QDateTime AppController::lastSyncTime()const{return lastSyncTime_;}
void AppController::setActiveView(int i){if(i<0||i==activeView_)return;activeView_=i;emit activeViewChanged();}
void AppController::refreshAll(){emit refreshAllRequested();}
void AppController::markSynced(){lastSyncTime_=QDateTime::currentDateTimeUtc();emit lastSyncTimeChanged();}
}
