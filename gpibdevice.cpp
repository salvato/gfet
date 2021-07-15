#include "gpibdevice.h"
//#include <QDebug>

GpibDevice::GpibDevice(int gpio, int address, QObject *parent)
    : QObject(parent)
    , gpibNumber(gpio)
    , gpibAddress(address)
    , gpibId(-1)
{
}


bool
GpibDevice::isGpibError(QString sErrorString) {
    if(ThreadIbsta() & ERR) {
        QString sError = ErrMsg(ThreadIbsta(), ThreadIberr(), ThreadIbcnt());
        emit sendMessage(sErrorString + QString("\n") + sError);
        return true;
    }
    return false;
}


QString
GpibDevice::ErrMsg(int sta, int err, long cntl) {
    QString sError, sTmp;

    sError = QString("status = 0x%1 <").arg(sta, 4, 16, QChar('0'));
    if (sta & ERR )  sError += QString(" ERR");
    if (sta & TIMO)  sError += QString(" TIMO");
    if (sta & END )  sError += QString(" END");
    if (sta & SRQI)  sError += QString(" SRQI");
    if (sta & RQS )  sError += QString(" RQS");
    if (sta & CMPL)  sError += QString(" CMPL");
    if (sta & LOK )  sError += QString(" LOK");
    if (sta & REM )  sError += QString(" REM");
    if (sta & CIC )  sError += QString(" CIC");
    if (sta & ATN )  sError += QString(" ATN");
    if (sta & TACS)  sError += QString(" TACS");
    if (sta & LACS)  sError += QString(" LACS");
    if (sta & DTAS)  sError += QString(" DTAS");
    if (sta & DCAS)  sError += QString(" DCAS");
    sError += QString(">");

    if(sta & ERR)  {
        sTmp = QString(" error = 0x%1").arg(err, 4, 16, QChar('0'));
        sError += sTmp;
        if (err == EDVR) sError += QString(" EDVR <DOS Error>");
        if (err == ECIC) sError += QString(" ECIC <Not CIC>");
        if (err == ENOL) sError += QString(" ENOL <No Listener>");
        if (err == EADR) sError += QString(" EADR <Address error>");
        if (err == EARG) sError += QString(" EARG <Invalid argument>");
        if (err == ESAC) sError += QString(" ESAC <Not Sys Ctrlr>");
        if (err == EABO) sError += QString(" EABO <Op. aborted>");
        if (err == ENEB) sError += QString(" ENEB <No GPIB board>");
        if (err == EOIP) sError += QString(" EOIP <Async I/O in prg>");
        if (err == ECAP) sError += QString(" ECAP <No capability>");
        if (err == EFSO) sError += QString(" EFSO <File sys. error>");
        if (err == EBUS) sError += QString(" EBUS <Command error>");
        if (err == ESTB) sError += QString(" ESTB <Status byte lost>");
        if (err == ESRQ) sError += QString(" ESRQ <SRQ stuck on>");
        if (err == ETAB) sError += QString(" ETAB <Table Overflow>");
    }
    sTmp = QString(" count = 0x%1").arg(cntl, 4, 16, QChar('0'));
    sError += sTmp;
    return sError;
}


uint
GpibDevice::gpibWrite(int ud, QString sCmd) {
    //qDebug() << QString("Writing %1 bytes of data: Data = %2").arg(sCmd.length()).arg(sCmd.toUtf8().constData());
    ibwrt(ud, sCmd.toUtf8().constData(), sCmd.length());
    isGpibError("GPIB Writing Error Writing");
    return uint(ThreadIbsta());
}


QString
GpibDevice::gpibRead(int ud) {
    QString sString;
    do {
        ibrd(ud, readBuf, sizeof(readBuf)-1);
        if(isGpibError("GPIB Reading Error"))
            return QString();
        readBuf[ThreadIbcnt()] = 0;
        sString += QString(readBuf);
    } while(ThreadIbcnt() == sizeof(readBuf)-1);
    return sString;
}


int
GpibDevice::init() {
    return NO_ERROR;
}


void
GpibDevice::onGpibCallback(int LocalUd, unsigned long LocalIbsta, unsigned long LocalIberr, long LocalIbcntl) {
    Q_UNUSED(LocalUd)
    Q_UNUSED(LocalIbsta)
    Q_UNUSED(LocalIberr)
    Q_UNUSED(LocalIbcntl)
}


void
GpibDevice::checkNotify() {
}
