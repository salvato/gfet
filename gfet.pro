#Copyright (C) 2021  Gabriele Salvato

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.


QT += core
QT += gui
QT += widgets


TARGET = gfet
TEMPLATE = app


SOURCES += main.cpp \
    fake236.cpp \
    idstab.cpp \
    vgtab.cpp
SOURCES += mainwindow.cpp
SOURCES += axesdialog.cpp
SOURCES += AxisFrame.cpp
SOURCES += AxisLimits.cpp
SOURCES += configuredialog.cpp
SOURCES += DataSetProperties.cpp
SOURCES += datastream2d.cpp
SOURCES += filetab.cpp
SOURCES += gpibdevice.cpp
SOURCES +=
SOURCES += keithley236.cpp
SOURCES += plot2d.cpp
SOURCES += plotpropertiesdlg.cpp


HEADERS += mainwindow.h \
    fake236.h \
    idstab.h \
    vgtab.h
HEADERS += axesdialog.h
HEADERS += AxisFrame.h
HEADERS += AxisLimits.h
HEADERS += configuredialog.h
HEADERS += DataSetProperties.h
HEADERS += datastream2d.h
HEADERS += filetab.h
HEADERS += gpibdevice.h
HEADERS +=
HEADERS += keithley236.h
HEADERS += plot2d.h
HEADERS += plotpropertiesdlg.h


FORMS   += mainwindow.ui


# For National Instruments GPIB Boards
LIBS += -L"/usr/local/lib" -lgpib # To include libgpib.so from /usr/local/lib
INCLUDEPATH += /usr/local/include


DISTFILES += doc/linux_Gpib_HowTo.txt \
    .gitignore \
    .gitignore
DISTFILES += LICENSE
DISTFILES += README.md
DISTFILES += doc/GPIBProgrammingReferenceManual.pdf
DISTFILES += doc/Keithley236Manual.pdf
DISTFILES += doc/Readme.txt


RESOURCES += resources.qrc
