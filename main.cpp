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
#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>
#include <QFileInfo>
#include <QThread>


void
myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
    fflush(stderr);
}


int
main(int argc, char *argv[]) {

    // Install the message handler
    qInstallMessageHandler(myMessageOutput);

    QApplication a(argc, argv);
    QMessageBox msgBox;

    int gpibBoardID = 0;

    QCoreApplication::setOrganizationDomain("Gabriele.Salvato");
    QCoreApplication::setOrganizationName("Gabriele.Salvato");
    QCoreApplication::setApplicationName("gfet");
    QCoreApplication::setApplicationVersion("0.0.1");


#ifndef TEST_NO_INTERFACE
#ifdef Q_OS_LINUX
    QString sGpibInterface = QString("/dev/gpib%1").arg(gpibBoardID);
    QFileInfo checkFile(sGpibInterface);
    while(!checkFile.exists()) {
        msgBox.setWindowTitle(QCoreApplication::applicationName());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QString("No %1 device file").arg(sGpibInterface));
        msgBox.setInformativeText(QString("Is the GPIB Interface connected ? "));
        msgBox.setStandardButtons(QMessageBox::Abort|QMessageBox::Retry);
        msgBox.setDefaultButton(QMessageBox::Retry);
        if(msgBox.exec() == QMessageBox::Abort)
            return 0;
    }
#endif
#endif

    MainWindow w(gpibBoardID);
    w.setWindowIcon(QIcon("qrc:/myLogoT.png"));
    w.thread()->setPriority(QThread::TimeCriticalPriority);
    w.show();
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

#ifndef TEST_NO_INTERFACE
    while(!w.checkInstruments()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(QCoreApplication::applicationName());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QString("GPIB Instruments Not Found"));
        msgBox.setInformativeText(QString("Switch on and retry"));
        msgBox.setStandardButtons(QMessageBox::Abort|QMessageBox::Retry);
        msgBox.setDefaultButton(QMessageBox::Retry);
        if(msgBox.exec()==QMessageBox::Abort)
            return 0;
    }
#endif
    w.updateUserInterface();

    QApplication::restoreOverrideCursor();
    return a.exec();
}
