#-------------------------------------------------
#
# Project created by QtCreator 2020-10-13T15:09:21
#
#-------------------------------------------------

QT += core
QT += gui
QT += widgets

TARGET = controlledMotor
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    ../Buggy/axesdialog.cpp \
    ../Buggy/AxisFrame.cpp \
    ../Buggy/AxisLimits.cpp \
    ../Buggy/DataSetProperties.cpp \
    ../Buggy/datastream2d.cpp \
    ../Buggy/dcmotor.cpp \
    ../Buggy/PID_v1.cpp \
    ../Buggy/plot2d.cpp \
    ../Buggy/plotpropertiesdlg.cpp \
    ../Buggy/rpmmeter.cpp \
    ../Buggy/motorController.cpp

HEADERS += \
        mainwindow.h \
    ../Buggy/axesdialog.h \
    ../Buggy/AxisFrame.h \
    ../Buggy/AxisLimits.h \
    ../Buggy/DataSetProperties.h \
    ../Buggy/datastream2d.h \
    ../Buggy/dcmotor.h \
    ../Buggy/PID_v1.h \
    ../Buggy/plot2d.h \
    ../Buggy/plotpropertiesdlg.h \
    ../Buggy/rpmmeter.h \
    ../Buggy/motorController.h

FORMS += \
        mainwindow.ui

LIBS += -lpigpiod_if2 # To include libpigpiod_if2.so from /usr/local/lib
LIBS += -lrt
LIBS += -lpthread

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../Buggy/plot.png
