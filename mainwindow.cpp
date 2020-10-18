#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <../Buggy/plot2d.h>
#include <../Buggy/dcmotor.h>
#include <../Buggy/rpmmeter.h>
#include <../Buggy/motorController.h>
#include <../Buggy/PID_v1.h>

#include <QThread>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    pUi(new Ui::MainWindow)
{
    pRightMotorThread = nullptr;
    pLeftMotorThread = nullptr;

    pUi->setupUi(this);
    pUi->LPslider->setRange(0, 700);
    pUi->LPslider->setTracking(true);
    pUi->LIslider->setTracking(true);
    pUi->LDslider->setTracking(true);

    pUi->RPslider->setRange(0, 700);
    pUi->RPslider->setTracking(true);
    pUi->RIslider->setTracking(true);
    pUi->RDslider->setTracking(true);

    restoreSettings();

    pUi->LPedit->setText(QString("%1").arg(pUi->LPslider->value()));
    pUi->LIedit->setText(QString("%1").arg(pUi->LIslider->value()));
    pUi->LDedit->setText(QString("%1").arg(pUi->LDslider->value()));

    pUi->RPedit->setText(QString("%1").arg(pUi->RPslider->value()));
    pUi->RIedit->setText(QString("%1").arg(pUi->RIslider->value()));
    pUi->RDedit->setText(QString("%1").arg(pUi->RDslider->value()));

    if(!initGpio())
        exit(EXIT_FAILURE);

    initPlots();

    // Create the speed meters for the two Motors
    leftSpeedPin     = 22;
    rightSpeedPin    = 5;
    pLeftSpeed  = new RPMmeter(leftSpeedPin,  gpioHostHandle);
    pRightSpeed = new RPMmeter(rightSpeedPin, gpioHostHandle);

    // Create the two Motors
    leftForwardPin   = 24;
    leftBackwardPin  = 23;
    rightForwardPin  = 27;
    rightBackwardPin = 17;
    pLeftMotor  = new DcMotor(leftForwardPin,  leftBackwardPin,  gpioHostHandle);
    pRightMotor = new DcMotor(rightForwardPin, rightBackwardPin, gpioHostHandle);

    // Create the two Motor Controllers
    pLMotor = new MotorController(pLeftMotor,  pLeftSpeed);
    pRMotor = new MotorController(pRightMotor, pRightSpeed);

    // Create the Motor Threads
    CreateLeftMotorThread();
    CreateRightMotorThread();

    // Send PID Parameters to the Motor Controller
    emit LPvalueChanged(double(pUi->LPslider->value()*0.01));
    emit LIvalueChanged(double(pUi->LIslider->value()*0.01));
    emit LDvalueChanged(double(pUi->LDslider->value()*0.01));

    emit RPvalueChanged(double(pUi->RPslider->value()*0.01));
    emit RIvalueChanged(double(pUi->RIslider->value()*0.01));
    emit RDvalueChanged(double(pUi->RDslider->value()*0.01));

    emit operate();
    connect(&testTimer, SIGNAL(timeout()),
            this, SLOT(changeSpeed()));
}

void
MainWindow::restoreSettings() {
    QSettings settings;
    restoreGeometry(settings.value(QString("ControlledMotor")).toByteArray());

    pUi->LPslider->setValue(settings.value(QString("LP_Value"), "0.0").toInt());
    pUi->LIslider->setValue(settings.value(QString("LI_Value"), "0.0").toInt());
    pUi->LDslider->setValue(settings.value(QString("LD_Value"), "0.0").toInt());

    pUi->RPslider->setValue(settings.value(QString("RP_Value"), "0.0").toInt());
    pUi->RIslider->setValue(settings.value(QString("RI_Value"), "0.0").toInt());
    pUi->RDslider->setValue(settings.value(QString("RD_Value"), "0.0").toInt());
}


