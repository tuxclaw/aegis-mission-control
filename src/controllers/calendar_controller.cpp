#include "controllers/calendar_controller.h"
#include <algorithm>
#include <utility>
#include "services/calendar_store.h"
namespace aegis {
CalendarController::CalendarController(CalendarStore*s,QObject*p):QObject(p),store_(s),events_(this){connect(s,&CalendarStore::changed,this,&CalendarController::refresh);}
CalendarEventModel*CalendarController::events(){return &events_;} bool CalendarController::loading()const{return loading_;} void CalendarController::finishError(const AegisError&e){emit errorRaised(e.userMessage,e.retryable);}
void CalendarController::refresh(){if(loading_)return;loading_=true;emit loadingChanged();store_->load().then(this,[this](const Result<QVector<dto::CalendarEventDto>>&r){loading_=false;emit loadingChanged();if(!r)finishError(r.error());else events_.setItems(r.value());});}
void CalendarController::createEvent(QString t,QDateTime s,QDateTime e,bool a,QString l,QString c,QString d){store_->create({{},t,d,s,e,a,l,c,{},{}}).then(this,[this](const Result<dto::CalendarEventDto>&r){if(!r)finishError(r.error());else emit toast(QStringLiteral("Event created"),1);});}
void CalendarController::updateEvent(QString id,QString t,QDateTime s,QDateTime e,bool a,QString l,QString c,QString d){const auto items=events_.items();const auto it=std::find_if(items.cbegin(),items.cend(),[&](const auto&x){return x.id==id;});if(it==items.cend()){finishError(makeError(ErrorCode::PathNotFound));return;}auto x=*it;x.title=t;x.description=d;x.start=s;x.end=e;x.allDay=a;x.location=l;x.color=c;store_->update(x).then(this,[this](const Result<dto::CalendarEventDto>&r){if(!r)finishError(r.error());else emit toast(QStringLiteral("Event updated"),1);});}
void CalendarController::deleteEvent(QString id){pendingDeleteId_=std::move(id);emit confirmRequested(QStringLiteral("deleteEvent"),QStringLiteral("Delete this calendar event?"));}
void CalendarController::confirmDeleteEvent(){if(pendingDeleteId_.isEmpty())return;const auto id=std::exchange(pendingDeleteId_,{});store_->remove(id).then(this,[this](const Result<void>&r){if(!r)finishError(r.error());else emit toast(QStringLiteral("Event deleted"),1);});}
}
