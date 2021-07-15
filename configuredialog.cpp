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
#include "configuredialog.h"

#include "idstab.h"
#include "vgtab.h"
#include "filetab.h"
#include "mainwindow.h"

#include <QTabWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>


ConfigureDialog::ConfigureDialog(MainWindow *parent)
    : QDialog(parent)
    , pIdsTab(nullptr)
    , pVgTab(nullptr)
    , pTabFile(nullptr)
    , pParent(parent)
{
    pTabWidget   = new QTabWidget();
    pTabFile     = new FileTab(this);
    iFileIndex   = pTabWidget->addTab(pTabFile,  tr("Out File"));
    pIdsTab      = new IDSTab(this);
    iIdsIndex    = pTabWidget->addTab(pIdsTab,  tr("I_DS"));
    pVgTab       = new VGTab(this);
    iIdsIndex    = pTabWidget->addTab(pVgTab,  tr("V_G"));

    pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                      QDialogButtonBox::Cancel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(pTabWidget);
    mainLayout->addWidget(pButtonBox);
    setLayout(mainLayout);

    setWindowTitle("G-FET Curves");
    connectSignals();
    setToolTips();
}


void
ConfigureDialog::setToolTips() {
    if(pTabFile) pTabWidget->setTabToolTip(iFileIndex,   QString("Output File configuration"));
    if(pIdsTab)  pTabWidget->setTabToolTip(iIdsIndex, QString("Ids Measure Unit configuration"));
    if(pVgTab)   pTabWidget->setTabToolTip(iVgIndex, QString("Vg Generator configuration"));
}


void
ConfigureDialog::connectSignals() {
    connect(pButtonBox, SIGNAL(accepted()),
            this, SLOT(onOk()));
    connect(pButtonBox, SIGNAL(rejected()),
            this, SLOT(onCancel()));
}


void
ConfigureDialog::onCancel() {
    if(pIdsTab)  pIdsTab->restoreSettings();
    if(pVgTab)   pVgTab->restoreSettings();
    if(pTabFile) pTabFile->restoreSettings();
    reject();
}


void
ConfigureDialog::onOk() {
    if(pTabFile->checkFileName()) {
        if(pIdsTab)  pIdsTab->saveSettings();
        if(pVgTab)   pVgTab->saveSettings();
        if(pTabFile) pTabFile->saveSettings();
        accept();
    }
    pTabWidget->setCurrentIndex(iFileIndex);
}
