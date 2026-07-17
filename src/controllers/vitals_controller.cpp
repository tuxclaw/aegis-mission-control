#include "controllers/vitals_controller.h"
#include <chrono>
#include "services/vitals_service.h"
namespace aegis {
VitalsController::VitalsController(VitalsService*s,QObject*p):QObject(p),service_(s){connect(s,&VitalsService::sampled,this,[this](const dto::VitalsDto&v){sample_=v;emit vitalsChanged();});connect(s,&VitalsService::failed,this,[this](const AegisError&e){emit errorRaised(e.userMessage,e.retryable);});}
double VitalsController::cpuPct()const{return sample_.cpu.utilizationPct;} double VitalsController::gpuPct()const{return sample_.gpu.utilizationPct;}
double VitalsController::memPct()const{return sample_.mem.totalBytes?100.0*static_cast<double>(sample_.mem.usedBytes)/static_cast<double>(sample_.mem.totalBytes):0.0;}
double VitalsController::diskUsagePct()const{return sample_.disks.isEmpty()||!sample_.disks.first().totalBytes?0.0:100.0*static_cast<double>(sample_.disks.first().usedBytes)/static_cast<double>(sample_.disks.first().totalBytes);}
quint64 VitalsController::netRxBytesPerSec()const{quint64 v=0;for(const auto&n:sample_.nets)v+=n.rxBytesPerSec;return v;} quint64 VitalsController::netTxBytesPerSec()const{quint64 v=0;for(const auto&n:sample_.nets)v+=n.txBytesPerSec;return v;}
QString VitalsController::gpuVendor()const{return sample_.gpu.vendor;} QString VitalsController::memUsed()const{return QString::number(static_cast<double>(sample_.mem.usedBytes)/(1024.0*1024.0*1024.0),'f',1)+QStringLiteral(" GiB");} QString VitalsController::memTotal()const{return QString::number(static_cast<double>(sample_.mem.totalBytes)/(1024.0*1024.0*1024.0),'f',1)+QStringLiteral(" GiB");}
QVariantList VitalsController::diskModel()const{QVariantList v;for(const auto&d:sample_.disks)v.append(QVariantMap{{QStringLiteral("mount"),d.mount},{QStringLiteral("totalBytes"),QVariant::fromValue(d.totalBytes)},{QStringLiteral("usedBytes"),QVariant::fromValue(d.usedBytes)}});return v;} QVariantList VitalsController::netModel()const{QVariantList v;for(const auto&n:sample_.nets)v.append(QVariantMap{{QStringLiteral("iface"),n.iface},{QStringLiteral("rxBytesPerSec"),QVariant::fromValue(n.rxBytesPerSec)},{QStringLiteral("txBytesPerSec"),QVariant::fromValue(n.txBytesPerSec)}});return v;}
void VitalsController::start(){service_->start(std::chrono::milliseconds(0));} void VitalsController::stop(){service_->stop();}
}
