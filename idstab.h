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

#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QRadioButton>
#include <QLabel>


class IDSTab : public QWidget
{
    Q_OBJECT
public:
    explicit IDSTab(QWidget *parent = nullptr);
    void restoreSettings();
    void saveSettings();

signals:

public slots:
    void onStartEdit_textChanged(const QString &arg1);
    void onStopEdit_textChanged(const QString &arg1);
    void onComplianceEdit_textChanged(const QString &arg1);
    void onWaitTimeEdit_textChanged(const QString &arg1);
    void onSweepPointsEdit_textChanged(const QString &arg1);
    void onMeasureIntervalEdit_textChanged(const QString &arg1);

protected:
    void setToolTips();
    void setCaptions();
    void initUI();
    void connectSignals();
    bool isCurrentValid(double dCurrent);
    bool isVoltageValid(double dVoltage);
    bool isComplianceValid(double dCompliance);
    bool isWaitTimeValid(int iWaitTime);
    bool isSweepPointNumberValid(int nSweepPoints);
    bool isIntervalValid(double interval);

public:
    double dStart;
    double dStop;
    double dStep;
    double dCompliance;
    int    iWaitTime;
    int    iNSweepPoints;
    double dInterval;

private:
    // Limit Values
    const double currentMin;
    const double currentMax;
    const double voltageMin;
    const double voltageMax;
    const int    waitTimeMin;
    const int    waitTimeMax;
    const int    nSweepPointsMin;
    const int    nSweepPointsMax;
    const double intervalMin;
    const double intervalMax;

    // QLineEdit styles
    QString sNormalStyle;
    QString sErrorStyle;

    // UI Elements
    QRadioButton SourceIButton;
    QRadioButton SourceVButton;
    QLabel       StartLabel;
    QLabel       StopLabel;
    QLabel       ComplianceLabel;
    QLineEdit    StartEdit;
    QLineEdit    StopEdit;
    QLineEdit    ComplianceEdit;
    QLineEdit    WaitTimeEdit;
    QLineEdit    SweepPointsEdit;
    QLineEdit    MeasureIntervalEdit;
};

