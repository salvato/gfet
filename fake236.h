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
#pragma once

#include <QtGlobal>

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include "gpibdevice.h"


class Fake236 : public GpibDevice
{
    Q_OBJECT

public:
    explicit Fake236(int gpio, int address, QObject *parent = Q_NULLPTR);
    virtual ~Fake236();

public:
    int      init();
    int      initVvsTSourceI(double dAppliedCurrent, double dCompliance);
    int      initSourceV(double dAppliedVoltage, double dCompliance);
    int      endMeasure();
    void     onGpibCallback(int ud, unsigned long ibsta, unsigned long iberr, long ibcntl);
    bool     initISweep(double startCurrent, double stopCurrent, double currentStep, double delay, double voltageCompliance);
    bool     initVSweep(double startVoltage, double stopVoltage, double voltageStep, double delay, double currentCompliance);
    int      stopSweep();
    bool     sendTrigger();
    bool     isReadyForTrigger();

signals:
    void     complianceEvent();
    void     clearCompliance();
    void     readyForTrigger();
    void     newReading(QDateTime currentTime, QString sReading);
    void     sweepDone(QDateTime currentTime, QString sSweepData);

public slots:
    void checkNotify();

protected:

public:
    const int SRQ_DISABLED;
    const int WARNING;
    const int SWEEP_DONE;
    const int TRIGGER_OUT;
    const int READING_DONE;
    const int READY_FOR_TRIGGER;
    const int K236_ERROR;
    const int COMPLIANCE;


private:
    bool   bStop;
    int    iComplianceEvents;
    double lastReading;
    bool   isSweeping;
};