void
MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue(QString("ControlledMotor"), saveGeometry());
    settings.setValue(QString("LP_Value"), pUi->LPslider->value());
    settings.setValue(QString("LI_Value"), pUi->LIslider->value());
    settings.setValue(QString("LD_Value"), pUi->LDslider->value());
    settings.setValue(QString("RP_Value"), pUi->RPslider->value());
    settings.setValue(QString("RI_Value"), pUi->RIslider->value());
    settings.setValue(QString("RD_Value"), pUi->RDslider->value());
}


void
MainWindow::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
    emit stopLMotor();
    emit stopRMotor();
    saveSettings();
    testTimer.stop();
    if(pLeftMotorThread) {
        pLeftMotorThread->quit();
        pLeftMotorThread->wait(1000);
        pLeftMotorThread->deleteLater();
    }
    if(pRightMotorThread) {
        pRightMotorThread->quit();
        pRightMotorThread->wait(1000);
        pRightMotorThread->deleteLater();
    }
    if(gpioHostHandle >= 0)
        pigpio_stop(gpioHostHandle);
    if(pLeftPlot)
        delete pLeftPlot;
    if(pRightPlot)
        delete pRightPlot;
    delete pUi;
}


MainWindow::~MainWindow() {
}


bool
MainWindow::initGpio() {
    char host[sizeof("localhost")+1] = {"localhost"};
    char port[sizeof("8888")+1] = {"8888"};
    gpioHostHandle = pigpio_start(host, port);
    if(gpioHostHandle < 0)
        return false;
    return true;
}


void
MainWindow::initPlots() {
    pLeftPlot = new Plot2D(nullptr, "Left Motor");

    pLeftPlot->NewDataSet(1, 2, QColor(255,   0, 255), Plot2D::iline, "SetPt");
    pLeftPlot->NewDataSet(2, 2, QColor(255, 255,   0), Plot2D::iline, "Speed");
    pLeftPlot->NewDataSet(3, 2, QColor(  0, 255, 255), Plot2D::iline, "PID-Out");

    pLeftPlot->SetShowTitle(1, true);
    pLeftPlot->SetShowTitle(2, true);
    pLeftPlot->SetShowTitle(3, true);

    pLeftPlot->SetShowDataSet(1, true);
    pLeftPlot->SetShowDataSet(2, true);
    pLeftPlot->SetShowDataSet(3, true);

    pLeftPlot->SetLimits(0.0, 1.0, -1.0, 1.0, true, true, false, false);
    pLeftPlot->UpdatePlot();
    pLeftPlot->show();

    nLeftPlotPoints = 0;

    pRightPlot = new Plot2D(nullptr, "Right Motor");

    pRightPlot->NewDataSet(1, 2, QColor(255,   0, 255), Plot2D::ipoint, "SetPt");
    pRightPlot->NewDataSet(2, 2, QColor(255, 255,   0), Plot2D::iline, "Speed");
    pRightPlot->NewDataSet(3, 2, QColor(  0, 255, 255), Plot2D::iline, "PID-Out");

    pRightPlot->SetShowTitle(1, true);
    pRightPlot->SetShowTitle(2, true);
    pRightPlot->SetShowTitle(3, true);

    pRightPlot->SetShowDataSet(1, true);
    pRightPlot->SetShowDataSet(2, true);
    pRightPlot->SetShowDataSet(3, true);

    pRightPlot->SetLimits(0.0, 1.0, -1.0, 1.0, true, true, false, false);
    pRightPlot->UpdatePlot();
    pRightPlot->show();

    nRightPlotPoints = 0;
}


