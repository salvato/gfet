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
#include "filetab.h"
#include "mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QSettings>
#include <QMessageBox>
#include <QGridLayout>
#include <QtDebug>


FileTab::FileTab(QWidget *parent)
    : QWidget(parent)
    , sBaseDir(QDir::homePath())
    , sOutFileName("data.dat")
{
    outFilePathButton.setText(QString("..."));

    // Build the Tab layout
    QGridLayout* pLayout = new QGridLayout();
    pLayout->addWidget(new QLabel("File Path"),          0, 0, 1, 1);
    pLayout->addWidget(&outPathEdit,                     0, 1, 1, 5);
    pLayout->addWidget(&outFilePathButton,               0, 6, 1, 1);
    pLayout->addWidget(new QLabel("File Name"),          1, 0, 1, 1);
    pLayout->addWidget(&outFileEdit,                     1, 1, 1, 6);
    pLayout->addWidget(new QLabel("Sample Information"), 2, 0, 1, 7);
    pLayout->addWidget(&sampleInformationEdit,           3, 0, 4, 7);
    setLayout(pLayout);

    connectSignals();
    restoreSettings();
    setToolTips();
    initUI();
}


void
FileTab::initUI() {
    sampleInformationEdit.setPlainText(sSampleInfo);
    outPathEdit.setText(sBaseDir);
    outFileEdit.setText(sOutFileName);
}


void
FileTab::setToolTips() {
    sampleInformationEdit.setToolTip(QString("Enter Sample description (multiline)"));
    outPathEdit.setToolTip(QString("Output File Folder"));
    outFileEdit.setToolTip(QString("Enter Output File Name"));
    outFilePathButton.setToolTip((QString("Press to Change Output File Folder")));
}


void
FileTab::connectSignals() {
    connect(&outFilePathButton, SIGNAL(clicked()),
            this, SLOT(on_outFilePathButton_clicked()));
}


void
FileTab::restoreSettings() {
    QSettings settings;
    sSampleInfo    = settings.value("FileTabSampleInfo", "").toString();
    sBaseDir       = settings.value("FileTabBaseDir", sBaseDir).toString();
    sOutFileName   = settings.value("FileTabOutFileName", sOutFileName).toString();
}


void
FileTab::saveSettings() {
    QSettings settings;
    sSampleInfo = sampleInformationEdit.toPlainText();
    settings.setValue("FileTabSampleInfo", sSampleInfo);
    settings.setValue("FileTabBaseDir", sBaseDir);
    settings.setValue("FileTabOutFileName", sOutFileName);
}


void
FileTab::on_outFilePathButton_clicked() {
    QFileDialog chooseDirDialog;
    QDir outDir(sBaseDir);
    chooseDirDialog.setFileMode(QFileDialog::DirectoryOnly);
    if(outDir.exists())
        chooseDirDialog.setDirectory(outDir);
    else
        chooseDirDialog.setDirectory(QDir::homePath());
    if(chooseDirDialog.exec() == QDialog::Accepted)
        sBaseDir = chooseDirDialog.selectedFiles().at(0);
    outPathEdit.setText(sBaseDir);
}


bool
FileTab::checkFileName() {
    sOutFileName = outFileEdit.text();
    if(sOutFileName == QString()) {
        QMessageBox::information(
                    this,
                    QString("Empty Output Filename"),
                    QString("Please enter a Valid Output File Name"));
        outFileEdit.setFocus();
        return false;
    }
    if(QDir(sBaseDir).exists(sOutFileName)) {
        int iAnswer = QMessageBox::question(
                    this,
                    QString("File Exists"),
                    QString("Do you want overwrite\n%1 ?").arg(sOutFileName),
                    QMessageBox::Yes,
                    QMessageBox::No,
                    QMessageBox::NoButton);
        if(iAnswer == QMessageBox::No) {
            outFileEdit.setFocus();
            return false;
        }
    }
    return true;
}
