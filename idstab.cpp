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
#include "idstab.h"
#include "mainwindow.h"

#include <QLineEdit>
#include <QLabel>
#include <QRadioButton>
#include <QGridLayout>
#include <QSettings>
#include <QtDebug>


IDSTab::IDSTab(QWidget *parent)
    : QWidget(parent)
    , currentMin(-1.0e-2)
    , currentMax(1.0e-2)
    , voltageMin(-110.0)
    , voltageMax(110.0)
    , waitTimeMin(100)
    , waitTimeMax(65000)
    , nSweepPointsMin(3)
    , nSweepPointsMax(500)
    , intervalMin(0.1)
    , intervalMax(600.0)
{
    // Build the Tab layout
    QGridLayout* pLayout = new QGridLayout();
    // Labels
    pLayout->addWidget(&StopLabel,                   2, 0, 1, 1);
    pLayout->addWidget(new QLabel("Rdgs Intv [ms]"), 4, 0, 1, 1);
    pLayout->addWidget(new QLabel("N°of Points"),    5, 0, 1, 1);
    pLayout->addWidget(&StartLabel,      1, 0, 1, 1);
    pLayout->addWidget(&ComplianceLabel, 3, 0, 1, 1);
    //Line Edits
    pLayout->addWidget(&StartEdit,      1, 1, 1, 1);
    pLayout->addWidget(&ComplianceEdit, 3, 1, 1, 1);
    pLayout->addWidget(&StopEdit,        2, 1, 1, 1);
    pLayout->addWidget(&WaitTimeEdit,    4, 1, 1, 1);
    pLayout->addWidget(&SweepPointsEdit, 5, 1, 1, 1);
    // Set the Layout
    setLayout(pLayout);

    sNormalStyle = StartEdit.styleSheet();

    sErrorStyle  = "QLineEdit { ";
    sErrorStyle += "color: rgb(255, 255, 255);";
    sErrorStyle += "background: rgb(255, 0, 0);";
    sErrorStyle += "selection-background-color: rgb(128, 128, 255);";
    sErrorStyle += "}";

    connectSignals();
    restoreSettings();
    initUI();
}


void
IDSTab::restoreSettings() {
    QSettings settings;
    dStart        = settings.value("IDSTabStart", 0.0).toDouble();
    dStop         = settings.value("IDSTabStop", 0.0).toDouble();
    dCompliance   = settings.value("IDSTabCompliance", 0.0).toDouble();
    iWaitTime     = settings.value("IDSTabWaitTime", 100).toInt();
    iNSweepPoints = settings.value("IDSTabSweepPoints", 100).toInt();
    dInterval     = settings.value("IDSTabMeasureInterval", 0.1).toDouble();
    dStep = (dStop-dStart) / iNSweepPoints;
}


void
IDSTab::saveSettings() {
    QSettings settings;
    settings.setValue("IDSTabStart",       dStart);
    settings.setValue("IDSTabStop",        dStop);
    settings.setValue("IDSTabCompliance",  dCompliance);
    settings.setValue("IDSTabWaitTime",    iWaitTime);
    settings.setValue("IDSTabSweepPoints", iNSweepPoints);
    settings.setValue("IDSTabMeasureInterval", dInterval);
}


void
IDSTab::setToolTips() {
    QString sHeader = QString("Enter values in range [%1 : %2]");
    StartEdit.setToolTip(sHeader.arg(voltageMin).arg(voltageMax));
    StopEdit.setToolTip(sHeader.arg(voltageMin).arg(voltageMax));
    ComplianceEdit.setToolTip(sHeader.arg(currentMin).arg(currentMax));
    SourceIButton.setToolTip("Source Current - Measure Voltage");
    SourceVButton.setToolTip("Source Voltage - Measure Current");
    WaitTimeEdit.setToolTip(sHeader.arg(waitTimeMin).arg(waitTimeMax));
    SweepPointsEdit.setToolTip((sHeader.arg(nSweepPointsMin).arg(nSweepPointsMax)));
    MeasureIntervalEdit.setToolTip(sHeader.arg(intervalMin).arg(intervalMax));
}


void
IDSTab::initUI() {
    // Measurement parameters
    SourceVButton.setChecked(true);
    if(!isVoltageValid(dStart))
        dStart = 0.0;
    if(!isVoltageValid(dStop))
        dStop = 0.0;
    StartLabel.setText("V Start [V]");
    StopLabel.setText("V Stop [V]");
    ComplianceLabel.setText("Compliance [A]");

    StartEdit.setText(QString("%1").arg(dStart, 0, 'g', 2));
    StopEdit.setText(QString("%1").arg(dStop, 0, 'g', 2));
    if(!isComplianceValid(dCompliance))
        dCompliance = 0.0;
    ComplianceEdit.setText(QString("%1").arg(dCompliance, 0, 'g', 2));
    if(!isWaitTimeValid(iWaitTime))
        iWaitTime = 100;
    WaitTimeEdit.setText(QString("%1").arg(iWaitTime));
    if(!isSweepPointNumberValid(iNSweepPoints))
        iNSweepPoints = 100;
    SweepPointsEdit.setText(QString("%1").arg(iNSweepPoints));
    if(!isIntervalValid(dInterval)) {
        dInterval = intervalMin;
    }
    MeasureIntervalEdit.setText(QString("%1").arg(dInterval, 0, 'f', 2));
    setToolTips();
}


