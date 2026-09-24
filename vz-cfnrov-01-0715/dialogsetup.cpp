#include "dialogsetup.h"
#include "ui_dialogsetup.h"
#include "mavlink/mavlink_manager.h"

#include <QString>
#include <QDebug>
#include <QtMath>

DialogSetup::DialogSetup(QWidget *parent, ActiveMap *map, MavlinkManager *mavlinkManager) :
    QDialog(parent),
    ui(new Ui::DialogSetup),
    m_mavlinkManager(mavlinkManager)
{
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) | Qt::WindowStaysOnTopHint);
    setFixedSize(860, 480);
    ui->setupUi(this);

    this->map = map;
    setMapValue();

    m_attTargetTimer = new QTimer(this);
    connect(m_attTargetTimer, &QTimer::timeout, this, &DialogSetup::onAttTargetTimer);
    connect(ui->pushButton_attStart, &QPushButton::clicked, this, &DialogSetup::on_pushButton_attStart_clicked);
    connect(ui->pushButton_attStop, &QPushButton::clicked, this, &DialogSetup::on_pushButton_attStop_clicked);

    ui->comboBox_frameConfig->addItem("0 - BlueROV1", 0);
    ui->comboBox_frameConfig->addItem("1 - Vectored", 1);
    ui->comboBox_frameConfig->addItem("2 - Vectored 6DOF", 2);
    ui->comboBox_frameConfig->addItem("3 - Vectored 6DOF 90", 3);
    ui->comboBox_frameConfig->addItem("4 - SimpleROV 3", 4);
    ui->comboBox_frameConfig->addItem("5 - SimpleROV 4", 5);
    ui->comboBox_frameConfig->addItem("6 - SimpleROV 5", 6);
    ui->comboBox_frameConfig->addItem("7 - Custom", 7);
    ui->comboBox_frameConfig->addItem("8 - Cleaner", 8);

    if (m_mavlinkManager) {
        int fc = m_mavlinkManager->getFrameConfig();
        int idx = ui->comboBox_frameConfig->findData(fc);
        if (idx >= 0) ui->comboBox_frameConfig->setCurrentIndex(idx);
    }
}

DialogSetup::~DialogSetup(){
    if (m_attTargetTimer) m_attTargetTimer->stop();
    delete ui;
}

void DialogSetup::setMapValue(){

    ui->lineEdit_addr->setText(map->addr);
    ui->lineEdit_port->setText(QString::number(map->port));

    ui->lineEdit_cameraLeft->code = map->frontCamera;
    ui->lineEdit_cameraLeft->setText(map->sFrontCamera);
    ui->lineEdit_led0Left->code = map->frontLed0;
    ui->lineEdit_led0Left->setText(map->sFrontLed0);
    ui->lineEdit_led1Left->code = map->frontLed1;
    ui->lineEdit_led1Left->setText(map->sFrontLed1);

    ui->lineEdit_cameraRight->code = map->backCamera;
    ui->lineEdit_cameraRight->setText(map->sBackCamera);
    ui->lineEdit_led0Right->code = map->backLed0;
    ui->lineEdit_led0Right->setText(map->sBackLed0);
    ui->lineEdit_led1Right->code = map->backLed1;
    ui->lineEdit_led1Right->setText(map->sBackLed1);

    ui->lineEdit_lt_up->code = map->padLT_up;
    ui->lineEdit_lt_up->setText(map->sPadLT_up);
    ui->lineEdit_lt_down->code = map->padLT_down;
    ui->lineEdit_lt_down->setText(map->sPadLT_down);
    ui->lineEdit_lt_left->code = map->padLT_left;
    ui->lineEdit_lt_left->setText(map->sPadLT_left);
    ui->lineEdit_lt_right->code = map->padLT_right;
    ui->lineEdit_lt_right->setText(map->sPadLT_right);

    ui->lineEdit_rt_up->code = map->padRT_up;
    ui->lineEdit_rt_up->setText(map->sPadRT_up);
    ui->lineEdit_rt_down->code = map->padRT_down;
    ui->lineEdit_rt_down->setText(map->sPadRT_down);
    ui->lineEdit_rt_left->code = map->padRT_left;
    ui->lineEdit_rt_left->setText(map->sPadRT_left);
    ui->lineEdit_rt_right->code = map->padRT_right;
    ui->lineEdit_rt_right->setText(map->sPadRT_right);

}