void
MainWindow::CreateLeftMotorThread() {
    pLeftMotorThread = new QThread();
    pLMotor->moveToThread(pLeftMotorThread);
    connect(pLeftMotorThread, SIGNAL(finished()),
            this, SLOT(onLeftMotorThreadDone()));
    connect(this, SIGNAL(operate()),
            pLMotor, SLOT(go()));
    connect(this, SIGNAL(stopLMotor()),
            pLMotor, SLOT(terminate()));
    connect(this, SIGNAL(LPvalueChanged(double)),
            pLMotor, SLOT(setP(double)));
    connect(this, SIGNAL(LIvalueChanged(double)),
            pLMotor, SLOT(setI(double)));
    connect(this, SIGNAL(LDvalueChanged(double)),
            pLMotor, SLOT(setD(double)));
    connect(this, SIGNAL(LSpeedChanged(double)),
            pLMotor, SLOT(setSpeed(double)));

    // Start the Motor Controller Thread
    pLeftMotorThread->start();

    currentLspeed = 0.0;
    emit LSpeedChanged(currentLspeed);
}


void
MainWindow::CreateRightMotorThread() {
    pRightMotorThread = new QThread();
    pRMotor->moveToThread(pRightMotorThread);
    connect(pRightMotorThread, SIGNAL(finished()),
            this, SLOT(onRightMotorThreadDone()));
    connect(this, SIGNAL(operate()),
            pRMotor, SLOT(go()));
    connect(this, SIGNAL(stopRMotor()),
            pRMotor, SLOT(terminate()));
    connect(this, SIGNAL(RPvalueChanged(double)),
            pRMotor, SLOT(setP(double)));
    connect(this, SIGNAL(RIvalueChanged(double)),
            pRMotor, SLOT(setI(double)));
    connect(this, SIGNAL(RDvalueChanged(double)),
            pRMotor, SLOT(setD(double)));
    connect(this, SIGNAL(RSpeedChanged(double)),
            pRMotor, SLOT(setSpeed(double)));

    // Start the Motor Controller Thread
    pRightMotorThread->start();

    currentRspeed = 0.0;
    emit RSpeedChanged(currentRspeed);
}


void
MainWindow::on_buttonStart_clicked() {
    if(pUi->buttonStart->text()== QString("Start")) {
        connect(pLMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewLMotorValues(double, double, double)));
        pLeftPlot->ClearDataSet(1);
        pLeftPlot->ClearDataSet(2);
        pLeftPlot->ClearDataSet(3);
        nLeftPlotPoints = 0;

        connect(pRMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewRMotorValues(double, double, double)));
        pRightPlot->ClearDataSet(1);
        pRightPlot->ClearDataSet(2);
        pRightPlot->ClearDataSet(3);
        nRightPlotPoints = 0;

        pLMotor->setPIDmode(MANUAL);
        pRMotor->setPIDmode(MANUAL);

        changeSpeed();
        testTimer.start(3000);
        pUi->buttonStart->setText("Stop");
    }
    else {
        testTimer.stop();
        currentLspeed = 0.0;
        currentRspeed = 0.0;
        emit LSpeedChanged(currentLspeed);
        emit RSpeedChanged(currentRspeed);
        disconnect(pLMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewLMotorValues(double, double, double)));
        disconnect(pRMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewRMotorValues(double, double, double)));
        pUi->buttonStart->setText("Start");
    }
}


void
MainWindow::on_LPslider_valueChanged(int value) {
    pUi->LPedit->setText(QString("%1").arg(value));
    emit LPvalueChanged(double(value*0.01));
}


void
MainWindow::on_LIslider_valueChanged(int value) {
    pUi->LIedit->setText(QString("%1").arg(value));
    emit LIvalueChanged(double(value*0.001));
}


void
MainWindow::on_LDslider_valueChanged(int value) {
    pUi->LDedit->setText(QString("%1").arg(value));
    emit LDvalueChanged(double(value*0.01));
}


void
MainWindow::onLeftMotorThreadDone() {
    qDebug() << "onLeftMotorThreadDone()";
}


void
MainWindow::on_RPslider_valueChanged(int value) {
    pUi->RPedit->setText(QString("%1").arg(value));
    emit RPvalueChanged(double(value*0.01));
}


void
MainWindow::on_RIslider_valueChanged(int value) {
    pUi->RIedit->setText(QString("%1").arg(value));
    emit RIvalueChanged(double(value*0.001));
}