void
IDSTab::connectSignals() {
    connect(&StartEdit, SIGNAL(textChanged(const QString)),
            this, SLOT(onStartEdit_textChanged(const QString)));
    connect(&StopEdit, SIGNAL(textChanged(const QString)),
            this, SLOT(onStopEdit_textChanged(const QString)));
    connect(&ComplianceEdit, SIGNAL(textChanged(const QString)),
            this, SLOT(onComplianceEdit_textChanged(const QString)));
    connect(&WaitTimeEdit, SIGNAL(textChanged(const QString)),
            this, SLOT(onWaitTimeEdit_textChanged(const QString)));
    connect(&SweepPointsEdit, SIGNAL(textChanged(const QString)),
            this, SLOT(onSweepPointsEdit_textChanged(const QString)));
    connect(&MeasureIntervalEdit, SIGNAL(textChanged(const QString)),
            this, SLOT(onMeasureIntervalEdit_textChanged(const QString)));
}


bool
IDSTab::isCurrentValid(double dCurrent) {
    return (dCurrent >= currentMin) &&
            (dCurrent <= currentMax);
}


bool
IDSTab::isVoltageValid(double dVoltage) {
    return (dVoltage >= voltageMin) &&
            (dVoltage <= voltageMax);
}


bool
IDSTab::isComplianceValid(double dCompliance){
    return isCurrentValid(dCompliance);
}


bool
IDSTab::isWaitTimeValid(int iWaitTime) {
    return (iWaitTime >= waitTimeMin) &&
            (iWaitTime <= waitTimeMax);
}


bool
IDSTab::isSweepPointNumberValid(int nSweepPoints) {
    return (nSweepPoints >= nSweepPointsMin) &&
            (nSweepPoints <= nSweepPointsMax);
}


bool
IDSTab::isIntervalValid(double interval) {
    return (interval >= intervalMin) && (interval <= intervalMax);
}


void
IDSTab::onStartEdit_textChanged(const QString &arg1) {
    double dTemp = arg1.toDouble();
    bool bValid = isVoltageValid(dTemp);
    if(bValid) {
        dStart = dTemp;
        StartEdit.setStyleSheet(sNormalStyle);
        dStep = (dStop-dStart) / iNSweepPoints;
    }
    else {
        StartEdit.setStyleSheet(sErrorStyle);
    }
}


void
IDSTab::onStopEdit_textChanged(const QString &arg1) {
    double dTemp = arg1.toDouble();
    bool bValid = isVoltageValid(dTemp);
    if(bValid) {
        dStop = dTemp;
        StopEdit.setStyleSheet(sNormalStyle);
        dStep = (dStop-dStart) / iNSweepPoints;
    }
    else {
        StopEdit.setStyleSheet(sErrorStyle);
    }
}


void
IDSTab::onComplianceEdit_textChanged(const QString &arg1) {
    double dTemp = arg1.toDouble();
    bool bValid = isCurrentValid(dTemp);
    if(bValid) {
        dCompliance = dTemp;
        ComplianceEdit.setStyleSheet(sNormalStyle);
    }
    else {
        ComplianceEdit.setStyleSheet(sErrorStyle);
    }
}


void
IDSTab::onWaitTimeEdit_textChanged(const QString &arg1) {
    int iTemp = arg1.toInt();
    if(isWaitTimeValid(iTemp)) {
        iWaitTime = iTemp;
        WaitTimeEdit.setStyleSheet(sNormalStyle);
    }
    else {
        WaitTimeEdit.setStyleSheet(sErrorStyle);
    }
}


void
IDSTab::onSweepPointsEdit_textChanged(const QString &arg1) {
    int iTemp = arg1.toInt();
    if(isSweepPointNumberValid(iTemp)) {
        iNSweepPoints = iTemp;
        SweepPointsEdit.setStyleSheet(sNormalStyle);
        dStep = (dStop-dStart) / iNSweepPoints;
    }
    else {
        SweepPointsEdit.setStyleSheet(sErrorStyle);
    }
}


void
IDSTab::onMeasureIntervalEdit_textChanged(const QString &arg1) {
    if(isIntervalValid(arg1.toDouble())){
        dInterval = arg1.toDouble();
        MeasureIntervalEdit.setStyleSheet(sNormalStyle);
    }
    else {
        MeasureIntervalEdit.setStyleSheet(sErrorStyle);
    }
}


