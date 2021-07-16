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

#include <QMainWindow>
#include <QDateTime>

#include "configuredialog.h"

#if defined(Q_OS_LINUX)
    #include <gpib/ib.h>
#else
    #include <ni4882.h>
#endif


#define TEST_NO_INTERFACE



namespace Ui {
    class MainWindow;
}


QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(Keithley236)
QT_FORWARD_DECLARE_CLASS(Plot2D)


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int iBoard, QWidget *parent = nullptr);
    ~MainWindow() Q_DECL_OVERRIDE;
    bool checkInstruments();
    void updateUserInterface();

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void restoreSettings();
    void saveSettings();
    void writeFileHeader();
    void startI_VScan();
    void startRds_VgScan();
    void initPlot();
    void stopMeasure();
    bool prepareOutputFile(QString sBaseDir, QString sFileName, int currentStep);
    bool prepareLogFile();
    void logMessage(QString sMessage);
    bool DecodeReadings(QString sDataRead, double *current, double *voltage);
    int  criticalError(QString sWhere, QString sText, QString sInfText);

private slots:
    void onLogMessage(QString sMessage);
    void on_startIDSButton_clicked();
    void onComplianceEvent();
    void onClearComplianceEvent();
    void onNewVgGeneratorReading(QDateTime dataTime, QString sDataRead);
    void onNewIdsEvaluatorReading(QDateTime dataTime, QString sDataRead);
    void onIdsSweepDone(QDateTime dataTime, QString sData);
    void onVgSweepDone(QDateTime dataTime, QString sData);
    void on_comboIds_currentIndexChanged(int indx);
    void on_startRdsButton_clicked();

public:
    enum measure {
        NoMeasure      = 0,
        IdsVds_vs_Vg   = 1,
        Rds_vs_Vg      = 2
    };
    measure presentMeasure;

private:
    Ui::MainWindow *ui;

    QFile           *pOutputFile;
    QFile           *pLogFile;
    Keithley236     *pIdsEvaluator;
    Keithley236     *pVgGenerator;
    Plot2D          *pPlotIdsVds;
    Plot2D          *pPlotRdsVg;
    ConfigureDialog *pConfigureDialog;

    QString          sNormalStyle;
    QString          sErrorStyle;

    int              gpibBoardID;
    Addr4882_t       idsAddress;
    Addr4882_t       vgAddress;
    QString          sMeasurementPlotLabel;
    int              maxPlotPoints;
    bool             bMeasureInProgress;
    double           currentVg;
    double           currentVds;
    double           Vg;
    double           Ig;
    double           Vds;
    double           Ids;
    int              currentStep;

    QString          sLogFileName;
    QString          sLogDir;
    QColor           Colors[9];
};


