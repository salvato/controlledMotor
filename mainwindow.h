#pragma once

#include <QMainWindow>
#include <QCloseEvent>
#include <QTimer>


namespace Ui {
class MainWindow;
}


QT_FORWARD_DECLARE_CLASS(Plot2D)
QT_FORWARD_DECLARE_CLASS(RPMmeter)
QT_FORWARD_DECLARE_CLASS(DcMotor)
QT_FORWARD_DECLARE_CLASS(MotorController)


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

protected:
    void restoreSettings();
    void saveSettings();
    void initPlots();
    bool initGpio();

signals:
    void operate();

    void stopLMotor();
    void LPvalueChanged(double Pvalue);
    void LIvalueChanged(double Ivalue);
    void LDvalueChanged(double Dvalue);
    void LSpeedChanged(double speed);

    void stopRMotor();
    void RPvalueChanged(double Pvalue);
    void RIvalueChanged(double Ivalue);
    void RDvalueChanged(double Dvalue);
    void RSpeedChanged(double speed);

private slots:
    void on_buttonStart_clicked();

    void on_LPslider_valueChanged(int value);
    void on_LIslider_valueChanged(int value);
    void on_LDslider_valueChanged(int value);
    void onLeftMotorThreadDone();
    void onNewLMotorValues(double wantedSpeed, double currentSpeed, double speed);

    void on_RPslider_valueChanged(int value);
    void on_RIslider_valueChanged(int value);
    void on_RDslider_valueChanged(int value);
    void onRightMotorThreadDone();
    void onNewRMotorValues(double wantedSpeed, double currentSpeed, double speed);

    void changeSpeed();

    void on_buttonOpenLoop_clicked();

private:
    void CreateLeftMotorThread();
    void CreateRightMotorThread();

private:
    uint leftSpeedPin;
    uint rightSpeedPin;
    uint leftForwardPin;
    uint leftBackwardPin;
    uint rightForwardPin;
    uint rightBackwardPin;

    int   gpioHostHandle;
    int   nLeftPlotPoints;
    int   nRightPlotPoints;

    double currentLspeed;
    double currentRspeed;

    Ui::MainWindow*  pUi;

    Plot2D*          pLeftPlot;
    Plot2D*          pRightPlot;
    RPMmeter*        pLeftSpeed;
    RPMmeter*        pRightSpeed;
    DcMotor*         pLeftMotor;
    DcMotor*         pRightMotor;
    MotorController* pLMotor;
    MotorController* pRMotor;
    QThread*         pLeftMotorThread;
    QThread*         pRightMotorThread;
    QTimer           testTimer;
};

