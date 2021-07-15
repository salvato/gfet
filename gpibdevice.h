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
#include <QTimer>


#if defined(Q_OS_LINUX)
#include <gpib/ib.h>
#else
#include <ni4882.h>
#endif


class GpibDevice : public QObject
{
    Q_OBJECT
public:
    explicit      GpibDevice(int gpio, int address, QObject *parent = Q_NULLPTR);
    virtual int    init();
    virtual void   onGpibCallback(int ud, unsigned long ibsta, unsigned long iberr, long ibcntl);

protected:
    uint    gpibWrite(int ud, QString sCmd);
    QString gpibRead(int ud);
    QString ErrMsg(int sta, int err, long cntl);
    bool    isGpibError(QString sErrorString);

signals:
    void    sendMessage(QString sMessage);

public slots:
    void checkNotify();

public:
    const int GPIB_DEVICE_NOT_PRESENT = -1000;
    const int NO_ERROR = 0;

protected:
    QString sCommand;
    QString sResponse;
    QTimer  pollTimer;
    int     pollInterval;
    int     gpibNumber;
    int     gpibAddress;
    int     gpibId;
    char    spollByte;
    int     iMask;
    char    readBuf[2001];
};