void DialogSetup::on_buttonBox_accepted(){

    map->addr = ui->lineEdit_addr->text();
    map->port = ui->lineEdit_port->text().toInt();

    map->frontCamera = ui->lineEdit_cameraLeft->code;
    map->sFrontCamera = ui->lineEdit_cameraLeft->text();
    map->frontLed0 = ui->lineEdit_led0Left->code;
    map->sFrontLed0 = ui->lineEdit_led0Left->text();
    map->frontLed1 = ui->lineEdit_led1Left->code;
    map->sFrontLed1 = ui->lineEdit_led1Left->text();

    map->backCamera = ui->lineEdit_cameraRight->code;
    map->sBackCamera = ui->lineEdit_cameraRight->text();
    map->backLed0 = ui->lineEdit_led0Right->code;
    map->sBackLed0 = ui->lineEdit_led0Right->text();
    map->backLed1 = ui->lineEdit_led1Right->code;
    map->sBackLed1 = ui->lineEdit_led1Right->text();

    map->padLT_up = ui->lineEdit_lt_up->code;
    map->sPadLT_up = ui->lineEdit_lt_up->text();
    map->padLT_down = ui->lineEdit_lt_down->code;
    map->sPadLT_down = ui->lineEdit_lt_down->text();
    map->padLT_left = ui->lineEdit_lt_left->code;
    map->sPadLT_left = ui->lineEdit_lt_left->text();
    map->padLT_right = ui->lineEdit_lt_right->code;
    map->sPadLT_right = ui->lineEdit_lt_right->text();


    map->padRT_up = ui->lineEdit_rt_up->code;
    map->sPadRT_up = ui->lineEdit_rt_up->text();
    map->padRT_down = ui->lineEdit_rt_down->code;
    map->sPadRT_down = ui->lineEdit_rt_down->text();
    map->padRT_left = ui->lineEdit_rt_left->code;
    map->sPadRT_left = ui->lineEdit_rt_left->text();
    map->padRT_right = ui->lineEdit_rt_right->code;
    map->sPadRT_right = ui->lineEdit_rt_right->text();

    if (m_mavlinkManager) {
        int fc = ui->comboBox_frameConfig->currentData().toInt();
        if (fc != m_mavlinkManager->getFrameConfig()) {
            m_mavlinkManager->setFrameConfig(fc);
        }
    }

}

void DialogSetup::on_pushButton_attStart_clicked()
{
    if (!m_mavlinkManager) {
        ui->label_attStatus->setText("状态: MavlinkManager未连接");
        return;
    }
    if (m_attTargetSending) return;

    m_attTargetSending = true;
    int rate = ui->spinBox_attRate->value();
    m_attTargetTimer->start(1000 / rate);
    ui->label_attStatus->setText(QString("状态: 正在发送 (%1Hz)").arg(rate));
}

void DialogSetup::on_pushButton_attStop_clicked()
{
    m_attTargetSending = false;
    m_attTargetTimer->stop();
    ui->label_attStatus->setText("状态: 已停止");
}

void DialogSetup::onAttTargetTimer()
{
    if (!m_mavlinkManager || !m_attTargetSending) return;

    float rollDeg = static_cast<float>(ui->doubleSpinBox_attRoll->value());
    float pitchDeg = static_cast<float>(ui->doubleSpinBox_attPitch->value());
    float yawDeg = static_cast<float>(ui->doubleSpinBox_attYaw->value());
    float thrust = static_cast<float>(ui->doubleSpinBox_attThrust->value());

    float rollRad = rollDeg * M_PI / 180.0f;
    float pitchRad = pitchDeg * M_PI / 180.0f;
    float yawRad = yawDeg * M_PI / 180.0f;

    float q[4];
    MavlinkManager::eulerToQuaternion(rollRad, pitchRad, yawRad, q);
    m_mavlinkManager->sendSetAttitudeTarget(q, thrust);
}