void
MainWindow::on_RDslider_valueChanged(int value) {
    pUi->RDedit->setText(QString("%1").arg(value));
    emit RDvalueChanged(double(value*0.01));
}


void
MainWindow::onRightMotorThreadDone() {
    qDebug() << "onRightMotorThreadDone()";
}


void
MainWindow::onNewLMotorValues(double wantedSpeed, double currentSpeed, double speed) {
    Q_UNUSED(wantedSpeed)
    pLeftPlot->NewPoint(1, double(nLeftPlotPoints), wantedSpeed);
    pLeftPlot->NewPoint(2, double(nLeftPlotPoints), currentSpeed);
    pLeftPlot->NewPoint(3, double(nLeftPlotPoints), speed);
    pLeftPlot->UpdatePlot();
    nLeftPlotPoints++;
}


void
MainWindow::onNewRMotorValues(double wantedSpeed, double currentSpeed, double speed) {
    Q_UNUSED(wantedSpeed)
    pRightPlot->NewPoint(1, double(nRightPlotPoints), wantedSpeed);
    pRightPlot->NewPoint(2, double(nRightPlotPoints), currentSpeed);
    pRightPlot->NewPoint(3, double(nRightPlotPoints), speed);
    pRightPlot->UpdatePlot();
    nRightPlotPoints++;
}


void
MainWindow::changeSpeed() {
    currentLspeed = 1.0 - currentLspeed;
    currentRspeed = 1.0 - currentRspeed;
    emit LSpeedChanged(currentLspeed);
    emit RSpeedChanged(currentRspeed);
}


void
MainWindow::on_buttonOpenLoop_clicked() {
    if(pUi->buttonOpenLoop->text() == QString("Stop")) {
        startStepTimer.stop();
        stopStepTimer.stop();
        currentLspeed = 0.0;
        currentRspeed = 0.0;
        emit LSpeedChanged(currentLspeed);
        emit RSpeedChanged(currentRspeed);
        disconnect(pLMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewLMotorValues(double, double, double)));
        disconnect(pRMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewRMotorValues(double, double, double)));
        pUi->buttonOpenLoop->setText("Open Loop");
    }
    else {
        connect(pLMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewLMotorValues(double, double, double)));
        pLeftPlot->ClearDataSet(1);
        pLeftPlot->ClearDataSet(2);
        pLeftPlot->ClearDataSet(3);
        nLeftPlotPoints = 0;

        connect(pRMotor, SIGNAL(MotorValues(double, double, double)),
                this, SLOT(onNewRMotorValues(double, double, double)));
        pRightPlot->ClearDataSet(1);
        pRightPlot->ClearDataSet(2);
        pRightPlot->ClearDataSet(3);
        nRightPlotPoints = 0;

        pLMotor->setPIDmode(MANUAL);
        pRMotor->setPIDmode(MANUAL);
        connect(&startStepTimer, SIGNAL(timeout()),
                 this, SLOT(onStartStep()));
        pUi->buttonOpenLoop->setText("Stop");
        startStepTimer.start(200);
    }
}


void
MainWindow::onStartStep() {
    currentLspeed = 0.5;
    currentRspeed = 0.5;
    emit LSpeedChanged(currentLspeed);
    emit RSpeedChanged(currentRspeed);
    connect(&stopStepTimer, SIGNAL(timeout()),
            this, SLOT(onStopStep()));
    stopStepTimer.start(1000);
    startStepTimer.stop();
    disconnect(&startStepTimer, SIGNAL(timeout()),
               this, SLOT(onStartStep()));
}


void
MainWindow::onStopStep() {
    currentLspeed = 0.0;
    currentRspeed = 0.0;
    emit LSpeedChanged(currentLspeed);
    emit RSpeedChanged(currentRspeed);

    stopStepTimer.stop();
    disconnect(&stopStepTimer, SIGNAL(timeout()),
               this, SLOT(onStopStep()));

    on_buttonOpenLoop_clicked();
}
