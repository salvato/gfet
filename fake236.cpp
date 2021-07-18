/*
 *
Copyright (C) 2016  Gabriele Salvato

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "fake236.h"

#include <QtMath>
#include <QDateTime>
#include <QThread>
//#include <QDebug>

#define MAX_COMPLIANCE_EVENTS 5

namespace fake236 {
static int  rearmMask;
#if !defined(Q_OS_LINUX)
int __stdcall
myCallback(int LocalUd, unsigned long LocalIbsta, unsigned long LocalIberr, long LocalIbcntl, void* callbackData) {
    reinterpret_cast<Keithley236*>(callbackData)->onGpibCallback(LocalUd, LocalIbsta, LocalIberr, LocalIbcntl);
    return Keithley236::rearmMask;
}
#endif
}


Fake236::Fake236(int gpio, int address, QObject *parent)
    : GpibDevice(gpio, address, parent)
    , SRQ_DISABLED(0)
    , WARNING(1)
    , SWEEP_DONE(2)
    , TRIGGER_OUT(4)
    , READING_DONE(8)
    , READY_FOR_TRIGGER(16)
    , K236_ERROR(32)
    , COMPLIANCE(128)
    //
    , isSweeping(false)
{
    iComplianceEvents = 0;
    pollInterval = 569;
}


Fake236::~Fake236() {
    if(gpibId != -1) {
#if defined(Q_OS_LINUX)
        pollTimer.stop();
        pollTimer.disconnect();
#else
        ibnotify(gpibId, 0, NULL, NULL);// disable notification
#endif
        ibonl(gpibId, 0);// Disable hardware and software.
    }
}


int
Fake236::init() {
    return NO_ERROR;
}


int
Fake236::initVvsTSourceI(double dAppliedCurrent, double dCompliance) {
    Q_UNUSED(dAppliedCurrent)
    Q_UNUSED(dCompliance)
    return NO_ERROR;
}


int
Fake236::initSourceV(double dAppliedVoltage, double dCompliance) {
    Q_UNUSED(dAppliedVoltage)
    Q_UNUSED(dCompliance)
    return NO_ERROR;
}


int
Fake236::endMeasure() {
    return NO_ERROR;
}


bool
Fake236::initISweep(double startCurrent,
                        double stopCurrent,
                        double currentStep,
                        double delay,
                        double voltageCompliance) {
    Q_UNUSED(startCurrent)
    Q_UNUSED(stopCurrent)
    Q_UNUSED(currentStep)
    Q_UNUSED(delay)
    Q_UNUSED(voltageCompliance)
    isSweeping = true;
    return true;
}


bool
Fake236::initVSweep(double startVoltage,
                        double stopVoltage,
                        double voltageStep,
                        double delay,
                        double currentCompliance) {
    Q_UNUSED(startVoltage)
    Q_UNUSED(stopVoltage)
    Q_UNUSED(voltageStep)
    Q_UNUSED(delay)
    Q_UNUSED(currentCompliance)
    isSweeping = true;
    return true;
}


int
Fake236::stopSweep() {
    isSweeping = false;
    return NO_ERROR;
}


void
Fake236::onGpibCallback(int LocalUd, unsigned long LocalIbsta, unsigned long LocalIberr, long LocalIbcntl) {
    Q_UNUSED(LocalIbsta)
    Q_UNUSED(LocalIbcntl)

    spollByte = 0;
    int iStatus = ibrsp(LocalUd, &spollByte);
    if(iStatus & ERR) {
        emit sendMessage(QString(Q_FUNC_INFO) + QString("GPIB error %1").arg(LocalIberr));
        emit sendMessage(QString(Q_FUNC_INFO) + QString("ibrsp() returned: %1").arg(iStatus));
        emit sendMessage(QString(Q_FUNC_INFO) + QString("Serial Poll Response Byte %1").arg(spollByte));
    }

    if(spollByte & COMPLIANCE) {// Compliance
        iComplianceEvents++;
        emit complianceEvent();
        QThread::msleep(300);
    }
    else
        emit clearCompliance();

    if(spollByte & K236_ERROR) {// Error
        gpibWrite(LocalUd, "U1X");
        sCommand = gpibRead(LocalUd);
        QString sError = QString(Q_FUNC_INFO) + QString("Error ")+ sCommand;
        emit sendMessage(sError);
    }

    if(spollByte & WARNING) {// Warning
        gpibWrite(LocalUd, "U9X");
        sCommand = gpibRead(LocalUd);
        QString sError = QString(Q_FUNC_INFO) + QString("Warning ")+ sCommand;
        emit sendMessage(sError);
    }

    if(spollByte & SWEEP_DONE) {// Sweep Done
        QDateTime currentTime = QDateTime::currentDateTime();
        QString sString = gpibRead(LocalUd);
        fake236::rearmMask = RQS;
        emit sweepDone(currentTime, sString);
        return;
    }

    if(spollByte & TRIGGER_OUT) {// Trigger Out
        QString sError = QString(Q_FUNC_INFO) + QString("Trigger Out ?");
        emit sendMessage(sError);
    }

    if(spollByte & READY_FOR_TRIGGER) {// Ready for trigger
        emit readyForTrigger();
    }

    if((spollByte & READING_DONE) && !isSweeping){// Reading Done
        sResponse = gpibRead(LocalUd);
        if(sResponse != QString()) {
            QDateTime currentTime = QDateTime::currentDateTime();
            emit newReading(currentTime, sResponse);
        }
    }

    fake236::rearmMask = RQS;
}


bool
Fake236::isReadyForTrigger() {
    ibrsp(gpibId, &spollByte);
    if(isGpibError(QString(Q_FUNC_INFO) + ": Error in ibrsp()"))
        return false;
    return ((spollByte & READY_FOR_TRIGGER) != 0);
}


bool
Fake236::sendTrigger() {
    ibtrg(gpibId);
    if(isGpibError(QString(Q_FUNC_INFO) + "Trigger Error"))
        return false;
    return true;
}


void
Fake236::checkNotify() {
#if defined(Q_OS_LINUX)
    onGpibCallback(gpibId, uint(ThreadIbsta()), uint(ThreadIberr()), ThreadIbcnt());
#endif
}
