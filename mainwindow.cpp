﻿/*
 *
Copyright (C) 2021  Gabriele Salvato

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

/*
 * Il programma usa due Keithley 236 (K236) per misurare le curve
 * caratteristiche di un GFET (Graphene Field Effect Transistor).
 * Il primo K236 genera la tensione di Gate (Vg) e misura la corente
 * di perdita (Ig) che dovrebbe esser nulla per lo strato di SiO2.
 * Il secondo K236 misura la curva Ids vs Vds alla Vg impostata.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "idstab.h"
#include "vgtab.h"
#include "filetab.h"
#include "keithley236.h"
#include "plot2d.h"

#include <qmath.h>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QThread>
#include <QLayout>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

//#define MY_DEBUG
#define MAXTIMINGS 83



MainWindow::MainWindow(int iBoard, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pOutputFile(nullptr)
    , pLogFile(nullptr)
    , pIdsEvaluator(nullptr)
    , pVgGenerator(nullptr)
    , pPlotIdsVds(nullptr)
    , pPlotRdsVg(nullptr)
    , pConfigureDialog(nullptr)
{
    // Init internal variables
    maxPlotPoints        = 3000;
    gpibBoardID          = iBoard;
    bMeasureInProgress   = false;

    // Prepare message logging
    sLogFileName = QString("gFETLog.txt");
    sLogDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if(!sLogDir.endsWith(QString("/"))) sLogDir+= QString("/");
    sLogFileName = sLogDir+sLogFileName;
    #ifndef MY_DEBUG
        prepareLogFile();
    #endif

    // Setup User Interface
    ui->setupUi(this);
    ui->statusBar->setSizeGripEnabled(false); // Remove the resize-handle in the lower right corner
    setFixedSize(size()); // Make the size of the window fixed
    setWindowIcon(QIcon("qrc:/myLogoT.png"));

    // Setup the QLineEdit styles
    sNormalStyle = ui->labelCompliance->styleSheet();
    sErrorStyle  = "QLabel { color: rgb(255, 255, 255); background: rgb(255, 0, 0); selection-background-color: rgb(128, 128, 255); }";

    // Restore Geometry and State of the window
    QSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());
    restoreSettings();

    thread()->setPriority(QThread::TimeCriticalPriority);

//    #ifdef TEST_NO_INTERFACE
//        for(int i=0; i<2; i++) {
//            ui->comboIds->addItem(QString("%1").arg(i+10));
//        }
//    #endif

    // Some plot colors...
    Colors[0] = QColor(  0,   0, 255);
    Colors[1] = QColor(  0, 255,   0);
    Colors[2] = QColor(  0, 255, 255);
    Colors[3] = QColor(255,   0,   0);
    Colors[4] = QColor(255,   0, 255);
    Colors[5] = QColor(255, 255,   0);
    Colors[6] = QColor(255, 255, 255);

    presentMeasure = NoMeasure;
}


MainWindow::~MainWindow() {
    if(pIdsEvaluator     != nullptr) delete pIdsEvaluator;
    if(pVgGenerator      != nullptr) delete pVgGenerator;
    if(pPlotIdsVds != nullptr) delete pPlotIdsVds;
    if(pConfigureDialog  != nullptr) delete pConfigureDialog;
    if(pOutputFile       != nullptr) delete pOutputFile;
    if(pLogFile          != nullptr) delete pLogFile;
    delete ui;
}


///////////////////////
/// ToDo: Improve !!!
///////////////////////
void
MainWindow::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
    QSettings settings;
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());
    saveSettings();
    if(bMeasureInProgress) {
        if(pOutputFile) {
            if(pOutputFile->isOpen())
                pOutputFile->close();
            pOutputFile->deleteLater();
            pOutputFile = nullptr;
        }
        if(pIdsEvaluator) {
            pIdsEvaluator->endMeasure();
        }
        if(pVgGenerator) {
            pVgGenerator->endMeasure();
        }
    }
    if(pIdsEvaluator) delete pIdsEvaluator;
    if(pVgGenerator) delete pVgGenerator;
    pIdsEvaluator = nullptr;
    pVgGenerator = nullptr;

    if(pLogFile) {
        if(pLogFile->isOpen()) {
            pLogFile->flush();
        }
        pLogFile->deleteLater();
        pLogFile = nullptr;
    }
}


void
MainWindow::restoreSettings() {
    QSettings settings;
    idsAddress = Addr4882_t(settings.value("IdsAddress", 0).toInt());
    vgAddress  = Addr4882_t(settings.value("VgAddress",  0).toInt());
}


void
MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue("IdsAddress", idsAddress);
    settings.setValue("VgAddress",  vgAddress);
}


bool
MainWindow::prepareLogFile() {
    // Rotate 5 previous logs, removing the oldest, to avoid data loss
    QFileInfo checkFile(sLogFileName);
    if(checkFile.exists() && checkFile.isFile()) {
        QDir renamed;
        renamed.remove(sLogFileName+QString("_4.txt"));
        for(int i=4; i>0; i--) {
            renamed.rename(sLogFileName+QString("_%1.txt").arg(i-1),
                           sLogFileName+QString("_%1.txt").arg(i));
        }
        renamed.rename(sLogFileName,
                       sLogFileName+QString("_0.txt"));
    }
    // Open the new log file
    pLogFile = new QFile(sLogFileName);
    if (!pLogFile->open(QIODevice::WriteOnly)) {
        QMessageBox::information(nullptr, "Conductivity",
                                 QString("Unable to open file %1: %2.")
                                 .arg(sLogFileName, pLogFile->errorString()));
        delete pLogFile;
        pLogFile = nullptr;
    }
    return true;
}


void
MainWindow::logMessage(QString sMessage) {
    QDateTime dateTime;
    QString sDebugMessage = dateTime.currentDateTime().toString() +
                            QString(" - ") +
                            sMessage;
    if(pLogFile) {
        if(pLogFile->isOpen()) {
            pLogFile->write(sDebugMessage.toUtf8().data());
            pLogFile->write("\n");
            pLogFile->flush();
        }
        else
            qDebug() << sDebugMessage;
    }
    else
        qDebug() << sDebugMessage;
}


int
MainWindow::criticalError(QString sWhere, QString sText, QString sInfText) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(sWhere);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(sText);
    msgBox.setInformativeText(sInfText);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}


bool
MainWindow::checkInstruments() {
    if(pIdsEvaluator != nullptr) delete pIdsEvaluator;
    if(pVgGenerator  != nullptr) delete pVgGenerator;
    pIdsEvaluator = nullptr;
    pVgGenerator  = nullptr;
    idsAddress = 0;
    vgAddress  = 0;
    ui->comboIds->clear();
    ui->statusBar->showMessage("Checking for the GPIB Instruments");
    Addr4882_t padlist[31];
    Addr4882_t resultlist[31];
    for(uint16_t i=0; i<30; i++) padlist[i] = i+1;
    padlist[30] = NOADDR;
    // Resets the GPIB bus by asserting the 'interface clear' bus line
    SendIFC(gpibBoardID);
    if(ThreadIbsta() & ERR) {
        criticalError(QString(Q_FUNC_INFO),
                      QString("SendIFC() Error"),
                      QString("Is the GPIB Interface connected ?"));
        return false;
    }
    // Enable assertion of REN when System Controller
    // Required by the Keithley 236
    ibconfig(gpibBoardID, IbcSRE, 1);
    if(ThreadIbsta() & ERR) {
        criticalError(QString(Q_FUNC_INFO),
                      QString("ibconfig() Error"),
                      QString("Unable to set REN When SC"));
        return false;
    }
    // If addrlist contains only the constant NOADDR,
    // the Universal Device Clear (DCL) message is sent
    // to all the devices on the bus
    Addr4882_t addrlist;
    addrlist = NOADDR;
    DevClearList(gpibBoardID, &addrlist);
    if(ThreadIbsta() & ERR) {
        criticalError(QString(Q_FUNC_INFO),
                      QString("DevClearList() failed"),
                      QString("Are the Instruments Connected and Switched On ?"));
        return false;
    }
    // Find all the instruments connected to the GPIB Bus
    FindLstn(gpibBoardID, padlist, resultlist, 30);
    if(ThreadIbsta() & ERR) {
        criticalError(QString(Q_FUNC_INFO),
                      QString("FindLstn() failed"),
                      QString("Are the Instruments Connected and Switched On ?"));
        return false;
    }
    int nDevices = ThreadIbcnt();
#if defined(MY_DEBUG)
    logMessage(QString("Found %1 Instruments connected to the GPIB Bus").arg(nDevices));
#endif
    // Identify the instruments connected to the GPIB Bus
    QString sCommand, sInstrumentID;
    char readBuf[257];
    // Check for the Keithley 236
    sCommand = "U0X";
    for(int i=0; i<nDevices; i++) {
        DevClear(gpibBoardID, resultlist[i]);
        Send(gpibBoardID, resultlist[i], sCommand.toUtf8().constData(), sCommand.length(), DABend);
        Receive(gpibBoardID, resultlist[i], readBuf, 256, STOPend);
        readBuf[ThreadIbcnt()] = '\0';
        sInstrumentID = QString(readBuf);
#if defined(MY_DEBUG)
        logMessage(QString("Address= %1 - InstrumentID= %2")
                   .arg(resultlist[i])
                   .arg(sInstrumentID));
#endif
        if(sInstrumentID.contains("236", Qt::CaseInsensitive)) {
            ui->comboIds->addItem(QString("%1").arg(resultlist[i]));
        }
    } // for(int i=0; i<nDevices; i++)

    if(ui->comboIds->count() == 0) {
        criticalError(QString(Q_FUNC_INFO),
                      QString("Unable to Proceed"),
                      QString("No K236 Found"));
        return false;
    }


    // If only a single K236 is Found
    // use it as Ids Measuring System
    if(ui->comboIds->count() == 1) {
        idsAddress = Addr4882_t(ui->comboIds->itemText(0).toInt());
        pIdsEvaluator = new Keithley236(gpibBoardID, idsAddress, this);
        connect(pIdsEvaluator, SIGNAL(sendMessage(QString)),
                this, SLOT(onLogMessage(QString)));
        ui->statusBar->showMessage("Ids Evaluator Found! Ready to Start");
        return true;
    }

    // Two or more K236 are present...
    // Check if we already have choosen the Ids Evaluator
    for(int i=0; i<2; i++) {
        if(ui->comboIds->itemText(i).toInt() == idsAddress) {
            pIdsEvaluator = new Keithley236(gpibBoardID, idsAddress, this);
            connect(pIdsEvaluator, SIGNAL(sendMessage(QString)),
                    this, SLOT(onLogMessage(QString)));
            vgAddress = Addr4882_t(ui->comboIds->itemText(1-i).toInt());
            pVgGenerator = new Keithley236(gpibBoardID, vgAddress, this);
            connect(pVgGenerator, SIGNAL(sendMessage(QString)),
                    this, SLOT(onLogMessage(QString)));
            ui->comboIds->setCurrentIndex(i);
            ui->statusBar->showMessage("GPIB Instruments Found! Ready to Start");
            return true;
        }
    }
    idsAddress = Addr4882_t(ui->comboIds->itemText(0).toInt());
    pIdsEvaluator = new Keithley236(gpibBoardID, idsAddress, this);
    connect(pIdsEvaluator, SIGNAL(sendMessage(QString)),
            this, SLOT(onLogMessage(QString)));
    vgAddress = Addr4882_t(ui->comboIds->itemText(1).toInt());
    pVgGenerator = new Keithley236(gpibBoardID, vgAddress, this);
    connect(pVgGenerator, SIGNAL(sendMessage(QString)),
            this, SLOT(onLogMessage(QString)));
    ui->comboIds->setCurrentIndex(0);
    ui->statusBar->showMessage("GPIB Instruments Found! Ready to Start");
    return true;
}


/////////////////////////////////////////////
////////////////////////////// To be modified
/////////////////////////////////////////////
void
MainWindow::updateUserInterface() {
    if(presentMeasure == NoMeasure) {
        ui->statusGroupBox->setEnabled(true);
        ui->startIDSButton->setEnabled(true);
        ui->startRdsButton->setEnabled(true);
    }

    else if(presentMeasure == IdsVds_vs_Vg) {
        ui->statusGroupBox->setDisabled(true);
        ui->startIDSButton->setEnabled(true);
        ui->startRdsButton->setDisabled(true);
    }

    else if(presentMeasure == Rds_vs_Vg) {
        ui->statusGroupBox->setDisabled(true);
        ui->startIDSButton->setDisabled(true);
        ui->startRdsButton->setEnabled(true);
    }
}

/////////////////////////////////////////////
////////////////////////////// To be modified
/////////////////////////////////////////////
void
MainWindow::writeFileHeader() {
    // To cope with the GnuPlot way to handle the comment lines
    pOutputFile->write(QString("%1 %2 %3 %4\n")
                       .arg("#V_G[V]", 12)
                       .arg("I_G[A]",  12)
                       .arg("V_DS[V]", 12)
                       .arg("I_DS[A]", 12)
                       .toLocal8Bit());
    QStringList HeaderLines = pConfigureDialog->pTabFile->sSampleInfo.split("\n");
    for(int i=0; i<HeaderLines.count(); i++) {
        pOutputFile->write("# ");
        pOutputFile->write(HeaderLines.at(i).toLocal8Bit());
        pOutputFile->write("\n");
    }
    pOutputFile->write(QString("# Vds_Start=%1[V] Vds_Stop=%2[V] Compliance=%3[A]\n")
                       .arg(pConfigureDialog->pIdsTab->dStart)
                       .arg(pConfigureDialog->pIdsTab->dStop)
                       .arg(pConfigureDialog->pIdsTab->dCompliance).toLocal8Bit());
    pOutputFile->write(QString("# Vg_Start=%1[V] Vg_Stop=%2[V] Compliance=%3[A]\n")
                       .arg(pConfigureDialog->pVgTab->dStart)
                       .arg(pConfigureDialog->pVgTab->dStop)
                       .arg(pConfigureDialog->pVgTab->dCompliance).toLocal8Bit());
    pOutputFile->flush();
}


void
MainWindow::startI_VScan() {
    double dStart = pConfigureDialog->pIdsTab->dStart;
    double dStop = pConfigureDialog->pIdsTab->dStop;
    int nSweepPoints = pConfigureDialog->pIdsTab->iNSweepPoints;
    double dStep = qAbs(dStop - dStart) / double(nSweepPoints);
    double dDelayms = double(pConfigureDialog->pIdsTab->iWaitTime);
    double dCompliance = pConfigureDialog->pIdsTab->dCompliance;
    connect(pIdsEvaluator, SIGNAL(sweepDone(QDateTime,QString)),
            this, SLOT(onIdsSweepDone(QDateTime,QString)));
    pIdsEvaluator->initVSweep(dStart, dStop, dStep, dDelayms, dCompliance);
    while(!pIdsEvaluator->isReadyForTrigger()) {}
    pIdsEvaluator->sendTrigger();
    bMeasureInProgress = true;
    ui->statusBar->showMessage("Sweeping...Please Wait");
}


void
MainWindow::stopMeasure() {
    if(pOutputFile) {
        pOutputFile->close();
        pOutputFile->deleteLater();
        pOutputFile = nullptr;
    }
    if(pIdsEvaluator != nullptr) {
        pIdsEvaluator->disconnect();
        pIdsEvaluator->stopSweep();
    }
    if(pVgGenerator != nullptr) {
        pVgGenerator->disconnect();
        pVgGenerator->stopSweep();
    }
    if(presentMeasure == IdsVds_vs_Vg)
        ui->startIDSButton->setText("Ids-Vds (vs Vg)");
    else
        ui->startRdsButton->setText("Rds (vs Vg)");
    presentMeasure = NoMeasure;
    updateUserInterface();
    QApplication::restoreOverrideCursor();
}


bool
MainWindow::prepareOutputFile(QString sBaseDir,
                              QString sFileName,
                              int currentStep)
{
    if(pOutputFile) {
        pOutputFile->close();
        pOutputFile->deleteLater();
        pOutputFile = nullptr;
    }
    QFileInfo fileInfo(sFileName);
    QString sName = fileInfo.baseName();
    QString sExt = fileInfo.completeSuffix();
    QString sFilePath = QString("%1/%2_%3.%4")
                                .arg(sBaseDir,
                                sName)
                                .arg(currentStep)
                                .arg(sExt);
    pOutputFile = new QFile(sFilePath);
    if(!pOutputFile->open(QIODevice::Text|QIODevice::WriteOnly)) {
        QMessageBox::critical(this,
                              "Error: Unable to Open Output File",
                              sFilePath);
        ui->statusBar->showMessage("Unable to Open Output file...");
        return false;
    }
    return true;
}


void
MainWindow::initPlot() {
    if(pPlotIdsVds) delete pPlotIdsVds;
    pPlotIdsVds = nullptr;
    // Plot of Current vs Voltage
    sMeasurementPlotLabel = QString("Ids[A] vs Vds[V]");
    pPlotIdsVds = new Plot2D(this, sMeasurementPlotLabel);
    pPlotIdsVds->setWindowTitle(pConfigureDialog->pTabFile->sOutFileName);
    pPlotIdsVds->setMaxPoints(maxPlotPoints);
    pPlotIdsVds->SetLimits(0.0, 1.0, 0.0, 1.0, true, true, false, false);
    pPlotIdsVds->UpdatePlot();
    pPlotIdsVds->show();
}


void
MainWindow::on_startIDSButton_clicked() {
    if(ui->startIDSButton->text().contains("Stop")) {
        stopMeasure();
        ui->statusBar->showMessage("Measure Stopped");
        return;
    }
    //else
    if(pConfigureDialog) delete pConfigureDialog;
    pConfigureDialog = new ConfigureDialog(this);
    if(pConfigureDialog->exec() == QDialog::Rejected)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    presentMeasure = IdsVds_vs_Vg;
    // Initializing Ids Evaluator
    ui->statusBar->showMessage("Initializing Ids Evaluator...");
    if(pIdsEvaluator->init()) {
        ui->statusBar->showMessage("Unable to Initialize Ids Evaluator...");
        stopMeasure();
        return;
    }
    connect(pIdsEvaluator, SIGNAL(complianceEvent()),
            this, SLOT(onComplianceEvent()));
    connect(pIdsEvaluator, SIGNAL(clearCompliance()),
            this, SLOT(onClearComplianceEvent()));

    // Initializing Vg Generator
    if(pVgGenerator) {
        ui->statusBar->showMessage("Initializing Vg Generator..");
        if(pVgGenerator->init()) {
            ui->statusBar->showMessage("Unable to Initialize Keithley 236...");
            QApplication::restoreOverrideCursor();
            return;
        }
        connect(pVgGenerator, SIGNAL(complianceEvent()),
                this, SLOT(onComplianceEvent()));
        connect(pVgGenerator, SIGNAL(clearCompliance()),
                this, SLOT(onClearComplianceEvent()));
    }

    // Init the Plots
    initPlot();

    ////////////////////////////////
    /// Ready to Start the Measure
    ////////////////////////////////

    // Generate the first value of Vg...
    currentVg = pConfigureDialog->pVgTab->dStart;
    if(pVgGenerator) {
        pVgGenerator->initSourceV(currentVg, pConfigureDialog->pVgTab->dCompliance);
        while(!pVgGenerator->isReadyForTrigger()) {}
        connect(pVgGenerator, SIGNAL(newReading(QDateTime,QString)),
                this, SLOT(onNewVgGeneratorReading(QDateTime,QString)));
        pVgGenerator->sendTrigger();
    }
    else {
        // Configure Vg and press OK
    }
    currentStep = 1;
    // Then Start the Ids vs Vds Scan
    startI_VScan();
    ui->startIDSButton->setText("Stop");
    updateUserInterface();
}


void
MainWindow::onNewVgGeneratorReading(QDateTime dataTime, QString sDataRead) {
    Q_UNUSED(dataTime)
    if(!DecodeReadings(sDataRead, &Ids, &Vds))
        return;
    ui->currentEdit->setText(QString("%1").arg(Ids, 10, 'g', 4, ' '));
    ui->voltageEdit->setText(QString("%1").arg(Vds, 10, 'g', 4, ' '));
    QString sTitle = QString("Vds=%1").arg(Vds);
    pPlotIdsVds->NewDataSet(currentStep,//Id
                                  3, //Pen Width
                                  Colors[currentStep % 7],
                                  Plot2D::iline,
                                  sTitle
                                  );
    pPlotIdsVds->SetShowDataSet(currentStep, true);
    pPlotIdsVds->SetShowTitle(currentStep, true);
    pPlotIdsVds->UpdatePlot();
}


void
MainWindow::onComplianceEvent() {
    ui->labelCompliance->setText("Compliance");
    ui->labelCompliance->setStyleSheet(sErrorStyle);
    logMessage("Compliance Event");
}


void
MainWindow::onClearComplianceEvent() {
    ui->labelCompliance->setText("");
    ui->labelCompliance->setStyleSheet(sNormalStyle);
}


bool
MainWindow::DecodeReadings(QString sDataRead, double *current, double *voltage) { // Decode readings
    QStringList sMeasures = QStringList(sDataRead.split(",", Qt::SkipEmptyParts));
    if(sMeasures.count() < 2) {
        logMessage("Measurement Format Error");
        return false;
    }
    *current = sMeasures.at(1).toDouble();
    *voltage = sMeasures.at(0).toDouble();
    return true;
}


void
MainWindow::onIdsSweepDone(QDateTime dataTime, QString sData) {
    Q_UNUSED(dataTime)
    disconnect(pIdsEvaluator, SIGNAL(sweepDone(QDateTime,QString)), this, nullptr);
    ui->statusBar->showMessage("Sweep Done: Decoding readings...Please wait");
    QStringList sMeasures = QStringList(sData.split(",", Qt::SkipEmptyParts));
    if(sMeasures.count() < 2) {
        stopMeasure();
        ui->statusBar->showMessage(QString(Q_FUNC_INFO) + QString(" Error: No Sweep Values"));
        onClearComplianceEvent();
        return;
    }
    // Open the Output file
    ui->statusBar->showMessage("Opening Output file...");
    if(!prepareOutputFile(pConfigureDialog->pTabFile->sBaseDir,
                          pConfigureDialog->pTabFile->sOutFileName,
                          currentStep))
    {
        stopMeasure();
        return;
    }
    // Write File Header
    writeFileHeader();
    ui->statusBar->showMessage("Sweep Done: Updating Plot...Please wait");
    double Ids, Vds;
    for(int i=0; i<sMeasures.count(); i+=2) {
        Vds = sMeasures.at(i).toDouble();
        Ids = sMeasures.at(i+1).toDouble();
        QString sData = QString("%1 %2 %3 %4\n")
                .arg(Vg, 12, 'g', 6, ' ')
                .arg(Ig, 12, 'g', 6, ' ')
                .arg(Vds,   12, 'g', 6, ' ')
                .arg(Ids,   12, 'g', 6, ' ');
        pOutputFile->write(sData.toLocal8Bit());
        pPlotIdsVds->NewPoint(currentStep, Vds, Ids);
    }
    pPlotIdsVds->UpdatePlot();
    pOutputFile->flush();
    pOutputFile->close();
    currentVg += pConfigureDialog->pVgTab->dStep;
    if(currentVg > pConfigureDialog->pVgTab->dStop) {
        stopMeasure();
        ui->statusBar->showMessage("Measure Done");
        onClearComplianceEvent();
        return;
    }
    if(pVgGenerator) {
        pVgGenerator->initSourceV(currentVg, pConfigureDialog->pVgTab->dCompliance);
        while(!pVgGenerator->isReadyForTrigger()) {}
        pVgGenerator->sendTrigger();
    }
    else {
        // Configure Vg and press OK
    }
    QString sTitle = QString("Vg=%1").arg(currentVg);
    currentStep++;
    pPlotIdsVds->NewDataSet(currentStep,//Id
                                  3, //Pen Width
                                  Colors[currentStep % 7],
                                  Plot2D::iline,
                                  sTitle
                                  );
    pPlotIdsVds->SetShowDataSet(currentStep, true);
    pPlotIdsVds->SetShowTitle(currentStep, true);
    pPlotIdsVds->UpdatePlot();
    // Start the new Ids vs Vds Scan
    startI_VScan();
}


void
MainWindow::onNewIdsEvaluatorReading(QDateTime dataTime,QString sDataRead) {
    Q_UNUSED(dataTime)
    Q_UNUSED(sDataRead)
}


void
MainWindow::onLogMessage(QString sMessage) {
    logMessage(sMessage);
}


void
MainWindow::on_comboIds_currentIndexChanged(int indx) {
    if(pIdsEvaluator != nullptr) delete pIdsEvaluator;
    if(pVgGenerator  != nullptr) delete pVgGenerator;
    pIdsEvaluator = pVgGenerator  = nullptr;
    idsAddress = vgAddress  = 0;

    int iAddr = ui->comboIds->itemText(indx).toInt();

    idsAddress = Addr4882_t(iAddr);
    pIdsEvaluator = new Keithley236(gpibBoardID, idsAddress, this);
    connect(pIdsEvaluator, SIGNAL(sendMessage(QString)),
            this, SLOT(onLogMessage(QString)));
    if(ui->comboIds->count() > 1) {
        vgAddress = Addr4882_t(ui->comboIds->itemText(1-indx).toInt());
        pVgGenerator = new Keithley236(gpibBoardID, vgAddress, this);
        connect(pVgGenerator, SIGNAL(sendMessage(QString)),
                this, SLOT(onLogMessage(QString)));
    }
}


void
MainWindow::on_startRdsButton_clicked() {
    if(ui->startRdsButton->text().contains("Stop")) {
        stopMeasure();
        ui->statusBar->showMessage("Measure Stopped");
        return;
    }
    //else
    if(pConfigureDialog) delete pConfigureDialog;
    pConfigureDialog = new ConfigureDialog(this);
    if(pConfigureDialog->exec() == QDialog::Rejected)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    presentMeasure = Rds_vs_Vg;

    currentVds = pConfigureDialog->pIdsTab->dStart;
    pIdsEvaluator->initSourceV(currentVds, pConfigureDialog->pIdsTab->dCompliance);
    while(!pIdsEvaluator->isReadyForTrigger()) {}
    connect(pIdsEvaluator, SIGNAL(newReading(QDateTime,QString)),
            this, SLOT(onNewIdsEvaluatorReading(QDateTime,QString)));
    pIdsEvaluator->sendTrigger();
    currentStep = 1;
    // Then Start the Rds vs Vg Scan
    startRds_VgScan();
    ui->startRdsButton->setText("Stop");

    updateUserInterface();
}
