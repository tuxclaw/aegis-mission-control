#include "controllers/agent_controller.h"
#include <algorithm>
#include "services/openclaw_cli.h"
namespace aegis {
AgentController::AgentController(OpenClawCli*c,QObject*p):QObject(p),cli_(c),agents_(this){}
AgentListModel*AgentController::agents(){return &agents_;} bool AgentController::loading()const{return loading_;}
int AgentController::activeCount()const{return static_cast<int>(std::count_if(all_.cbegin(),all_.cend(),[](const auto&a){return a.status==dto::AgentStatus::Active;}));}
void AgentController::refresh(){if(loading_)return;loading_=true;emit loadingChanged();cli_->listAgents().then(this,[this](const Result<QVector<dto::AgentDto>>&r){loading_=false;emit loadingChanged();if(!r){emit errorRaised(r.error().userMessage,r.error().retryable);return;}all_=r.value();emit activeCountChanged();filterBy(filter_);emit refreshed();});}
void AgentController::filterBy(QString s){filter_=s.toLower();if(filter_.isEmpty()||filter_==QStringLiteral("all")){agents_.setItems(all_);return;}QVector<dto::AgentDto>v;for(const auto&a:all_){const auto active=filter_==QStringLiteral("active")&&a.status==dto::AgentStatus::Active;const auto idle=filter_==QStringLiteral("idle")&&a.status==dto::AgentStatus::Idle;const auto error=filter_==QStringLiteral("error")&&a.status==dto::AgentStatus::Error;if(active||idle||error)v.append(a);}agents_.setItems(std::move(v));}
}
