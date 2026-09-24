/***************************************************************************//**
 * @file example/MainWindow.cpp
 * @author  Marek M. Cel <marekcel@marekcel.pl>
 *
 * @section LICENSE
 *
 * Copyright (C) 2013 Marek M. Cel
 *
 * This file is part of QFlightInstruments. You can redistribute and modify it
 * under the terms of GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Further information about the GNU General Public License can also be found
 * on the world wide web at http://www.gnu.org.
 *
 * ---
 *
 * Copyright (C) 2013 Marek M. Cel
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 ******************************************************************************/

#include <iostream>
#include <cmath>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#include "dialogsetup.h"
#include <QSettings>
#include <QDir>
#include <QDateTime>
#include <QLocale>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStandardPaths>
#include <QtMath>
#include <QApplication>
#include <QScreen>
#include <QCursor>

namespace {
constexpr uint16_t kCustom1ButtonMask = uint16_t(1u << 1); // BTN1 / buttons bit 1 / value 2
constexpr uint16_t kCustom2ButtonMask = uint16_t(1u << 2); // BTN2 / buttons bit 2 / value 4
constexpr uint16_t kCustomButtonMask = uint16_t(kCustom1ButtonMask | kCustom2ButtonMask);
}

// 主窗口先从配置文件恢复 ROV 地址和按键映射，再建立界面、
// 视频与 MAVLink 连接；配置读取保持在控件初始化之前。
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    if(1==1)
    {

        QSettings *iniFile = new QSettings("setup", QSettings::IniFormat);
        iniFile->beginGroup("system");
        actMap.addr = iniFile->value("rovaddr", "192.168.1.101").toString();
        actMap.port = iniFile->value("rovport", 15001).toInt();
        iniFile->endGroup();

        iniFile->beginGroup("map");
        actMap.frontCamera = iniFile->value("fc").toInt();
        actMap.sFrontCamera = iniFile->value("fcs").toString();
        actMap.frontLed0 = iniFile->value("fl0").toInt();
        actMap.sFrontLed0 = iniFile->value("fl0s").toString();
        actMap.frontLed1 = iniFile->value("fl1").toInt();
        actMap.sFrontLed1 = iniFile->value("fl1s").toString();

        actMap.backCamera = iniFile->value("bc").toInt();
        actMap.sBackCamera = iniFile->value("bcs").toString();
        actMap.backLed0 = iniFile->value("bl0").toInt();
        actMap.sBackLed0 = iniFile->value("bl0s").toString();
        actMap.backLed1 = iniFile->value("bl1").toInt();
        actMap.sBackLed1 = iniFile->value("bl1s").toString();

        actMap.padLT_up = iniFile->value("lt_up").toInt();
        actMap.sPadLT_up = iniFile->value("lt_ups").toString();
        actMap.padLT_down = iniFile->value("lt_down").toInt();
        actMap.sPadLT_down = iniFile->value("lt_downs").toString();
        actMap.padLT_left = iniFile->value("lt_left").toInt();
        actMap.sPadLT_left = iniFile->value("lt_lefts").toString();
        actMap.padLT_right = iniFile->value("lt_right").toInt();
        actMap.sPadLT_right = iniFile->value("lt_rights").toString();

        actMap.padLB_up = iniFile->value("lb_up").toInt();
        actMap.sPadLB_up = iniFile->value("lb_ups").toString();
        actMap.padLB_down = iniFile->value("lb_down").toInt();
        actMap.sPadLB_down = iniFile->value("lb_downs").toString();
        actMap.padLB_left = iniFile->value("lb_left").toInt();
        actMap.sPadLB_left = iniFile->value("lb_lefts").toString();
        actMap.padLB_right = iniFile->value("lb_right").toInt();
        actMap.sPadLB_right = iniFile->value("lb_rights").toString();

        actMap.padRT_up = iniFile->value("rt_up").toInt();
        actMap.sPadRT_up = iniFile->value("rt_ups").toString();
        actMap.padRT_down = iniFile->value("rt_down").toInt();
        actMap.sPadRT_down = iniFile->value("rt_downs").toString();
        actMap.padRT_left = iniFile->value("rt_left").toInt();
        actMap.sPadRT_left = iniFile->value("rt_lefts").toString();
        actMap.padRT_right = iniFile->value("rt_right").toInt();
        actMap.sPadRT_right = iniFile->value("rt_rights").toString();

        actMap.padRB_up = iniFile->value("rb_up").toInt();
        actMap.sPadRB_up = iniFile->value("rb_ups").toString();
        actMap.padRB_down = iniFile->value("rb_down").toInt();
        actMap.sPadRB_down = iniFile->value("rb_downs").toString();
        actMap.padRB_left = iniFile->value("rb_left").toInt();
        actMap.sPadRB_left = iniFile->value("rb_lefts").toString();
        actMap.padRB_right = iniFile->value("rb_right").toInt();
        actMap.sPadRB_right = iniFile->value("rb_rights").toString();
        iniFile->endGroup();

        delete iniFile;
        qDebug() << "config =" << actMap.addr << ":" << actMap.port;
    }

    ui->setupUi(this);

    // 在原预留区域中安装 Pixhawk 三维姿态模型。
    auto *attitude3DLayout = new QVBoxLayout(ui->widget_4);
    attitude3DLayout->setContentsMargins(0, 0, 0, 0);
    attitude3DLayout->setSpacing(0);
    m_attitude3DWidget = new Attitude3DWidget(ui->widget_4);
    attitude3DLayout->addWidget(m_attitude3DWidget);

    // 顶栏按钮 hover/pressed 视觉反馈
    QString topBtnStyle =
        "QPushButton{border:none;background:transparent;}"
        "QPushButton:hover{background:rgba(255,255,255,40);border-radius:3px;}"
        "QPushButton:pressed{background:rgba(255,255,255,80);border-radius:3px;}";
    ui->pushButton_fullscreen->setStyleSheet(topBtnStyle);
    ui->pushButton_setup->setStyleSheet(topBtnStyle);
    ui->pushButton_close->setStyleSheet(topBtnStyle);

    // 云台方向按钮控制
    connect(ui->btnLeft_lt, &QPushButton::pressed, this, [this]() {
        m_panLeftPressed = true;
        m_gimbalControlTimer->start();
    });
    connect(ui->btnLeft_lt, &QPushButton::released, this, [this]() {
        m_panLeftPressed = false;
    });
    connect(ui->btnRight_lt, &QPushButton::pressed, this, [this]() {
        m_panRightPressed = true;
        m_gimbalControlTimer->start();
    });
    connect(ui->btnRight_lt, &QPushButton::released, this, [this]() {
        m_panRightPressed = false;
    });
    connect(ui->btnUp_lt, &QPushButton::pressed, this, [this]() {
        m_tiltUpPressed = true;
        m_gimbalControlTimer->start();
    });
    connect(ui->btnUp_lt, &QPushButton::released, this, [this]() {
        m_tiltUpPressed = false;
    });
    connect(ui->btnDown_lt, &QPushButton::pressed, this, [this]() {
        m_tiltDownPressed = true;
        m_gimbalControlTimer->start();
    });
    connect(ui->btnDown_lt, &QPushButton::released, this, [this]() {
        m_tiltDownPressed = false;
    });

    // 前云台回中按钮
    connect(ui->btnFrontGimbalReset, &QPushButton::clicked, this, [this]() {
        if (m_mavlinkManager) {
            m_mavlinkManager->centerFrontCameraGimbal();
            qDebug() << "前云台已回中";
        }
    });

    // 后云台方向按钮控制
    connect(ui->btnLeft_3, &QPushButton::pressed, this, [this]() {
        m_rearPanLeftPressed = true;
        m_rearGimbalControlTimer->start();
    });
    connect(ui->btnLeft_3, &QPushButton::released, this, [this]() {
        m_rearPanLeftPressed = false;
    });
    connect(ui->btnRight_3, &QPushButton::pressed, this, [this]() {
        m_rearPanRightPressed = true;
        m_rearGimbalControlTimer->start();
    });
    connect(ui->btnRight_3, &QPushButton::released, this, [this]() {
        m_rearPanRightPressed = false;
    });
    connect(ui->btnUp_3, &QPushButton::pressed, this, [this]() {
        m_rearTiltUpPressed = true;
        m_rearGimbalControlTimer->start();
    });
    connect(ui->btnUp_3, &QPushButton::released, this, [this]() {
        m_rearTiltUpPressed = false;
    });
    connect(ui->btnDown_3, &QPushButton::pressed, this, [this]() {
        m_rearTiltDownPressed = true;
        m_rearGimbalControlTimer->start();
    });
    connect(ui->btnDown_3, &QPushButton::released, this, [this]() {
        m_rearTiltDownPressed = false;
    });

    // 后云台回中按钮
    connect(ui->btnRearGimbalReset, &QPushButton::clicked, this, [this]() {
        if (m_mavlinkManager) {
            m_mavlinkManager->centerRearCameraGimbal();
            qDebug() << "后云台已回中";
        }
    });

    // 初始化状态栏
    labCell_0 = new QLabel("-", this);
    labCell_1=new QLabel("-", this);
    labCell_0->setObjectName(QStringLiteral("armStatusBadge"));
    labCell_1->setObjectName(QStringLiteral("systemStatusText"));
    ui->statusbar->addWidget(labCell_0, 0);
    ui->statusbar->addWidget(labCell_1, 1);
    setStatusBadge(labCell_0, QString::fromUtf8("未解锁"), "danger");
    labCell_1->setText(QString::fromUtf8("系统正在初始化…"));

    labCell_rpy = new QLabel("R:-- P:-- Y:--", this);
    labCell_rpy->setObjectName(QStringLiteral("attitudeBadge"));
    ui->statusbar->addPermanentWidget(labCell_rpy);

    labCell_2=new QLabel("未连接", this);
    labCell_2->setObjectName(QStringLiteral("connectionBadge"));
    ui->statusbar->addPermanentWidget(labCell_2);
    setStatusBadge(labCell_2, QString::fromUtf8("未连接"), "warning");


    ui->widget_ctrlPanel->setStyleSheet("#widget_ctrlPanel{background-color:#000;}#widget_leftCtrlPanel{border:1px solid gray}#widget_rightCtrlPanel{border:1px solid gray}");

    // 初始化补光灯亮度滑动条
    ui->label_leftLight->setStyleSheet("color:#FFF; font-weight:bold;");
    ui->label_rightLight->setStyleSheet("color:#FFF; font-weight:bold;");
    ui->slider_frontLight->setInvertedAppearance(true);
    ui->slider_frontLight->setInvertedControls(false);
    ui->slider_rearLight->setInvertedAppearance(true);
    ui->slider_rearLight->setInvertedControls(false);
    ui->slider_frontLight->setStyleSheet(
            "QSlider::sub-page:vertical{background:#3A3A3A;border-radius:5px;}"
            "QSlider::handle:vertical{background:#FFFFFF;width:18px;height:18px;margin:-4px 0;border-radius:9px;}"
            "QSlider::handle:vertical:hover{background:#E0E0FF;}"
            "QSlider::handle:vertical:pressed{background:#82B1FF;}"
            "QSlider::groove:vertical{background:#2979FF;width:10px;border-radius:5px;}");
    ui->slider_rearLight->setStyleSheet(
           "QSlider::sub-page:vertical{background:#3A3A3A;border-radius:5px;}"
           "QSlider::handle:vertical{background:#FFFFFF;width:18px;height:18px;margin:-4px 0;border-radius:9px;}"
           "QSlider::handle:vertical:hover{background:#E0E0FF;}"
           "QSlider::handle:vertical:pressed{background:#82B1FF;}"
           "QSlider::groove:vertical{background:#2979FF;width:10px;border-radius:5px;}");
    connect(ui->slider_frontLight, &QSlider::valueChanged, this, &MainWindow::onFrontLightBrightnessChanged);
    connect(ui->slider_rearLight, &QSlider::valueChanged, this, &MainWindow::onRearLightBrightnessChanged);
    ui->slider_frontLight->setValue(0);
    ui->slider_rearLight->setValue(0);
    applyProfessionalTheme();
    MainWindow::showFullScreen();
    ui->statusbar->show();

    m_frontLightOn = false;
    m_rearLightOn = false;


    m_udpHandler=new UdpHandler(this);
    m_udpDataCount=0;
    connect(m_udpHandler, &UdpHandler::dataReceived, this, &MainWindow::onUdpDataReceived);
    connect(m_udpHandler, &UdpHandler::connected, this, &MainWindow::onUdpConnected);
    connect(m_udpHandler, &UdpHandler::disconnected, this, &MainWindow::onUdpDisconnected);
    // UDP 自动连接
    QTimer::singleShot(1000, this, [this]() {
        m_udpHandler->setRemoteAddress(actMap.addr, actMap.port);
        m_udpHandler->bind(5555);
    });

    // 前摄像头
    m_rtspPlayer=new FFmpegRtspPlayer(this);
    m_rtspPlayer->setShowTimestamp(true);
    m_rtspPlayer->setTimestampFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_rtspPlayer->setTimestampPrefix(QString::fromUtf8("前摄像头"));
    m_rtspPlayer->setTimestampPosition(Qt::TopRightCorner);
    connect(m_rtspPlayer, &FFmpegRtspPlayer::started, this, &MainWindow::onRtspStarted);
    connect(m_rtspPlayer, &FFmpegRtspPlayer::stopped, this, &MainWindow::onRtspStopped);
    connect(m_rtspPlayer, &FFmpegRtspPlayer::error, this, &MainWindow::onRtspError);
    connect(m_rtspPlayer, &FFmpegRtspPlayer::fpsUpdated, this, &MainWindow::onRtspFpsUpdated);
    connect(m_rtspPlayer, &FFmpegRtspPlayer::videoSizeChanged, this, &MainWindow::onVideoSizeChanged);
    connect(m_rtspPlayer, &FFmpegRtspPlayer::newFrame, this, &MainWindow::onNewFrameReceived);
    ui->widget_camera_0->installEventFilter(this);

    // 后摄像头
    m_rtspPlayerRear=new FFmpegRtspPlayer(this);
    m_rtspPlayerRear->setShowTimestamp(true);
    m_rtspPlayerRear->setTimestampFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_rtspPlayerRear->setTimestampPrefix(QString::fromUtf8("后摄像头"));
    m_rtspPlayerRear->setTimestampPosition(Qt::TopRightCorner);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::started, this, &MainWindow::onRearRtspStarted);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::stopped, this, &MainWindow::onRearRtspStopped);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::error, this, &MainWindow::onRearRtspError);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::fpsUpdated, this, &MainWindow::onRearRtspFpsUpdated);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::videoSizeChanged, this, &MainWindow::onRearVideoSizeChanged);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::newFrame, this, &MainWindow::onRearNewFrameReceived);
    ui->widget_camera_back_1->installEventFilter(this);

    // 双路录像按钮悬浮在视频画面左上角，录像文件包含画面时间水印。
    m_frontRecordButton = ui->frontRecordButton;
    m_frontRecordButton->setCursor(Qt::PointingHandCursor);
    m_frontRecordButton->setToolTip(QString::fromUtf8("录制前摄像头视频到本地 AVI 文件"));
    updateRecordingButton(m_frontRecordButton, false);
    m_frontRecordButton->raise();
    m_frontRecordButton->show();

    m_rearRecordButton = ui->rearRecordButton;
    m_rearRecordButton->setCursor(Qt::PointingHandCursor);
    m_rearRecordButton->setToolTip(QString::fromUtf8("录制后摄像头视频到本地 AVI 文件"));
    updateRecordingButton(m_rearRecordButton, false);
    m_rearRecordButton->raise();
    m_rearRecordButton->show();

    // 放大按钮由 Designer 创建，运行时根据视频控件尺寸贴在右下角。
    m_frontExpandButton = ui->frontExpandButton;
    m_rearExpandButton = ui->rearExpandButton;
    m_frontExpandButton->setToolTip(QString::fromUtf8("在独立窗口中放大前摄像头画面"));
    m_rearExpandButton->setToolTip(QString::fromUtf8("在独立窗口中放大后摄像头画面"));
    connect(m_frontExpandButton, &QPushButton::clicked, this, [this]() {
        showCameraPreview(false);
    });
    connect(m_rearExpandButton, &QPushButton::clicked, this, [this]() {
        showCameraPreview(true);
    });
    updateCameraExpandButtonPositions();

    // 没有视频帧时也每秒刷新摄像头名称和本机日期时间。
    auto *videoOverlayTimer = new QTimer(this);
    videoOverlayTimer->setInterval(1000);
    connect(videoOverlayTimer, &QTimer::timeout, this, [this]() {
        if (ui->widget_camera_0)
            ui->widget_camera_0->update();
        if (ui->widget_camera_back_1)
            ui->widget_camera_back_1->update();
        if (m_frontRecordButton)
            m_frontRecordButton->raise();
        if (m_rearRecordButton)
            m_rearRecordButton->raise();
        if (m_frontExpandButton)
            m_frontExpandButton->raise();
        if (m_rearExpandButton)
            m_rearExpandButton->raise();
        if (m_frontPreviewCanvas)
            m_frontPreviewCanvas->update();
        if (m_rearPreviewCanvas)
            m_rearPreviewCanvas->update();
    });
    videoOverlayTimer->start();

    connect(m_frontRecordButton, &QPushButton::clicked, this, [this]() {
        toggleVideoRecording(m_rtspPlayer, QStringLiteral("front"),
                             QString::fromUtf8("前摄像头"));
    });
    connect(m_rearRecordButton, &QPushButton::clicked, this, [this]() {
        toggleVideoRecording(m_rtspPlayerRear, QStringLiteral("rear"),
                             QString::fromUtf8("后摄像头"));
    });

    connect(m_rtspPlayer, &FFmpegRtspPlayer::recordingStarted, this,
            [this](const QString &filePath) {
        updateRecordingButton(m_frontRecordButton, true);
        setStatusBadge(labCell_2, QString::fromUtf8("前摄像头录像中"), "danger");
        labCell_2->setToolTip(filePath);
    });
    connect(m_rtspPlayer, &FFmpegRtspPlayer::recordingStopped, this,
            [this](const QString &filePath) {
        updateRecordingButton(m_frontRecordButton, false);
        setStatusBadge(labCell_2, QString::fromUtf8("前摄像头录像已保存"), "info");
        labCell_2->setToolTip(filePath);
    });
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::recordingStarted, this,
            [this](const QString &filePath) {
        updateRecordingButton(m_rearRecordButton, true);
        setStatusBadge(labCell_2, QString::fromUtf8("后摄像头录像中"), "danger");
        labCell_2->setToolTip(filePath);
    });
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::recordingStopped, this,
            [this](const QString &filePath) {
        updateRecordingButton(m_rearRecordButton, false);
        setStatusBadge(labCell_2, QString::fromUtf8("后摄像头录像已保存"), "info");
        labCell_2->setToolTip(filePath);
    });
    const auto showRecordingError = [this](const QString &message) {
        QMessageBox::warning(this, QString::fromUtf8("录像失败"), message);
    };
    connect(m_rtspPlayer, &FFmpegRtspPlayer::recordingError,
            this, showRecordingError);
    connect(m_rtspPlayerRear, &FFmpegRtspPlayer::recordingError,
            this, showRecordingError);

    // 摄像头重连定时器
    m_frontCamRetryTimer = new QTimer(this);
    m_frontCamRetryTimer->setInterval(5000);
    m_frontCamRetryTimer->setSingleShot(true);
    connect(m_frontCamRetryTimer, &QTimer::timeout, this, &MainWindow::onFrontCamRetryTimeout);
    m_rearCamRetryTimer = new QTimer(this);
    m_rearCamRetryTimer->setInterval(5000);
    m_rearCamRetryTimer->setSingleShot(true);
    connect(m_rearCamRetryTimer, &QTimer::timeout, this, &MainWindow::onRearCamRetryTimeout);

    m_heartbeatCheckTimer = new QTimer(this);
    m_heartbeatCheckTimer->setInterval(3000);
    connect(m_heartbeatCheckTimer, &QTimer::timeout, this, &MainWindow::onHeartbeatCheckTimeout);

    m_frontFrameCheckTimer = new QTimer(this);
    m_frontFrameCheckTimer->setInterval(5000);
    m_frontFrameCheckTimer->setSingleShot(true);
    connect(m_frontFrameCheckTimer, &QTimer::timeout, this, &MainWindow::onFrontFrameCheckTimeout);

    m_rearFrameCheckTimer = new QTimer(this);
    m_rearFrameCheckTimer->setInterval(5000);
    m_rearFrameCheckTimer->setSingleShot(true);
    connect(m_rearFrameCheckTimer, &QTimer::timeout, this, &MainWindow::onRearFrameCheckTimeout);

    // 自动打开双路摄像头。统一走同一入口，保证初次启动与设置变更行为一致。
    QTimer::singleShot(2000, this, &MainWindow::restartCameraStreams);


    // 初始化 MAVLink 管理器
    m_mavlinkManager = new MavlinkManager(this);

    // 连接 MAVLink 管理器信号
    connect(m_mavlinkManager, &MavlinkManager::sendData,
            [this](const QByteArray &data) {
        if (m_udpHandler && m_udpHandler->isConnected()) {
            m_udpHandler->sendData(data);
        }
    });

    connect(m_mavlinkManager, &MavlinkManager::heartbeatReceived,
            this, &MainWindow::onHeartbeatReceived);
    connect(m_mavlinkManager, &MavlinkManager::attitudeUpdated,
            this, &MainWindow::onAttitudeUpdated);
    connect(m_mavlinkManager, &MavlinkManager::gpsUpdated,
            this, &MainWindow::onGpsUpdated);
    connect(m_mavlinkManager, &MavlinkManager::statusTextReceived,
            this, &MainWindow::onStatusTextReceived);
    connect(m_mavlinkManager, &MavlinkManager::commandAckReceived,
            this, &MainWindow::onCommandAckReceived);

    // 姿态旋转信号连接
    connect(m_mavlinkManager, &MavlinkManager::rotationStateChanged,
            this, &MainWindow::onRotationStateChanged);
    connect(m_mavlinkManager, &MavlinkManager::machineOrientationChanged,
            this, &MainWindow::onMachineOrientationChanged);

    // GUIDED清洗状态信号连接
    connect(m_mavlinkManager, &MavlinkManager::guidedCleanStateChanged,
            this, [this](GuidedCleanState state) {
        Q_UNUSED(state);
    });

    connect(m_mavlinkManager, &MavlinkManager::frameConfigChanged,
            this, &MainWindow::onFrameConfigChanged);

    connect(ui->pushButton_switchToClean, &QPushButton::clicked,
            this, &MainWindow::onRotationToCleanClicked);
    connect(ui->pushButton_switchToDive, &QPushButton::clicked,
            this, &MainWindow::onRotationToDiveClicked);

    connect(ui->comboBox_flightMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFlightModeChanged);

    m_slideArm = new SlideToUnlock(SlideToUnlock::Arm, ui->armSlideHost);
    m_slideDisarm = new SlideToUnlock(SlideToUnlock::Disarm, ui->disarmSlideHost);
    auto *armSlideLayout = new QVBoxLayout(ui->armSlideHost);
    armSlideLayout->setContentsMargins(0, 0, 0, 0);
    armSlideLayout->addWidget(m_slideArm);
    auto *disarmSlideLayout = new QVBoxLayout(ui->disarmSlideHost);
    disarmSlideLayout->setContentsMargins(0, 0, 0, 0);
    disarmSlideLayout->addWidget(m_slideDisarm);
    connect(m_slideArm, &SlideToUnlock::actionTriggered, this, &MainWindow::onSlideArmTriggered);
    connect(m_slideDisarm, &SlideToUnlock::actionTriggered, this, &MainWindow::onSlideDisarmTriggered);

    //    // 添加 PFD 数据更新连接
    connect(m_mavlinkManager, &MavlinkManager::vehicleStateUpdated,
            this, &MainWindow::updatePfdFromMavlink);

    // 初始化手动控制定时器
    m_manualControlTimer = new QTimer(this);
    m_manualControlTimer->setInterval(40);  // 25Hz, 与QGC一致
    connect(m_manualControlTimer, &QTimer::timeout,
            this, &MainWindow::onManualControlTimer);
    // 遥测流只在收到真实飞控心跳后请求，避免把UDP已绑定误认为Pixhawk在线。

    // 初始化云台控制定时器
    m_gimbalControlTimer = new QTimer(this);
    m_gimbalControlTimer->setInterval(100);
    connect(m_gimbalControlTimer, &QTimer::timeout, this, &MainWindow::onGimbalControlTimer);

    m_rearGimbalControlTimer = new QTimer(this);
    m_rearGimbalControlTimer->setInterval(100);
    connect(m_rearGimbalControlTimer, &QTimer::timeout, this, &MainWindow::onRearGimbalControlTimer);

    // 初始化游戏手柄
    initGamepad();
    initAttitudeLog();
    if (m_mavlinkManager) {
        m_mavlinkManager->initCommandLog();
        m_mavlinkManager->initAllLog();
    }

    // 连接 MavlinkManager 的云台状态更新信号
    connect(m_mavlinkManager, &MavlinkManager::frontGimbalUpdated,
            this, [this](uint16_t pan, uint16_t tilt) {
        // 可以在这里更新UI显示云台位置
        qDebug() << "前云台位置更新 - Pan:" << pan << "Tilt:" << tilt;
    });

#if 0
    m_vf = new QCameraViewfinder(ui->widget_camera_0);
    m_vf->resize(ui->widget_camera_0->size());
    ui->widget_camera_0->close();
#else

#endif

    // 初始化虚拟摇杆
    m_virtualJoystick = new VirtualJoystick(false,this);
    m_virtualJoystick->setMavlinkManager(m_mavlinkManager);
    m_virtualJoystick->setAutoCenterThrottle(true);  // 油门回中
    m_virtualJoystick->setLeftHandedMode(false);      // 左手模式：箭头显示在右摇杆上；右手模式：箭头显示在左摇杆上
    // VirtualJoystick作为覆盖层会拦截鼠标事件，导致顶栏按钮无法点击
    // 设置鼠标事件透传，让摇杆子控件自行处理鼠标事件
    m_virtualJoystick->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_virtualJoystick->setVisible(true);
    m_virtualJoystickEnabled = true;

    // 设置虚拟摇杆的最小高度
    // 设置固定大小而不是依赖布局
    //    m_virtualJoystick->setMinimumSize(150, 150);  // 设置最小尺寸
    //    m_virtualJoystick->setFixedHeight(200);        // 固定高度
    //    m_virtualJoystick->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);


    // 左右摇杆由运行时代码放入 UI 预留容器；容器在 MainWindow.ui 中居中。
    // 这里仅安装摇杆，不在代码中修改控制区的位置。
    // 将左摇杆显示到 widget_leftBottomPad 区域
    if (ui->widget_leftBottomPad) {
        ui->widget_leftBottomPad->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        ui->widget_leftBottomPad->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QGridLayout *leftPadLayout = qobject_cast<QGridLayout*>(ui->widget_leftBottomPad->layout());
        if (leftPadLayout && m_virtualJoystick->getLeftStick()) {
            m_virtualJoystick->getLeftStick()->setParent(ui->widget_leftBottomPad);
            m_virtualJoystick->getLeftStick()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            leftPadLayout->addWidget(m_virtualJoystick->getLeftStick(), 0, 0, 4, 4);
        }
    }
    // 将右摇杆显示到 widget_9 区域
    if (ui->widget_9) {
        ui->widget_9->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        ui->widget_9->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QGridLayout *rightPadLayout = qobject_cast<QGridLayout*>(ui->widget_9->layout());
        if (rightPadLayout && m_virtualJoystick->getRightStick()) {
            m_virtualJoystick->getRightStick()->setParent(ui->widget_9);
            m_virtualJoystick->getRightStick()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            rightPadLayout->addWidget(m_virtualJoystick->getRightStick(), 0, 0, 4, 4);
        }
    }
    // 当连接成功时
    connect(m_udpHandler, &UdpHandler::connected, this, [this]() {
        m_virtualJoystick->setInitialConnectComplete(false);
    });

    // 当断开连接时
    connect(m_udpHandler, &UdpHandler::disconnected, this, [this]() {
        m_virtualJoystick->setInitialConnectComplete(false);
        m_virtualJoystick->reCenterAll();
    });

    connect(m_virtualJoystick, &VirtualJoystick::joystickValueChanged,
            this, &MainWindow::onJoystickValueChanged);

    // 所有操作控件创建完成后，迁移到四行 ROV 专业控制台。
    setupFourRowDashboard();

    // PWM显示 DockWidget
    m_pwmDock = new QDockWidget(tr("6路PWM值"), this);
    m_pwmDisplayWidget = new PwmDisplayWidget(m_pwmDock);
    m_pwmDock->setWidget(m_pwmDisplayWidget);
    m_pwmDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_pwmDock->setFeatures(QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable |
                           QDockWidget::DockWidgetClosable);
    m_pwmDock->setFloating(true);
    m_pwmDock->hide();
    m_pwmDock->setMinimumSize(320, 400);
    addDockWidget(Qt::RightDockWidgetArea, m_pwmDock);

    ui->dashboardPwmButton->setCheckable(true);
    ui->dashboardPwmButton->setToolTip(tr("显示/隐藏PWM值"));

    // 连接舵机输出信号
    connect(m_mavlinkManager, &MavlinkManager::servoUpdated,
            this, &MainWindow::onServoUpdated);

    // 连接PWM反转信号
    connect(m_pwmDisplayWidget, &PwmDisplayWidget::reverseChanged,
            this, &MainWindow::onPwmReverseChanged);

    // 连接PWM启用信号
    connect(m_pwmDisplayWidget, &PwmDisplayWidget::enabledChanged,
            this, &MainWindow::onPwmEnabledChanged);

    // 连接PID参数设置信号
    connect(m_pwmDisplayWidget, &PwmDisplayWidget::pidParamSetRequested,
            this, [this](const QString &paramId, float value) {
        if (!m_mavlinkManager) return;
        m_mavlinkManager->sendParamSet(paramId.toUtf8().constData(),
                                       value, MAV_PARAM_TYPE_REAL32);
        qDebug() << "设置PID参数" << paramId << "=" << value;
    });

    // 连接PID参数写入EEPROM信号
    connect(m_pwmDisplayWidget, &PwmDisplayWidget::pidSaveToEepromRequested,
            this, [this]() {
        if (!m_mavlinkManager) return;
        m_mavlinkManager->sendCommandLong(MAV_CMD_PREFLIGHT_STORAGE, 1.0f);
        qDebug() << "PID参数写入EEPROM";
    });

    // 连接参数值信号
    connect(m_mavlinkManager, &MavlinkManager::paramValueReceived,
            this, &MainWindow::onParamValueReceived);

}

// 更新状态文字和动态样式属性；重新抛光控件以立即刷新 Qt 样式表。
void MainWindow::setStatusBadge(QLabel *label, const QString &text, const char *state)
{
    if (!label)
        return;
    label->setText(text);
    label->setProperty("statusBadge", true);
    label->setProperty("state", QString::fromLatin1(state));
    label->style()->unpolish(label);
    label->style()->polish(label);
    label->update();
}

// 四行面板的静态布局由 MainWindow.ui 维护；这里只向占位控件安装
// 需要自绘的仪表，并连接急停、设置、帮助和时钟的运行时行为。
void MainWindow::setupFourRowDashboard()
{
    // MainWindow.ui owns the layout. Only painted gauges need runtime widgets.
    m_connectionStateValue = ui->connectionStateValue;
    m_connectionAddressValue = ui->connectionAddressValue;
    m_connectionAddressValue->setText(actMap.addr);

    const auto installGauge = [](QWidget *host, QWidget *gauge) {
        auto *hostLayout = new QVBoxLayout(host);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->setSpacing(0);
        hostLayout->addWidget(gauge);
    };
    m_pitchGauge = new ROVAttitudeGauge(ROVAttitudeGauge::Pitch, ui->pitchPanelGaugeHost);
    m_rollGauge = new ROVAttitudeGauge(ROVAttitudeGauge::Roll, ui->rollPanelGaugeHost);
    m_thrusterGauge = new ROVThrusterGauge(ui->thrusterPanelGaugeHost);
    m_depthGauge = new ROVDepthGauge(ui->depthGaugeHost);
    installGauge(ui->pitchPanelGaugeHost, m_pitchGauge);
    installGauge(ui->rollPanelGaugeHost, m_rollGauge);
    installGauge(ui->thrusterPanelGaugeHost, m_thrusterGauge);
    installGauge(ui->depthGaugeHost, m_depthGauge);

    m_emergencyButton = ui->emergencyStopButton;
    connect(m_emergencyButton, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            onEmergencyStop();
            return;
        }
        if (QMessageBox::question(this, QString::fromUtf8("解除急停"),
                                  QString::fromUtf8("确认设备安全并解除急停？"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes) {
            m_emergencyStop = false;
        } else {
            m_emergencyButton->setChecked(true);
        }
    });
    connect(ui->dashboardExitButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->dashboardSettingsButton, &QPushButton::clicked,
            this, &MainWindow::showSetupDlg);
    connect(ui->dashboardHelpButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, QString::fromUtf8("WZ-724 操作帮助"),
            QString::fromUtf8("鼠标拖动 3D 区域可旋转视角，滚轮缩放。\n"
                              "左右圆盘控制 ROV，四向盘控制前后云台。\n"
                              "红色急停会立即上锁并禁止再次解锁。"));
    });
    connect(ui->dashboardPwmButton, &QPushButton::clicked,
            this, &MainWindow::onPwmButtonClicked);

    auto *dashboardClockTimer = new QTimer(this);
    dashboardClockTimer->setInterval(1000);
    connect(dashboardClockTimer, &QTimer::timeout, this, [this]() {
        ui->dashboardClock->setText(
            QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")));
    });
    ui->dashboardClock->setText(
        QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")));
    dashboardClockTimer->start();

    ui->legacyControls->hide();
    m_frontRecordButton->raise();
    m_rearRecordButton->raise();
}

// 从配置的 ROV 地址提取主机名，拼出前摄像头的主码流地址。
QString MainWindow::frontCameraUrl() const
{
    QString host = actMap.addr.trimmed();
    DEBUG<<host;
    if (host.startsWith(QStringLiteral("rtsp://"), Qt::CaseInsensitive))
        host.remove(0, 7);
    const int pathStart = host.indexOf(QLatin1Char('/'));
    if (pathStart >= 0)
        host.truncate(pathStart);
    return host.isEmpty()
        ? QString()
        : QStringLiteral("rtsp://%1/live/main_stream_1").arg(host);
}

// 后摄像头使用同一主机的第二路主码流。
QString MainWindow::rearCameraUrl() const
{
    QString host = actMap.addr.trimmed();
    if (host.startsWith(QStringLiteral("rtsp://"), Qt::CaseInsensitive))
        host.remove(0, 7);
    const int pathStart = host.indexOf(QLatin1Char('/'));
    if (pathStart >= 0)
        host.truncate(pathStart);
    return host.isEmpty()
        ? QString()
        : QStringLiteral("rtsp://%1/live/main_stream_2").arg(host);
}

// 设置变更后清空旧画面和重试状态，再异步连接前后两路视频。
// 地址为空时只更新面板提示，不发起网络连接。
void MainWindow::restartCameraStreams()
{
    const QString frontUrl = frontCameraUrl();
    const QString rearUrl = rearCameraUrl();
    DEBUG<<frontUrl;
    if (frontUrl.isEmpty() || rearUrl.isEmpty()) {
        m_frontVideoStatus = QString::fromUtf8("请在设置中填写 ROV 地址");
        m_rearVideoStatus = m_frontVideoStatus;
        ui->widget_camera_0->update();
        ui->widget_camera_back_1->update();
        return;
    }

    if (m_frontCamRetryTimer)
        m_frontCamRetryTimer->stop();
    if (m_rearCamRetryTimer)
        m_rearCamRetryTimer->stop();
    {
        //在锁保护下，把前后两路视频帧清空。一般调用场景：**停止视频流、关闭摄像头 /rtsp、重置画面**。
        QMutexLocker locker(&m_frameMutex);
        m_currentFrame = QImage();
        m_currentFrameRear = QImage();
    }
    m_frontVideoStatus = QString::fromUtf8("正在连接前摄像头…");
    m_rearVideoStatus = QString::fromUtf8("正在连接后摄像头…");
    ui->widget_camera_0->show();
    ui->widget_camera_back_1->show();
    ui->widget_camera_0->update();
    ui->widget_camera_back_1->update();

    // startPlayAsync 自身会中断旧连接，因此无论此前是否已进入 Playing 状态，
    // 设置保存后都能按最新地址真正发起连接。
    if (m_rtspPlayer)
        m_rtspPlayer->startPlayAsync(frontUrl);
    if (m_rtspPlayerRear)
        m_rtspPlayerRear->startPlayAsync(rearUrl);
}

// 恢复统一的仪表盘样式，并刷新通过动态属性选择的按钮外观。
void MainWindow::applyProfessionalTheme()
{
    setWindowTitle(QString::fromUtf8("WZ-724 水下机器人控制系统"));
    ui->label_2->setText(QString::fromUtf8("WZ-724  水下机器人控制系统"));

    // 清除 Designer 中的旧局部样式，让统一主题接管。
    const QList<QWidget *> oldStyledWidgets = {
        ui->widget_top, ui->widget_cameraPanel, ui->widget_camera_0,
        ui->widget_camera_back_1, ui->widget_ctrlPanel,
        ui->comboBox_flightMode, ui->pushButton_switchToDive,
        ui->pushButton_switchToClean, ui->statusbar,
        ui->label_leftLight, ui->label_rightLight,
        ui->slider_frontLight, ui->slider_rearLight
    };
    for (QWidget *widget : oldStyledWidgets)
        widget->setStyleSheet(QString());

    ui->widget_top->setMinimumHeight(44);
    ui->horizontalLayout->setContentsMargins(8, 5, 8, 5);
    ui->horizontalLayout->setSpacing(7);
    ui->gridLayout_2->setContentsMargins(7, 6, 7, 6);
    ui->gridLayout_2->setSpacing(7);
    ui->gridLayout->setContentsMargins(7, 6, 7, 7);
    ui->gridLayout->setHorizontalSpacing(7);
    ui->gridLayout->setVerticalSpacing(6);

    const QList<QPushButton *> topTools = {
        ui->pushButton_fullscreen, ui->pushButton_setup, ui->pushButton_close
    };
    for (QPushButton *button : topTools)
        button->setProperty("role", QStringLiteral("topTool"));
    ui->pushButton_switchToDive->setProperty("role", QStringLiteral("modeDive"));
    ui->pushButton_switchToClean->setProperty("role", QStringLiteral("modeClean"));

    const QList<QPushButton *> gimbalButtons = {
        ui->btnLeft_lt, ui->btnRight_lt, ui->btnUp_lt, ui->btnDown_lt,
        ui->btnFrontGimbalReset, ui->btnLeft_3, ui->btnRight_3,
        ui->btnUp_3, ui->btnDown_3, ui->btnRearGimbalReset
    };
    for (QPushButton *button : gimbalButtons)
        button->setProperty("role", QStringLiteral("gimbal"));

    // 动态属性设置后重新应用样式，保证 Qt 5.14 立即识别属性选择器。
    QList<QWidget *> themedControls;
    for (QPushButton *button : topTools)
        themedControls.append(button);
    themedControls << ui->pushButton_switchToDive << ui->pushButton_switchToClean;
    for (QPushButton *button : gimbalButtons)
        themedControls.append(button);
    for (QWidget *widget : themedControls) {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }

}


// 退出时先停控制和重连定时器，再中断视频网络读取并回收后台服务。
// m_shutdownComplete 保证重复调用不会再次释放同一批资源。
void MainWindow::stopBackgroundServices()
{
    if (m_shutdownComplete)
        return;
    m_shutdownComplete = true;

    // 先停止所有重连和控制定时器，避免关闭视频时又触发自动重连。
    const QList<QTimer *> timers = {
        m_manualControlTimer, m_gimbalControlTimer,
        m_rearGimbalControlTimer, m_frontCamRetryTimer,
        m_rearCamRetryTimer, m_heartbeatCheckTimer,
        m_frontFrameCheckTimer, m_rearFrameCheckTimer
    };
    for (QTimer *shutdownTimer : timers) {
        if (shutdownTimer)
            shutdownTimer->stop();
    }

    if (m_udpHandler) {
        QObject::disconnect(m_udpHandler, nullptr, this, nullptr);
        m_udpHandler->disconnect();
    }
    if (m_udpSocket)
        m_udpSocket->close();
    if (m_camera)
        m_camera->stop();
    if (m_gamepad)
        m_gamepad->stopPolling();

    // 先同时通知两路 FFmpeg 中断网络读写，再分别回收线程。
    if (m_rtspPlayer)
        m_rtspPlayer->requestShutdown();
    if (m_rtspPlayerRear)
        m_rtspPlayerRear->requestShutdown();

    // 屏蔽退出期间的 stopped/error 回调，避免界面更新和重连。
    if (m_rtspPlayer) {
        QObject::disconnect(m_rtspPlayer, nullptr, this, nullptr);
        m_rtspPlayer->shutdown();
    }
    if (m_rtspPlayerRear) {
        QObject::disconnect(m_rtspPlayerRear, nullptr, this, nullptr);
        m_rtspPlayerRear->shutdown();
    }

    closeAttitudeLog();
    if (m_mavlinkManager)
        m_mavlinkManager->closeCommandLog();
        m_mavlinkManager->closeinitAllLog();
}

MainWindow::~MainWindow(){
    stopBackgroundServices();
    // 预览窗口是带父对象的顶层窗口，主界面退出时主动关闭并回收。
    delete m_frontPreviewWindow;
    delete m_rearPreviewWindow;
    delete ui;
}

void MainWindow::changeEvent(QEvent *event){
    if(event->type() == QEvent::WindowStateChange){
        if(this->windowState() == Qt::WindowMaximized){
            this->showFullScreen();
            ui->widget_top->show();
            ui->statusbar->show();
        }
    }
    QMainWindow::changeEvent(event);
}

// 用户确认退出后先隐藏窗口并请求网络读取中断；
// 耗时的线程回收留给析构阶段执行。
void MainWindow::closeEvent(QCloseEvent *event){
    if (m_closeAccepted || m_shutdownComplete) {
        event->accept();
        return;
    }

    if(QMessageBox::question(this, "提示", "确定要退出控制器？",
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No)
            == QMessageBox::Yes){

        m_closeAccepted = true;
        // 窗口立即消失；耗时的线程回收随后在析构阶段完成，避免出现“未响应”。
        const QList<QTimer *> quickStopTimers = {
            m_manualControlTimer, m_gimbalControlTimer,
            m_rearGimbalControlTimer, m_frontCamRetryTimer,
            m_rearCamRetryTimer, m_heartbeatCheckTimer,
            m_frontFrameCheckTimer, m_rearFrameCheckTimer
        };
        for (QTimer *shutdownTimer : quickStopTimers) {
            if (shutdownTimer)
                shutdownTimer->stop();
        }
        if (m_rtspPlayer)
            m_rtspPlayer->requestShutdown();
        if (m_rtspPlayerRear)
            m_rtspPlayerRear->requestShutdown();
        hide();
        event->accept();
    }else {
        event->ignore();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event){
    QMainWindow::resizeEvent(event);

    //int _height = 720 * this->size().width() / 2560;
    //ui->widget_cameraPanel->setMinimumHeight(_height);

    if(m_camera){
        m_vf->resize(ui->widget_camera_0->size());
    }

    int _width = this->width();
    labCell_0->setMaximumWidth(60);
    labCell_1->setMinimumWidth(_width / 2 - 36);

    //if(!bInited){
    //        bInited=true;
    //        ui->widgetPFD->hide();
    //        ui->widgetPFD->show();
    //    }

}

void MainWindow::keyPressEvent(QKeyEvent *event){
    int keycode = event->key();
    if(keycode == Qt::Key_Escape){
        if(this->isFullScreen()){
            this->showNormal();
            ui->widget_top->show();
        }
    }

    // 云台控制 - 方向键
    if(keycode == Qt::Key_Left && !event->isAutoRepeat()){
        m_panLeftPressed = true;
        m_gimbalControlTimer->start();
        qDebug() << "云台向左旋转";
    }
    if(keycode == Qt::Key_Right && !event->isAutoRepeat()){
        m_panRightPressed = true;
        m_gimbalControlTimer->start();
        qDebug() << "云台向右旋转";
    }
    if(keycode == Qt::Key_Up && !event->isAutoRepeat()){
        m_tiltUpPressed = true;
        m_gimbalControlTimer->start();
        qDebug() << "云台向上旋转";
    }
    if(keycode == Qt::Key_Down && !event->isAutoRepeat()){
        m_tiltDownPressed = true;
        m_gimbalControlTimer->start();
        qDebug() << "云台向下旋转";
    }

    // 云台回中 - 空格键
    if(keycode == Qt::Key_Space && !event->isAutoRepeat()){
        centerFrontGimbal();
    }

    // 云台预设位置 - 数字键
    if(keycode == Qt::Key_0 && !event->isAutoRepeat()){
        // setFrontGimbalPreset(1);  // 预设位置1
        m_mavlinkManager->sendArmCommand(false);
    }

    if(keycode == Qt::Key_1 && !event->isAutoRepeat()){
        m_armingInProgress = true;
        m_manualX = 0;
        m_manualY = 0;
        m_manualZ = 500;
        m_manualR = 0;
        m_manualS = 0;
        m_manualT = 0;
        m_mavlinkManager->sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 19);
        if (!m_manualControlTimer->isActive()) {
            m_manualControlTimer->start();
        }
        QTimer::singleShot(500, this, [this]()
        {
            m_mavlinkManager->sendRcChannelsOverride(1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500);
            QTimer::singleShot(200, this, [this]()
            {
                m_mavlinkManager->sendRcChannelsOverride(1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500);
                m_mavlinkManager->sendArmCommand(true);
                QTimer::singleShot(1000, this, [this]()
                {
                    m_armingInProgress = false;
                });
            });
        });
    }


    //qDebug() << "key pressed :" << keycode;
    if(keycode == actMap.frontCamera){
        //on_pushButton_triggercamera_0_clicked();
    }
    if(keycode == actMap.frontLed0){
        if (m_mavlinkManager) {
            if (m_frontLightOn) {
                m_mavlinkManager->setFrontLightPercent(0);
                m_frontLightOn = false;
            } else {
                m_mavlinkManager->setFrontLightPercent(100);
                m_frontLightOn = true;
            }
        }
    }

    // 旧版这里只修改本地仪表造成“虚拟姿态”。现在保持按键映射不变，
    // 但统一转成MAVLink MANUAL_CONTROL，仪表只显示Pixhawk回传姿态。
    if(keycode == actMap.padLB_left && !event->isAutoRepeat()){ m_keyYawLeft = true; updateKeyboardManualControl(); }
    if(keycode == actMap.padLB_right && !event->isAutoRepeat()){ m_keyYawRight = true; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_up && !event->isAutoRepeat()){ m_keyPitchUp = true; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_down && !event->isAutoRepeat()){ m_keyPitchDown = true; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_left && !event->isAutoRepeat()){ m_keyRollLeft = true; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_right && !event->isAutoRepeat()){ m_keyRollRight = true; updateKeyboardManualControl(); }

    // 姿态旋转快捷键
    if(keycode == Qt::Key_F5 && !event->isAutoRepeat()){
        onRotationToCleanClicked();
    }
    if(keycode == Qt::Key_F6 && !event->isAutoRepeat()){
        onRotationToDiveClicked();
    }
    if(keycode == Qt::Key_F7 && !event->isAutoRepeat()){
        onRotationAbortClicked();
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event){
    int keycode = event->key();
    //qDebug() << "key released :" << keycode;
    // 云台控制释放
    if(keycode == Qt::Key_Left && !event->isAutoRepeat()){
        m_panLeftPressed = false;
        qDebug() << "云台停止向左";
    }
    if(keycode == Qt::Key_Right && !event->isAutoRepeat()){
        m_panRightPressed = false;
        qDebug() << "云台停止向右";
    }
    if(keycode == Qt::Key_Up && !event->isAutoRepeat()){
        m_tiltUpPressed = false;
        qDebug() << "云台停止向上";
    }
    if(keycode == Qt::Key_Down && !event->isAutoRepeat()){
        m_tiltDownPressed = false;
        qDebug() << "云台停止向下";
    }

    // 检查是否所有云台控制键都已释放
    //        if(!m_panLeftPressed && !m_panRightPressed && !m_tiltUpPressed && !m_tiltDownPressed){
    //            m_gimbalControlTimer->stop();
    //            // 恢复状态栏显示
    //            labCell_2->setText("云台已停止");
    //        }
    if(keycode == actMap.padLB_left && !event->isAutoRepeat()){ m_keyYawLeft = false; updateKeyboardManualControl(); }
    if(keycode == actMap.padLB_right && !event->isAutoRepeat()){ m_keyYawRight = false; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_up && !event->isAutoRepeat()){ m_keyPitchUp = false; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_down && !event->isAutoRepeat()){ m_keyPitchDown = false; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_left && !event->isAutoRepeat()){ m_keyRollLeft = false; updateKeyboardManualControl(); }
    if(keycode == actMap.padRB_right && !event->isAutoRepeat()){ m_keyRollRight = false; updateKeyboardManualControl(); }

    QMainWindow::keyReleaseEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == ui->widget_camera_0 || obj == ui->widget_camera_back_1) &&
        event->type() == QEvent::Resize) {
        updateCameraExpandButtonPositions();
    }
    if (obj == ui->widget_camera_0 && event->type() == QEvent::Paint) {
        paintVideoWidget(static_cast<QPaintEvent*>(event));
        return true;
    }
    if (obj == ui->widget_camera_back_1 && event->type() == QEvent::Paint) {
        paintRearVideoWidget(static_cast<QPaintEvent*>(event));
        return true;
    }
    if (obj == m_frontPreviewCanvas && event->type() == QEvent::Paint) {
        paintCameraWidget(m_frontPreviewCanvas, false);
        return true;
    }
    if (obj == m_rearPreviewCanvas && event->type() == QEvent::Paint) {
        paintCameraWidget(m_rearPreviewCanvas, true);
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

// 将 Designer 中的放大按钮始终固定在两路视频画面的右下角。
void MainWindow::updateCameraExpandButtonPositions()
{
    const auto placeButton = [](QWidget *camera, QPushButton *button) {
        if (!camera || !button)
            return;
        const QSize buttonSize(88, 30);
        button->setGeometry(qMax(8, camera->width() - buttonSize.width() - 12),
                            qMax(8, camera->height() - buttonSize.height() - 12),
                            buttonSize.width(), buttonSize.height());
        button->raise();
    };
    placeButton(ui->widget_camera_0, m_frontExpandButton);
    placeButton(ui->widget_camera_back_1, m_rearExpandButton);
}

// 打开约 960×540 的独立预览窗口。窗口保留系统标题栏，可以拖动、缩放和关闭；
// 关闭只销毁预览控件，原摄像头面板和 RTSP 连接保持不变。
void MainWindow::showCameraPreview(bool rearCamera)
{
    QWidget *&previewWindow = rearCamera ? m_rearPreviewWindow : m_frontPreviewWindow;
    QWidget *&previewCanvas = rearCamera ? m_rearPreviewCanvas : m_frontPreviewCanvas;
    if (previewWindow) {
        previewWindow->showNormal();
        previewWindow->raise();
        previewWindow->activateWindow();
        return;
    }

    previewWindow = new QWidget(this, Qt::Window);
    previewWindow->setAttribute(Qt::WA_DeleteOnClose);
    previewWindow->setWindowTitle(rearCamera
        ? QString::fromUtf8("后摄像头 - 放大预览")
        : QString::fromUtf8("前摄像头 - 放大预览"));
    previewWindow->setStyleSheet(QStringLiteral("background:#03090e;"));

    auto *layout = new QVBoxLayout(previewWindow);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    previewCanvas = new QWidget(previewWindow);
    previewCanvas->setMinimumSize(480, 270);
    previewCanvas->installEventFilter(this);
    layout->addWidget(previewCanvas);

    QScreen *screen = QApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QApplication::primaryScreen();
    const QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
    const QSize previewSize(qMin(960, qMax(480, available.width() - 80)),
                            qMin(540, qMax(270, available.height() - 80)));
    previewWindow->resize(previewSize);
    previewWindow->move(available.center() - QPoint(previewSize.width() / 2,
                                                    previewSize.height() / 2));

    QWidget **windowSlot = rearCamera ? &m_rearPreviewWindow : &m_frontPreviewWindow;
    QWidget **canvasSlot = rearCamera ? &m_rearPreviewCanvas : &m_frontPreviewCanvas;
    connect(previewWindow, &QObject::destroyed, this, [windowSlot, canvasSlot]() {
        *windowSlot = nullptr;
        *canvasSlot = nullptr;
    });
    previewWindow->show();
}

// 弹出窗口复用主界面的最新视频帧，并按窗口尺寸重新绘制，避免放大小画面截图。
void MainWindow::paintCameraWidget(QWidget *target, bool rearCamera)
{
    if (!target)
        return;

    QPainter painter(target);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    QMutexLocker locker(&m_frameMutex);

    const QImage &frame = rearCamera ? m_currentFrameRear : m_currentFrame;
    const QString &status = rearCamera ? m_rearVideoStatus : m_frontVideoStatus;
    const double fps = rearCamera ? m_rearVideoFps : m_frontVideoFps;
    const QString cameraName = rearCamera ? QString::fromUtf8("后摄像头")
                                          : QString::fromUtf8("前摄像头");
    const QRect viewport = target->rect();
    const bool live = !frame.isNull();
    painter.fillRect(viewport, QColor(3, 9, 14));

    if (live) {
        const qreal angleRad = qDegreesToRadians(qreal(m_videoLevelCompensationDeg));
        const qreal cosAngle = qAbs(qCos(angleRad));
        const qreal sinAngle = qAbs(qSin(angleRad));
        const qreal requiredWidth = cosAngle * target->width() + sinAngle * target->height();
        const qreal requiredHeight = sinAngle * target->width() + cosAngle * target->height();
        const qreal scale = qMax(requiredWidth / frame.width(), requiredHeight / frame.height());
        const QSizeF scaledSize(frame.width() * scale, frame.height() * scale);
        painter.save();
        painter.translate(viewport.center());
        painter.rotate(m_videoLevelCompensationDeg);
        painter.drawImage(QRectF(-scaledSize.width() / 2.0, -scaledSize.height() / 2.0,
                                 scaledSize.width(), scaledSize.height()), frame);
        painter.restore();
    } else {
        painter.setPen(QColor(137, 164, 178));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 16));
        painter.drawText(viewport.adjusted(30, 60, -30, -60),
                         Qt::AlignCenter | Qt::TextWordWrap, status);
    }

    const QString timestamp = cameraName + QStringLiteral("  ") +
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    QFont overlayFont(QStringLiteral("Consolas"), 12, QFont::Bold);
    painter.setFont(overlayFont);
    const QFontMetrics metrics(overlayFont);
    const QRect textRect = metrics.boundingRect(timestamp).adjusted(-10, -5, 10, 5);
    const QRect overlayRect(target->width() - textRect.width() - 12,
                            12, textRect.width(), textRect.height());
    painter.setBrush(QColor(5, 16, 24, 205));
    painter.setPen(QPen(QColor(42, 78, 96), 1));
    painter.drawRoundedRect(overlayRect, 5, 5);
    painter.setPen(QColor(205, 229, 239));
    painter.drawText(overlayRect, Qt::AlignCenter, timestamp);

    const QString liveText = live
        ? QString::fromUtf8("● LIVE  %1 FPS").arg(fps, 0, 'f', 1)
        : QString::fromUtf8("○ OFFLINE");
    QFont liveFont(QStringLiteral("Consolas"), 11, QFont::Bold);
    painter.setFont(liveFont);
    const QFontMetrics liveMetrics(liveFont);
    const QSize badgeSize(liveMetrics.horizontalAdvance(liveText) + 20,
                          liveMetrics.height() + 12);
    const QRect liveRect(12, viewport.height() - badgeSize.height() - 12,
                         badgeSize.width(), badgeSize.height());
    painter.setBrush(QColor(5, 16, 24, 215));
    painter.setPen(QPen(live ? QColor(46, 151, 119) : QColor(63, 87, 100), 1));
    painter.drawRoundedRect(liveRect, 6, 6);
    painter.setPen(live ? QColor(124, 228, 188) : QColor(139, 158, 168));
    painter.drawText(liveRect, Qt::AlignCenter, liveText);
}

void MainWindow::paintVideoWidget(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(ui->widget_camera_0);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    QMutexLocker locker(&m_frameMutex);
    const QRect viewport = ui->widget_camera_0->rect();
    const bool live = !m_currentFrame.isNull();
    painter.fillRect(viewport, QColor(3, 9, 14));

    if (live) {
        const QSize widgetSize = ui->widget_camera_0->size();
        const qreal angleRad = qDegreesToRadians(qreal(m_videoLevelCompensationDeg));
        const qreal cosAngle = qAbs(qCos(angleRad));
        const qreal sinAngle = qAbs(qSin(angleRad));
        // 根据旋转角度扩大画面，使其始终覆盖固定的视频矩形区域。
        const qreal requiredWidth = cosAngle * widgetSize.width() + sinAngle * widgetSize.height();
        const qreal requiredHeight = sinAngle * widgetSize.width() + cosAngle * widgetSize.height();
        const qreal scale = qMax(requiredWidth / m_currentFrame.width(),
                                  requiredHeight / m_currentFrame.height());
        const QSizeF scaledSize(m_currentFrame.width() * scale,
                                m_currentFrame.height() * scale);

        painter.save();
        painter.translate(viewport.center());
        // 摄像头随 ROV 横滚时，场景在原始视频中会反向偏转；
        // 按 ROV 的相对横滚角旋转显示帧，将场景恢复到初始角度。
        painter.rotate(m_videoLevelCompensationDeg);
        painter.drawImage(QRectF(-scaledSize.width() / 2.0,
                                 -scaledSize.height() / 2.0,
                                 scaledSize.width(), scaledSize.height()),
                          m_currentFrame);
        painter.restore();
    } else {
        painter.setPen(QColor(137, 164, 178));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 13));
        painter.drawText(viewport.adjusted(24, 52, -24, -48),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         m_frontVideoStatus);

        const QString timestamp = QString::fromUtf8("前摄像头  ") +
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QFont overlayFont(QStringLiteral("Consolas"), 12, QFont::Bold);
        painter.setFont(overlayFont);
        const QFontMetrics metrics(overlayFont);
        const QRect textRect = metrics.boundingRect(timestamp).adjusted(-10, -5, 10, 5);
        QRect overlayRect(ui->widget_camera_0->width() - textRect.width() - 10,
                          10, textRect.width(), textRect.height());
        painter.setBrush(QColor(5, 16, 24, 205));
        painter.setPen(QPen(QColor(42, 78, 96), 1));
        painter.drawRoundedRect(overlayRect, 5, 5);
        painter.setPen(QColor(205, 229, 239));
        painter.drawText(overlayRect, Qt::AlignCenter, timestamp);
    }

    // 视频已恢复为窗口坐标系后再绘制时间框，避免随画面旋转。
    if (live) {
        const QString timestamp = QString::fromUtf8("前摄像头  ") +
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QFont overlayFont(QStringLiteral("Consolas"), 12, QFont::Bold);
        painter.setFont(overlayFont);
        const QFontMetrics metrics(overlayFont);
        const QRect textRect = metrics.boundingRect(timestamp).adjusted(-10, -5, 10, 5);
        const QRect overlayRect(ui->widget_camera_0->width() - textRect.width() - 10,
                                10, textRect.width(), textRect.height());
        painter.setBrush(QColor(5, 16, 24, 205));
        painter.setPen(QPen(QColor(42, 78, 96), 1));
        painter.drawRoundedRect(overlayRect, 5, 5);
        painter.setPen(QColor(205, 229, 239));
        painter.drawText(overlayRect, Qt::AlignCenter, timestamp);
    }

    const QString liveText = live
        ? QString::fromUtf8("● LIVE  %1 FPS").arg(m_frontVideoFps, 0, 'f', 1)
        : QString::fromUtf8("○ OFFLINE");
    QFont liveFont(QStringLiteral("Consolas"), 10, QFont::Bold);
    painter.setFont(liveFont);
    const QFontMetrics liveMetrics(liveFont);
    const QSize badgeSize(liveMetrics.horizontalAdvance(liveText) + 18,
                          liveMetrics.height() + 10);
    QRect liveRect(10, viewport.height() - badgeSize.height() - 10,
                   badgeSize.width(), badgeSize.height());
    painter.setBrush(QColor(5, 16, 24, 215));
    painter.setPen(QPen(live ? QColor(46, 151, 119) : QColor(63, 87, 100), 1));
    painter.drawRoundedRect(liveRect, 6, 6);
    painter.setPen(live ? QColor(124, 228, 188) : QColor(139, 158, 168));
    painter.drawText(liveRect, Qt::AlignCenter, liveText);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(41, 68, 84), 1));
    painter.drawRoundedRect(viewport.adjusted(0, 0, -1, -1), 8, 8);
}

void MainWindow::paintRearVideoWidget(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(ui->widget_camera_back_1);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    QMutexLocker locker(&m_frameMutex);
    const QRect viewport = ui->widget_camera_back_1->rect();
    const bool live = !m_currentFrameRear.isNull();
    painter.fillRect(viewport, QColor(3, 9, 14));

    if (live) {
        const QSize widgetSize = ui->widget_camera_back_1->size();
        const qreal angleRad = qDegreesToRadians(qreal(m_videoLevelCompensationDeg));
        const qreal cosAngle = qAbs(qCos(angleRad));
        const qreal sinAngle = qAbs(qSin(angleRad));
        const qreal requiredWidth = cosAngle * widgetSize.width() + sinAngle * widgetSize.height();
        const qreal requiredHeight = sinAngle * widgetSize.width() + cosAngle * widgetSize.height();
        const qreal scale = qMax(requiredWidth / m_currentFrameRear.width(),
                                  requiredHeight / m_currentFrameRear.height());
        const QSizeF scaledSize(m_currentFrameRear.width() * scale,
                                m_currentFrameRear.height() * scale);

        painter.save();
        painter.translate(viewport.center());
        // 后摄像头使用与前摄像头一致的横滚稳定方向。
        painter.rotate(m_videoLevelCompensationDeg);
        painter.drawImage(QRectF(-scaledSize.width() / 2.0,
                                 -scaledSize.height() / 2.0,
                                 scaledSize.width(), scaledSize.height()),
                          m_currentFrameRear);
        painter.restore();
    } else {
        painter.setPen(QColor(137, 164, 178));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 13));
        painter.drawText(viewport.adjusted(24, 52, -24, -48),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         m_rearVideoStatus);

        const QString timestamp = QString::fromUtf8("后摄像头  ") +
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QFont overlayFont(QStringLiteral("Consolas"), 12, QFont::Bold);
        painter.setFont(overlayFont);
        const QFontMetrics metrics(overlayFont);
        const QRect textRect = metrics.boundingRect(timestamp).adjusted(-10, -5, 10, 5);
        QRect overlayRect(ui->widget_camera_back_1->width() - textRect.width() - 10,
                          10, textRect.width(), textRect.height());
        painter.setBrush(QColor(5, 16, 24, 205));
        painter.setPen(QPen(QColor(42, 78, 96), 1));
        painter.drawRoundedRect(overlayRect, 5, 5);
        painter.setPen(QColor(205, 229, 239));
        painter.drawText(overlayRect, Qt::AlignCenter, timestamp);
    }

    // 视频已恢复为窗口坐标系后再绘制时间框，避免随画面旋转。
    if (live) {
        const QString timestamp = QString::fromUtf8("后摄像头  ") +
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QFont overlayFont(QStringLiteral("Consolas"), 12, QFont::Bold);
        painter.setFont(overlayFont);
        const QFontMetrics metrics(overlayFont);
        const QRect textRect = metrics.boundingRect(timestamp).adjusted(-10, -5, 10, 5);
        const QRect overlayRect(ui->widget_camera_back_1->width() - textRect.width() - 10,
                                10, textRect.width(), textRect.height());
        painter.setBrush(QColor(5, 16, 24, 205));
        painter.setPen(QPen(QColor(42, 78, 96), 1));
        painter.drawRoundedRect(overlayRect, 5, 5);
        painter.setPen(QColor(205, 229, 239));
        painter.drawText(overlayRect, Qt::AlignCenter, timestamp);
    }

    const QString liveText = live
        ? QString::fromUtf8("● LIVE  %1 FPS").arg(m_rearVideoFps, 0, 'f', 1)
        : QString::fromUtf8("○ OFFLINE");
    QFont liveFont(QStringLiteral("Consolas"), 10, QFont::Bold);
    painter.setFont(liveFont);
    const QFontMetrics liveMetrics(liveFont);
    const QSize badgeSize(liveMetrics.horizontalAdvance(liveText) + 18,
                          liveMetrics.height() + 10);
    QRect liveRect(10, viewport.height() - badgeSize.height() - 10,
                   badgeSize.width(), badgeSize.height());
    painter.setBrush(QColor(5, 16, 24, 215));
    painter.setPen(QPen(live ? QColor(46, 151, 119) : QColor(63, 87, 100), 1));
    painter.drawRoundedRect(liveRect, 6, 6);
    painter.setPen(live ? QColor(124, 228, 188) : QColor(139, 158, 168));
    painter.drawText(liveRect, Qt::AlignCenter, liveText);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(41, 68, 84), 1));
    painter.drawRoundedRect(viewport.adjusted(0, 0, -1, -1), 8, 8);
}

void MainWindow::updateRecordingButton(QPushButton *button, bool recording)
{
    if (!button)
        return;

    button->setText(recording ? QString::fromUtf8("■ 停止录像")
                              : QString::fromUtf8("● 开始录像"));
    button->setStyleSheet(recording
        ? QStringLiteral(
              "QPushButton{color:#fff0f2;background:rgba(183,67,79,235);"
              "border:1px solid #ee8490;border-radius:7px;"
              "font-weight:600;padding:5px 12px;}"
              "QPushButton:hover{background:#ca5260;}"
              "QPushButton:pressed{background:#8f3540;}")
        : QStringLiteral(
              "QPushButton{color:#d7e8ef;background:rgba(8,22,31,220);"
              "border:1px solid #456477;border-radius:7px;"
              "font-weight:600;padding:5px 12px;}"
              "QPushButton:hover{color:white;background:#172d3a;"
              "border-color:#dc6570;}"
              "QPushButton:pressed{background:#47232a;}"));
}

void MainWindow::toggleVideoRecording(FFmpegRtspPlayer *player,
                                      const QString &cameraTag,
                                      const QString &cameraName)
{
    if (!player)
        return;
    // 正在录制 → 停止录制
    if (player->isRecording())
    {
        player->stopRecording();
        return;
    }

    if (!player->isPlaying() || player->getVideoSize().isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("无法录像"),
                                 cameraName + QString::fromUtf8("视频流尚未连接"));
        return;
    }

    QString moviesDirectory =
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (moviesDirectory.isEmpty())
        moviesDirectory = QDir::homePath();
    moviesDirectory = QDir(moviesDirectory).filePath(QStringLiteral("wz-724"));
    QDir().mkpath(moviesDirectory);

    const QString defaultName = QStringLiteral("wz-724_%1_%2.avi")
        .arg(cameraTag,
             QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QString filePath = QFileDialog::getSaveFileName(
        this,
        cameraName + QString::fromUtf8("录像保存位置"),
        QDir(moviesDirectory).filePath(defaultName),
        QString::fromUtf8("AVI 视频 (*.avi)"));
    if (filePath.isEmpty())
        return;
    if (!filePath.endsWith(QStringLiteral(".avi"), Qt::CaseInsensitive))
        filePath += QStringLiteral(".avi");

    player->startRecording(filePath);
}

void MainWindow::updateKeyboardManualControl()
{
    constexpr int16_t keyboardDeflection = 500;

    const int yawDirection = (m_keyYawRight ? 1 : 0) - (m_keyYawLeft ? 1 : 0);
    const int pitchDirection = (m_keyPitchUp ? 1 : 0) - (m_keyPitchDown ? 1 : 0);
    const int rollDirection = (m_keyRollRight ? 1 : 0) - (m_keyRollLeft ? 1 : 0);

    m_manualR = static_cast<int16_t>(yawDirection * keyboardDeflection);
    m_manualX = static_cast<int16_t>(pitchDirection * keyboardDeflection);
    m_manualY = static_cast<int16_t>(rollDirection * keyboardDeflection);

    if (m_mavlinkConnected && m_manualControlTimer && !m_manualControlTimer->isActive()) {
        m_manualControlTimer->start();
    }
    DEBUG<<"1111111111111:updateKeyboardManualControl";
    onManualControlTimer();
}

void MainWindow::requestMavlinkStreams()
{
    if (!m_mavlinkManager || !m_mavlinkConnected) {
        return;
    }

    // 所有仪表数据均从Pixhawk MAVLink遥测获取。
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_ATTITUDE, 100000);          // 10Hz
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_SERVO_OUTPUT_RAW, 200000); // 5Hz
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_VFR_HUD, 200000);          // 5Hz
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 200000);
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_SCALED_PRESSURE, 200000);
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_SCALED_PRESSURE2, 200000);
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_SCALED_PRESSURE3, 200000);
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_RC_CHANNELS, 500000);       // 2Hz
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_GPS_RAW_INT, 1000000);     // 1Hz
    m_mavlinkManager->sendSetMessageInterval(MAVLINK_MSG_ID_SYS_STATUS, 1000000);      // 1Hz
    m_mavlinkManager->sendParamRequestRead("FRAME_CONFIG");
}

void MainWindow::on_pushButton_fullscreen_clicked(){
    if(this->isFullScreen()){
        this->showNormal();
        ui->widget_top->show();
    }else{
        this->showFullScreen();
        ui->widget_top->hide();
        ui->statusbar->show();
    }
}

void MainWindow::on_pushButton_close_clicked(){
    this->close();
}

#if 0
void MainWindow::on_pushButton_triggercamera_0_clicked(){
    if(m_camera){
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
        ui->widget_camera_0->close();

        ui->pushButton_triggercamera_0->setText("打开前摄像头");
        ui->pushButton_triggercamera_0->setIcon(QIcon(":/btn/pic/turnoncamera.png"));
    }else{
        //获取可用摄像头设备并输出在控制台
        QList<QCameraInfo> infos = QCameraInfo::availableCameras();
        qDebug() << infos.value(0).deviceName() << ":" <<infos.value(0).description();
        QString camera = infos.value(0).deviceName();
        qDebug() << camera;

        //显示摄像头
        m_camera = new QCamera(camera.toUtf8(), this);

        m_camera->setViewfinder(m_vf);
        m_vf->show();
        m_camera->start();

        ui->widget_camera_0->show();
        m_vf->resize(ui->widget_camera_0->size());

        ui->pushButton_triggercamera_0->setText("关闭前摄像头");
        ui->pushButton_triggercamera_0->setIcon(QIcon(":/btn/pic/turnoffcamera.png"));
    }

    labCell_0->setText("");
}
#endif


void MainWindow::showSetupDlg(){
    ActiveMap _map = actMap;
    DialogSetup * dlgSetup = new DialogSetup(this, &_map, m_mavlinkManager);
    int ret = dlgSetup->exec();
    delete dlgSetup;
    if(ret == QDialog::Accepted){
        actMap = _map;
        QSettings *iniFile = new QSettings("setup", QSettings::IniFormat);
        iniFile->beginGroup("system");
        iniFile->setValue("rovaddr", actMap.addr);
        iniFile->setValue("rovport", actMap.port);
        iniFile->endGroup();

        iniFile->beginGroup("map");

        iniFile->setValue("fc", actMap.frontCamera);
        iniFile->setValue("fcs", actMap.sFrontCamera);
        iniFile->setValue("fl0", actMap.frontLed0);
        iniFile->setValue("fl0s", actMap.sFrontLed0);
        iniFile->setValue("fl1", actMap.frontLed1);
        iniFile->setValue("fl1s", actMap.sFrontLed1);

        iniFile->setValue("bc", actMap.backCamera);
        iniFile->setValue("bcs", actMap.sBackCamera);
        iniFile->setValue("bl0", actMap.backLed0);
        iniFile->setValue("bl0s", actMap.sBackLed0);
        iniFile->setValue("bl1", actMap.backLed1);
        iniFile->setValue("bl1s", actMap.sBackLed1);

        iniFile->setValue("lt_up", actMap.padLT_up);
        iniFile->setValue("lt_ups", actMap.sPadLT_up);
        iniFile->setValue("lt_down", actMap.padLT_down);
        iniFile->setValue("lt_downs", actMap.sPadLT_down);
        iniFile->setValue("lt_left", actMap.padLT_left);
        iniFile->setValue("lt_lefts", actMap.sPadLT_left);
        iniFile->setValue("lt_right", actMap.padLT_right);
        iniFile->setValue("lt_rights", actMap.sPadLT_right);

        iniFile->setValue("lb_up", actMap.padLB_up);
        iniFile->setValue("lb_ups", actMap.sPadLB_up);
        iniFile->setValue("lb_down", actMap.padLB_down);
        iniFile->setValue("lb_downs", actMap.sPadLB_down);
        iniFile->setValue("lb_left", actMap.padLB_left);
        iniFile->setValue("lb_lefts", actMap.sPadLB_left);
        iniFile->setValue("lb_right", actMap.padRB_right);
        iniFile->setValue("lb_rights", actMap.sPadLB_right);

        iniFile->setValue("rt_up", actMap.padRT_up);
        iniFile->setValue("rt_ups", actMap.sPadRT_up);
        iniFile->setValue("rt_down", actMap.padRT_down);
        iniFile->setValue("rt_downs", actMap.sPadRT_down);
        iniFile->setValue("rt_left", actMap.padRT_left);
        iniFile->setValue("rt_lefts", actMap.sPadRT_left);
        iniFile->setValue("rt_right", actMap.padRT_right);
        iniFile->setValue("rt_rights", actMap.sPadRT_right);

        iniFile->setValue("rb_up", actMap.padRB_up);
        iniFile->setValue("rb_ups", actMap.sPadRB_up);
        iniFile->setValue("rb_down", actMap.padRB_down);
        iniFile->setValue("rb_downs", actMap.sPadRB_down);
        iniFile->setValue("rb_left", actMap.padRB_left);
        iniFile->setValue("rb_lefts", actMap.sPadRB_left);
        iniFile->setValue("rb_right", actMap.padRB_right);
        iniFile->setValue("rb_rights", actMap.sPadRB_right);

        iniFile->endGroup();
        iniFile->sync();
        delete iniFile;

        // 地址或端口变更后重新连接 UDP 和 RTSP。即使此前连接失败、播放器
        // 尚未进入 Playing 状态，也必须使用新地址重新发起双路连接。
        m_udpHandler->setRemoteAddress(actMap.addr, actMap.port);
        if (m_connectionAddressValue)
            m_connectionAddressValue->setText(actMap.addr);
        restartCameraStreams();

    }
}

void MainWindow::initUdpSocket(){
    m_groupAddress.setAddress("127.0.0.1");
    m_udpSocket = new QUdpSocket;
    if(m_udpSocket->bind(15002)){
        //QMessageBox::information(this, "提示", "成功绑定端口 15002");

        connect(m_udpSocket, &QUdpSocket::readyRead, this, [=]{
            ReadPendingDataframs();
        });
    } else {
        //QMessageBox::critical(this, "错误", "绑定端口 15002 失败，请检查端口是否被占用");
    }
}

void MainWindow::ReadPendingDataframs(){
    if(m_udpSocket)
        // 检查是否有待处理的数据报
        while (m_udpSocket->hasPendingDatagrams()) {
            QByteArray array;
            // 根据待处理数据报的大小调整数组的大小
            array.resize(m_udpSocket->pendingDatagramSize());
            // 读取数据报
            m_udpSocket->readDatagram(array.data(), array.size());

            // labCell_1 已显示系统状态

            // 将接收到的字节数组转换为 QString 类型，以便显示
            QString buf = QString::fromUtf8(array);
            // 将接收到的消息显示在 UI 的接收框中
            //ui->recvEdit->appendPlainText(buf);
            qDebug() << "received : " << buf << endl;
        }
}

void MainWindow::on_pushButton_setup_clicked(){
    showSetupDlg();
}

// ========== UDP 槽函数 ==========
void MainWindow::onUdpConnectClicked()
{
    m_udpHandler->setRemoteAddress(actMap.addr, actMap.port);
    m_udpHandler->bind(5555);
}

void MainWindow::onUdpDisconnectClicked()
{
    // logMessage("UDP: 正在断开连接...", "blue");

    m_udpHandler->disconnect();
}

void MainWindow::onUdpSendClicked()
{

    QByteArray sendData(5,'\0');

    m_udpHandler->sendData(sendData);
}

void MainWindow::onUdpDataReceived(const QByteArray &data, const QHostAddress &senderAddr, quint16 senderPort)
{
    Q_UNUSED(senderAddr)
    Q_UNUSED(senderPort)
    // 解析 MAVLink 数据
    if (m_mavlinkManager) {
        m_mavlinkManager->parseMavlinkData(data);
    }
}

void MainWindow::onUdpConnected()
{
    // UDP套接字绑定成功只代表传输层就绪，不能代表Pixhawk已连接。
    m_mavlinkConnected = false;
    setStatusBadge(labCell_2, QString::fromUtf8("未连接"), "warning");
    if (m_connectionStateValue) {
        m_connectionStateValue->setText(QString::fromUtf8("等待MAVLink心跳  ●"));
        m_connectionStateValue->setProperty("state", QStringLiteral("warning"));
        m_connectionStateValue->style()->unpolish(m_connectionStateValue);
        m_connectionStateValue->style()->polish(m_connectionStateValue);
    }
    m_heartbeatCheckTimer->start();
    if (m_virtualJoystick)
        m_virtualJoystick->setInitialConnectComplete(false);
}

void MainWindow::onUdpDisconnected()
{
    m_mavlinkConnected = false;
    setStatusBadge(labCell_2, QString::fromUtf8("已断开"), "danger");
    if (m_connectionStateValue) {
        m_connectionStateValue->setText(QString::fromUtf8("离线  ●"));
        m_connectionStateValue->setProperty("state", QStringLiteral("danger"));
        m_connectionStateValue->style()->unpolish(m_connectionStateValue);
        m_connectionStateValue->style()->polish(m_connectionStateValue);
    }
    m_heartbeatCheckTimer->stop();
    m_manualControlTimer->stop();
    if (m_virtualJoystick) {
        m_virtualJoystick->setInitialConnectComplete(false);
        m_virtualJoystick->reCenterAll();
    }
    if (m_mavlinkManager)
        m_mavlinkManager->resetVehicleConnection();
    m_countMutex.lock();
    m_udpDataCount=0;
    m_countMutex.unlock();
}

void MainWindow::onUdpError(const QString &error)
{
    //logMessage(QString("UDP 错误: %1").arg(error), "red");
}


#if 1

// ========== RTSP 槽函数 ==========
void MainWindow::onRtspPlayClicked()
{
    const QString url = frontCameraUrl();
    DEBUG<<url;
    if (url.isEmpty()) {
        return;
    }
    {
        QMutexLocker locker(&m_frameMutex);
        m_currentFrame = QImage();
    }
    ui->widget_camera_0->update();
    m_rtspPlayer->startPlayAsync(url);
}

void MainWindow::onRtspStopClicked()
{
    m_rtspPlayer->stopPlay();
}

void MainWindow::onRtspCaptureClicked()
{
    QImage frame ;//m_rtspPlayer->captureFrame();
    if (frame.isNull()) {

        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "保存截图",
                                                    QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png",
                                                    "PNG 图片 (*.png)");

    if (!fileName.isEmpty()) {
        if (frame.save(fileName)) {
            //logMessage(QString("RTSP: 截图已保存到 %1").arg(fileName), "green");
        } else {
            // logMessage("RTSP: 截图保存失败", "red");
        }
    }
}

void MainWindow::onRtspStarted()
{
    m_frontCamRetryTimer->stop();
    m_frontFrameCheckTimer->start();
    m_frontVideoStatus = QString::fromUtf8("前摄像头已连接，等待首帧…");
    ui->widget_camera_0->show();
    ui->widget_camera_0->update();
    updateGimbalStatus();
}

void MainWindow::onRtspStopped()
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame = QImage();
    m_frontVideoFps = 0.0;
    m_frontVideoStatus = QString::fromUtf8("前摄像头已断开，正在重连…");
    ui->widget_camera_0->update();
    m_frontFrameCheckTimer->stop();
    m_frontCamRetryTimer->start();
}

void MainWindow::onNewFrameReceived(const QImage &image)
{
    if (!m_rtspPlayer || !m_rtspPlayer->isPlaying()) {
        return;
    }
    if (!image.isNull()) {
        {
            QMutexLocker locker(&m_frameMutex);
            m_currentFrame = image.copy();
        }
        m_frontVideoStatus = QString::fromUtf8("前摄像头视频正常");
        ui->widget_camera_0->show();
        ui->widget_camera_0->update();
        if (m_frontPreviewCanvas)
            m_frontPreviewCanvas->update();
        m_frontFrameCheckTimer->start();
    }
}

void MainWindow::onVideoSizeChanged(const QSize &size)
{
    Q_UNUSED(size);
}

void MainWindow::onRtspError(const QString &error)
{
    {
        QMutexLocker locker(&m_frameMutex);
        m_currentFrame = QImage();
        m_frontVideoFps = 0.0;
    }
    m_frontVideoStatus = QString::fromUtf8("前摄像头连接失败：%1").arg(error);
    ui->widget_camera_0->update();
    if (m_frontPreviewCanvas)
        m_frontPreviewCanvas->update();
    m_frontFrameCheckTimer->stop();
    m_frontCamRetryTimer->start();
}

void MainWindow::onRtspFpsUpdated(double fps)
{
    m_frontVideoFps = fps;
    ui->widget_camera_0->update();
}

void MainWindow::onRearRtspStarted()
{
    m_rearCamRetryTimer->stop();
    m_rearFrameCheckTimer->start();
    m_rearVideoStatus = QString::fromUtf8("后摄像头已连接，等待首帧…");
    ui->widget_camera_back_1->show();
    ui->widget_camera_back_1->update();
    updateGimbalStatus();
}

void MainWindow::onRearRtspStopped()
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrameRear = QImage();
    m_rearVideoFps = 0.0;
    m_rearVideoStatus = QString::fromUtf8("后摄像头已断开，正在重连…");
    ui->widget_camera_back_1->update();
    m_rearFrameCheckTimer->stop();
    m_rearCamRetryTimer->start();
}

void MainWindow::onRearRtspError(const QString &error)
{
    {
        QMutexLocker locker(&m_frameMutex);
        m_currentFrameRear = QImage();
        m_rearVideoFps = 0.0;
    }
    m_rearVideoStatus = QString::fromUtf8("后摄像头连接失败：%1").arg(error);
    ui->widget_camera_back_1->update();
    m_rearFrameCheckTimer->stop();
    m_rearCamRetryTimer->start();
}

void MainWindow::onRearNewFrameReceived(const QImage &image)
{
    if (!m_rtspPlayerRear || !m_rtspPlayerRear->isPlaying()) {
        return;
    }
    if (!image.isNull()) {
        {
            QMutexLocker locker(&m_frameMutex);
            m_currentFrameRear = image.copy();
        }
        m_rearVideoStatus = QString::fromUtf8("后摄像头视频正常");
        ui->widget_camera_back_1->show();
        ui->widget_camera_back_1->update();
        if (m_rearPreviewCanvas)
            m_rearPreviewCanvas->update();
        m_rearFrameCheckTimer->start();
    }
}

void MainWindow::onRearRtspFpsUpdated(double fps)
{
    m_rearVideoFps = fps;
    ui->widget_camera_back_1->update();
    if (m_rearPreviewCanvas)
        m_rearPreviewCanvas->update();
}

void MainWindow::onRearVideoSizeChanged(const QSize &size)
{
    Q_UNUSED(size);
}

void MainWindow::onFrontCamRetryTimeout()
{
    if (m_rtspPlayer && !m_rtspPlayer->isPlaying()) {
        qDebug() << "前摄像头重连...";
        m_frontVideoStatus = QString::fromUtf8("正在重新连接前摄像头…");
        ui->widget_camera_0->update();
        const QString url = frontCameraUrl();
        if (!url.isEmpty())
            m_rtspPlayer->startPlayAsync(url);
    }
}

void MainWindow::onRearCamRetryTimeout()
{
    if (m_rtspPlayerRear && !m_rtspPlayerRear->isPlaying()) {
        qDebug() << "后摄像头重连...";
        m_rearVideoStatus = QString::fromUtf8("正在重新连接后摄像头…");
        ui->widget_camera_back_1->update();
        const QString url = rearCameraUrl();
        if (!url.isEmpty())
            m_rtspPlayerRear->startPlayAsync(url);
    }
}

void MainWindow::onHeartbeatCheckTimeout()
{
    if (!m_mavlinkManager) return;
    qint64 lastHb = m_mavlinkManager->getVehicleState().lastHeartbeatMs;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastHb > 3000) {
        if (m_mavlinkConnected) {
            m_mavlinkConnected = false;
            setStatusBadge(labCell_2, QString::fromUtf8("已断开"), "danger");
            if (m_connectionStateValue) {
                m_connectionStateValue->setText(QString::fromUtf8("离线  ●"));
                m_connectionStateValue->setProperty("state", QStringLiteral("danger"));
                m_connectionStateValue->style()->unpolish(m_connectionStateValue);
                m_connectionStateValue->style()->polish(m_connectionStateValue);
            }
            m_manualControlTimer->stop();
            if (m_virtualJoystick) {
                m_virtualJoystick->setInitialConnectComplete(false);
                m_virtualJoystick->reCenterAll();
            }
            if (m_mavlinkManager)
                m_mavlinkManager->resetVehicleConnection();
            qDebug() << "Pixhawk MAVLink心跳超时";
        }
        if (m_udpHandler) {
            m_udpHandler->disconnect();
            m_udpHandler->bind(5555);
            m_udpHandler->setRemoteAddress(actMap.addr, actMap.port);
        }
    }
}

void MainWindow::onFrontFrameCheckTimeout()
{
    if (m_rtspPlayer && m_rtspPlayer->isPlaying()) {
        qDebug() << "前摄像头5s无新帧，判定视频中断，重连...";
        m_rtspPlayer->stopPlay();
    }
}

void MainWindow::onRearFrameCheckTimeout()
{
    if (m_rtspPlayerRear && m_rtspPlayerRear->isPlaying()) {
        qDebug() << "后摄像头5s无新帧，判定视频中断，重连...";
        m_rtspPlayerRear->stopPlay();
    }
}

void MainWindow::onHeartbeatReceived()
{
    if (!m_mavlinkConnected) {
        m_mavlinkConnected = true;
        setStatusBadge(labCell_2, QString::fromUtf8("MAVLink已连接"), "ok");
        if (m_connectionStateValue) {
            m_connectionStateValue->setText(QString::fromUtf8("Pixhawk在线  ●"));
            m_connectionStateValue->setProperty("state", QStringLiteral("ok"));
            m_connectionStateValue->style()->unpolish(m_connectionStateValue);
            m_connectionStateValue->style()->polish(m_connectionStateValue);
        }

        if (m_virtualJoystick)
            m_virtualJoystick->setInitialConnectComplete(true);

        requestMavlinkStreams();

        // 保留原有首次连接行为，但只在确认真实飞控心跳后执行。
        setFrontGimbalPreset(3);
        ui->slider_frontLight->setValue(50);
        ui->slider_rearLight->setValue(50);
    }
    m_heartbeatCheckTimer->start();

    if (!m_manualControlTimer->isActive()) {
        m_manualControlTimer->start();
    }
    static int modeSetCount = 0;
    if (modeSetCount < 5 && m_mavlinkManager) {
        m_mavlinkManager->sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 19);
        modeSetCount++;
    }

    if (m_mavlinkManager) {
        uint32_t customMode = m_mavlinkManager->getCustomMode();
        int index = -1;
        switch (customMode) {
        case 19: index = 0; break;  // MANUAL
        case 0:  index = 1; break;  // STABILIZE
        case 1:  index = 2; break;  // ACRO
        case 2:  index = 3; break;  // ALT_HOLD
        case 16: index = 4; break;  // POSHOLD
        case 4:  index = 5; break;  // GUIDED
        }
        if (index >= 0 && ui->comboBox_flightMode->currentIndex() != index) {
            ui->comboBox_flightMode->blockSignals(true);
            ui->comboBox_flightMode->setCurrentIndex(index);
            ui->comboBox_flightMode->blockSignals(false);
        }

        bool armed = m_mavlinkManager->isArmed();
        if (armed) {
            setStatusBadge(labCell_0, QString::fromUtf8("已解锁"), "ok");
        } else {
            setStatusBadge(labCell_0, QString::fromUtf8("未解锁"), "danger");
        }
    }
}

void MainWindow::onAttitudeUpdated(float roll, float pitch, float yaw,
                                   float rollspeed, float pitchspeed, float yawspeed,
                                   float yawCumulative)
{
    // 首条姿态作为画面基准，后续横滚用于旋转显示画面。
    if (!m_videoLevelReferenceValid) {
        m_videoLevelReferenceRollDeg = roll;
        m_videoLevelReferenceValid = true;
    }
    m_videoLevelCompensationDeg = roll - m_videoLevelReferenceRollDeg;
    if (m_videoLevelCompensationDeg > 180.0f)
        m_videoLevelCompensationDeg -= 360.0f;
    else if (m_videoLevelCompensationDeg < -180.0f)
        m_videoLevelCompensationDeg += 360.0f;

    labCell_rpy->setText(QString("R:%1° P:%2° Y:%3°").arg(roll, 0, 'f', 1).arg(pitch, 0, 'f', 1).arg(yaw, 0, 'f', 1));

    ui->widgetPFD->setRoll(roll);
    ui->widgetPFD->setPitch(pitch);
    ui->widgetPFD->setHeading(yaw);
    ui->widgetPFD->update();
    if (m_attitude3DWidget)
        m_attitude3DWidget->setAttitude(roll, pitch, yaw);
    if (m_pitchGauge)
       {
           m_pitchGauge->setValue(pitch);
           m_pitchGauge->setValue_roll(roll);
           m_pitchGauge->setValue_pitch(pitch);

       }
    if (m_rollGauge)
    {
        m_rollGauge->setValue(roll);
        m_rollGauge->setValue_yaw(yaw);
    }

//    if (m_headingGauge)
//        m_headingGauge->setValue(yaw);

    if (m_headingValueLabel) {
        float heading = yaw;
        while (heading < 0.0f)
            heading += 360.0f;
        while (heading >= 360.0f)
            heading -= 360.0f;
        m_headingValueLabel->setText(QStringLiteral("%1").arg(heading, 3, 'f', 0, QChar('0')));
    }

    // 姿态更新独立于视频帧到达，立即刷新画面以保证补偿跟随姿态。
    ui->widget_camera_0->update();
    ui->widget_camera_back_1->update();

    if (m_attitudeLogStream) {
        QMutexLocker locker(&m_logMutex);
        *m_attitudeLogStream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
                             << QString(",%1,%2,%3,%4,%5,%6,%7")
                                .arg(roll, 0, 'f', 2)
                                .arg(pitch, 0, 'f', 2)
                                .arg(yaw, 0, 'f', 2)
                                .arg(rollspeed, 0, 'f', 2)
                                .arg(pitchspeed, 0, 'f', 2)
                                .arg(yawspeed, 0, 'f', 2)
                                .arg(yawCumulative, 0, 'f', 2)
                             << "\n";
        m_attitudeLogStream->flush();
    }
}


void MainWindow::onGpsUpdated(uint8_t fixType, uint8_t satellites, double lat, double lon)
{
    QString gpsStatus;
    switch (fixType) {
    case 2: gpsStatus = "2D"; break;
    case 3: gpsStatus = "3D"; break;
    case 4: gpsStatus = "DGPS"; break;
    case 5: gpsStatus = "RTK"; break;
    default: gpsStatus = "无"; break;
    }

}

void MainWindow::onStatusTextReceived(const QString &text, uint8_t severity)
{
    QString prefix;
    QString colorStyle = "QLabel { color: %1; }";

    switch (severity) {

    case MAV_SEVERITY_EMERGENCY: prefix = "[紧急]"; break;
    case MAV_SEVERITY_ALERT:     prefix = "[警报]"; break;
    case MAV_SEVERITY_CRITICAL:  prefix = "[严重]"; break;
    case MAV_SEVERITY_ERROR:     prefix = "[错误]"; break;
    case MAV_SEVERITY_WARNING:   prefix = "[警告]"; break;
    case MAV_SEVERITY_NOTICE:    prefix = "[注意]"; break;
    case MAV_SEVERITY_INFO:      prefix = "[信息]"; break;
    case MAV_SEVERITY_DEBUG:     prefix = "[调试]"; break;

    }

    // labCell_1->setText(prefix + " " + text);
}

void MainWindow::onCommandAckReceived(uint16_t command, uint8_t result)
{
    QString resultStr;
    QString colorStyle;

    switch (result) {
    case MAV_RESULT_ACCEPTED:
        resultStr = "成功";
        colorStyle = "QLabel { color: white; font-weight: bold; }";
        break;
    case MAV_RESULT_TEMPORARILY_REJECTED:
        resultStr = "临时拒绝";
        colorStyle = "QLabel { color: orange; font-weight: bold; }";
        break;
    case MAV_RESULT_DENIED:
        resultStr = "拒绝";
        colorStyle = "QLabel { color: red; font-weight: bold; }";
        break;
    case MAV_RESULT_UNSUPPORTED:
        resultStr = "不支持";
        colorStyle = "QLabel { color: red; font-weight: bold; }";
        break;
    case MAV_RESULT_FAILED:
        resultStr = "失败";
        colorStyle = "QLabel { color: red; font-weight: bold; }";
        break;
    default:
        resultStr = QString("未知(%1)").arg(result);
        colorStyle = "QLabel { color: yellow; font-weight: bold; }";
        break;
    }

    // 获取命令名称
    QString commandName = getCommandName(command);

    // 显示命令执行状态
    QString statusText = QString("命令: %1 (0x%2) - 结果: %3")
            .arg(commandName)
            .arg(command, 0, 16)
            .arg(resultStr);

    // 记录日志
    qDebug() << "Command ACK -" << commandName << "(" << command << ") result:" << resultStr;

    // 如果是舵机或云台命令，可以在这里更新UI状态
    if (command == MAV_CMD_DO_SET_SERVO) {
        qDebug() << "舵机设置命令执行" << resultStr;
    } else if (command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        qDebug() << "消息间隔设置命令执行" << resultStr;
    }
}

void MainWindow::onFlightModeChanged(int index)
{
    if (!m_mavlinkManager) return;

    static const int modeMap[] = {
        19,   // 0: 手动 (MANUAL)
        0,    // 1: 姿态稳定 (STABILIZE)
        1,    // 2: 特技 (ACRO)
        2,    // 3: 定深 (ALT_HOLD)
        16,   // 4: 定点 (POSHOLD)
        4     // 5: 引导角度 (GUIDED)
    };

    if (index < 0 || index >= 6) return;

    uint32_t customMode = modeMap[index];
    m_mavlinkManager->sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, customMode);
    qDebug() << "飞行模式切换: index=" << index << "customMode=" << customMode;
}

void MainWindow::onSlideArmTriggered()
{
    //解锁
    if (!m_mavlinkManager) return;
    if (m_emergencyStop) {
        QMessageBox::warning(this, "急停", "紧急停止已激活，禁止解锁！\n请检查设备状态后重试。");
        return;
    }
    m_armingInProgress = true;
    m_manualX = 0;
    m_manualY = 0;
    m_manualZ = 500;
    m_manualR = 0;
    m_mavlinkManager->sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 19);
    if (!m_manualControlTimer->isActive()) {
        m_manualControlTimer->start();
    }
    QTimer::singleShot(500, this, [this]() 
    {
        m_mavlinkManager->sendRcChannelsOverride(1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500);
        QTimer::singleShot(200, this, [this]()
        {
            m_mavlinkManager->sendRcChannelsOverride(1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500);
            m_mavlinkManager->sendArmCommand(true);
            QTimer::singleShot(1000, this, [this]()
            {
                m_armingInProgress = false;
            });
        });
    });
}

void MainWindow::onSlideDisarmTriggered()
{
    if (!m_mavlinkManager) return;
    m_mavlinkManager->sendArmCommand(false);
}


// 更新姿态数据
void MainWindow::updatePfdAttitude(float roll, float pitch, float yaw)
{
    // 直接更新 PFD 组件
    ui->widgetPFD->setRoll(roll);
    ui->widgetPFD->setPitch(pitch);
    ui->widgetPFD->setHeading(yaw);

    // 更新 HSI（如果单独存在）
    // ui->widgetHSI->setHeading(yaw);

    ui->widgetPFD->update();
}


// 更新升降率（m/s → ft/min）
void MainWindow::updatePfdClimbRate(float climbRate_ms)
{
    // 转换为 ft/min (1 m/s = 196.85 ft/min)
    float climbRate_fpm = climbRate_ms * 196.85f;

    // 限制范围在 -2000 到 2000 ft/min 之间
    if (climbRate_fpm > 2000.0f) climbRate_fpm = 2000.0f;
    if (climbRate_fpm < -2000.0f) climbRate_fpm = -2000.0f;

    ui->widgetPFD->setClimbRate(climbRate_fpm);

    ui->widgetPFD->update();
}

// 批量更新所有 PFD 数据
void MainWindow::updatePfdFromMavlink()
{
    const VehicleState& state = m_mavlinkManager->getVehicleState();

    // 更新状态栏
    QString statusText = QString("模式: %1 | 状态: %2 | 电池: %3V")
            .arg(m_mavlinkManager->getModeString())
            .arg(m_mavlinkManager->getSystemStatusString())
            .arg(state.sysStatus.voltageBattery / 1000.0f, 0, 'f', 2);

    labCell_1->setText(statusText);

    // 优先使用压力传感器深度；未收到压力数据时回退到相对高度的绝对值。
    if (state.depth.lastUpdateMs != 0 || state.globalPos.lastUpdateMs != 0) {
        const float depthMeters = state.depth.lastUpdateMs != 0
            ? qMax(0.0f, state.depth.depth)
            : qAbs(state.globalPos.relativeAlt / 1000.0f);
        ui->widgetPFD->setAltitude(depthMeters);
        if (m_depthValueLabel)
            m_depthValueLabel->setText(QString::number(depthMeters, 'f', 2));
        if (m_depthGauge)
            m_depthGauge->setDepth(depthMeters);
           // m_headingGauge->setValue_Depth(depthMeters);
    }

    if (state.depth.lastUpdateMs != 0 && m_pressureValueLabel) {
        m_pressureValueLabel->setText(QString::number(state.depth.pressure, 'f', 1));
    }

    // 更新地速
    static uint32_t lastAirspeedUpdate = 0;
    if (state.vfrHud.lastUpdateMs != lastAirspeedUpdate) {
        lastAirspeedUpdate = state.vfrHud.lastUpdateMs;
        const float speedMetersPerSecond = qMax(0.0f, state.vfrHud.groundspeed);
        ui->widgetPFD->setAirspeed(speedMetersPerSecond);
        if (m_speedValueLabel)
        {
            m_speedValueLabel->setText(QString::number(speedMetersPerSecond, 'f', 2));
            //速度
//            if(m_headingGauge)
//            {
//                //m_headingGauge->setValue_Speed(speedMetersPerSecond);
//            }431694056
        }

        if (m_headingValueLabel) {
            int heading = state.vfrHud.heading % 360;
            if (heading < 0)
                heading += 360;
            m_headingValueLabel->setText(QStringLiteral("%1").arg(heading, 3, 10, QChar('0')));
        }
    }

    // 更新升降率
    static uint32_t lastClimbUpdate = 0;
    if (state.vfrHud.lastUpdateMs != lastClimbUpdate) {
        lastClimbUpdate = state.vfrHud.lastUpdateMs;
        float climbRate_fpm = state.vfrHud.climbRate * 196.85f;
        if (climbRate_fpm > 2000.0f) climbRate_fpm = 2000.0f;
        if (climbRate_fpm < -2000.0f) climbRate_fpm = -2000.0f;
        ui->widgetPFD->setClimbRate(climbRate_fpm);
    }

    ui->widgetPFD->update();
}


QString MainWindow::getCommandName(uint16_t command)
{
    switch (command) {
    // 基本命令
    case MAV_CMD_NAV_WAYPOINT:           return "航点";
    case MAV_CMD_NAV_LOITER_UNLIM:       return "悬停";
    case MAV_CMD_NAV_LOITER_TURNS:       return "盘旋";
    case MAV_CMD_NAV_LOITER_TIME:        return "定时盘旋";
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:   return "返航";
    case MAV_CMD_NAV_LAND:               return "降落";
    case MAV_CMD_NAV_TAKEOFF:            return "起飞";
    case MAV_CMD_NAV_LAND_LOCAL:         return "本地降落";
    case MAV_CMD_NAV_TAKEOFF_LOCAL:      return "本地起飞";

        // 控制命令
    case MAV_CMD_COMPONENT_ARM_DISARM:   return "解锁/上锁";
    case MAV_CMD_DO_SET_MODE:            return "设置模式";
    case MAV_CMD_DO_SET_SERVO:           return "设置舵机";
    case MAV_CMD_DO_REPEAT_SERVO:        return "重复舵机";
    case MAV_CMD_DO_SET_RELAY:           return "设置继电器";
    case MAV_CMD_DO_REPEAT_RELAY:        return "重复继电器";

        // 消息和参数
    case MAV_CMD_REQUEST_MESSAGE:        return "请求消息";
    case MAV_CMD_SET_MESSAGE_INTERVAL:   return "设置消息间隔";
    case MAV_CMD_REQUEST_PROTOCOL_VERSION: return "请求协议版本";
    case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES: return "请求飞控能力";

        // 相机和云台
    case MAV_CMD_DO_DIGICAM_CONFIGURE:   return "配置相机";
    case MAV_CMD_DO_DIGICAM_CONTROL:     return "控制相机";
    case MAV_CMD_DO_MOUNT_CONFIGURE:     return "配置云台";
    case MAV_CMD_DO_MOUNT_CONTROL:       return "控制云台";

        // 校准
    case MAV_CMD_PREFLIGHT_CALIBRATION:  return "预飞校准";
    case MAV_CMD_PREFLIGHT_SET_SENSOR_OFFSETS: return "设置传感器偏移";
    case MAV_CMD_PREFLIGHT_STORAGE:      return "存储参数";
    case MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN: return "重启/关机";

    default:
        return QString("CMD_%1").arg(command);
    }
}

void MainWindow::onManualControlTimer()
{

    if (m_mavlinkManager && m_mavlinkConnected && !m_emergencyStop) {
        if (m_mavlinkManager->isRotating() || m_mavlinkManager->isGuidedCleaning()) {
            return;
        }
        //DEBUG<<m_manualX<<"-"<<m_manualY<<"-"<<m_manualR<<"-"<<m_manualZ<<"-"<<m_manualButtons;
        int16_t x = (qAbs(m_manualX) < 20) ? 0 : m_manualX;
        int16_t y = (qAbs(m_manualY) < 20) ? 0 : m_manualY;
        int16_t r = (qAbs(m_manualR) < 20) ? 0 : m_manualR;
        m_mavlinkManager->sendManualControl(x, y, m_manualZ, r, m_manualButtons, 0, 0, false);
    }
}

void MainWindow::setManualControlButton(uint16_t mask, bool pressed)
{
    qDebug()<<"按压按键："<<mask<<pressed;
    if (pressed)
    {
        m_manualButtons = static_cast<uint16_t>(m_manualButtons | mask);
    }
    else
    {
        m_manualButtons = static_cast<uint16_t>(
        m_manualButtons & static_cast<uint16_t>(~mask));
    }

    // 立即发送一次，避免很短的按键动作被40 ms定时器漏掉；后续继续按25 Hz
    // 连同当前摇杆轴一起发送，与QGC的MANUAL_CONTROL行为一致。
    if (m_manualControlTimer && !m_manualControlTimer->isActive()) {
        m_manualControlTimer->start();
    }
    DEBUG<<"2222222222:setManualControlButton";
    onManualControlTimer();
}

void MainWindow::onFrontLightBrightnessChanged(int value)
{
    int brightness = 100-value;
    m_frontLightBrightness = brightness;
    m_frontLightOn = (brightness > 0);
    if (m_mavlinkManager) {
        m_mavlinkManager->setFrontLightPercent(static_cast<uint8_t>(brightness));
    }
}

void MainWindow::onRearLightBrightnessChanged(int value)
{
    int brightness =  100-value;
    m_rearLightBrightness = brightness;
    m_rearLightOn = (brightness > 0);
    if (m_mavlinkManager) {
        m_mavlinkManager->setRearLightPercent(static_cast<uint8_t>(brightness));
    }
}


void MainWindow::onGimbalControlTimer()
{
    if (!m_mavlinkManager) {
        return;
    }

    int16_t panDelta = 0;
    int16_t tiltDelta = 0;

    // 处理左右旋转 - 互斥
    if (m_panLeftPressed && !m_panRightPressed) {
        panDelta = -PAN_STEP;   // 只按左键：向左
    } else if (m_panRightPressed && !m_panLeftPressed) {
        panDelta = PAN_STEP;    // 只按右键：向右
    }
    // 同时按下左右键：panDelta = 0，不动

    // 处理上下旋转 - 互斥
    if (m_tiltUpPressed && !m_tiltDownPressed) {
        tiltDelta = TILT_STEP;   // 只按上键：向上
    } else if (m_tiltDownPressed && !m_tiltUpPressed) {
        tiltDelta = -TILT_STEP;  // 只按下键：向下
    }
    // 同时按下上下键：tiltDelta = 0，不动

    // 发送云台移动命令
    if (panDelta != 0 || tiltDelta != 0) {
        if (panDelta != 0) {
            m_mavlinkManager->moveFrontPan(panDelta);
        }
        if (tiltDelta != 0) {
            m_mavlinkManager->moveFrontTilt(tiltDelta);
        }

        updateGimbalStatus();
    }
}

// 云台回中
void MainWindow::centerFrontGimbal()
{
    if (m_mavlinkManager) {
        m_mavlinkManager->centerFrontCameraGimbal();
        qDebug() << "前云台已回中";
        updateGimbalStatus();
    }
}

void MainWindow::updateGimbalStatus()
{
}

void MainWindow::onRearGimbalControlTimer()
{
    if (!m_mavlinkManager) {
        return;
    }

    int16_t panDelta = 0;
    int16_t tiltDelta = 0;

    if (m_rearPanLeftPressed && !m_rearPanRightPressed) {
        panDelta = -PAN_STEP;
    } else if (m_rearPanRightPressed && !m_rearPanLeftPressed) {
        panDelta = PAN_STEP;
    }

    if (m_rearTiltUpPressed && !m_rearTiltDownPressed) {
        tiltDelta = TILT_STEP;
    } else if (m_rearTiltDownPressed && !m_rearTiltUpPressed) {
        tiltDelta = -TILT_STEP;
    }

    if (panDelta != 0 || tiltDelta != 0) {
        if (panDelta != 0) {
            m_mavlinkManager->moveRearPan(panDelta);
        }
        if (tiltDelta != 0) {
            m_mavlinkManager->moveRearTilt(tiltDelta);
        }
        updateGimbalStatus();
    }

    if (!m_rearPanLeftPressed && !m_rearPanRightPressed &&
        !m_rearTiltUpPressed && !m_rearTiltDownPressed) {
        m_rearGimbalControlTimer->stop();
    }
}

// 云台预设位置
void MainWindow::setFrontGimbalPreset(int preset)
{
    if (!m_mavlinkManager) return;

    switch (preset) {
    case 1:  // 预设1：左前上
        m_mavlinkManager->setFrontCameraGimbal(1200, 1800);
        qDebug() << "云台预设1：左前上";
        break;
    case 2:  // 预设2：右前下
        m_mavlinkManager->setFrontCameraGimbal(1800, 1200);
        qDebug() << "云台预设2：右前下";
        break;
    case 3:  // 预设3：正中
        m_mavlinkManager->centerFrontCameraGimbal();
        qDebug() << "云台预设3：正中";
        break;
    default:
        break;
    }

    updateGimbalStatus();
}


void MainWindow::onVirtualJoystickToggled(bool enabled)
{
    m_virtualJoystickEnabled = enabled;
    m_virtualJoystick->setVisible(enabled);

    if (enabled) {
        if (!m_manualControlTimer->isActive()) {
            m_manualControlTimer->start();
        }
    } else {
        m_manualX = 0;
        m_manualY = 0;
        m_manualZ = 500;
        m_manualR = 0;
    }
}

void MainWindow::onJoystickValueChanged(qreal roll, qreal pitch, qreal yaw, qreal thrust)
{
    if (m_armingInProgress) return;

    //DEBUG<<"roll："<<roll<<"pitch："<<pitch<<"yaw："<<yaw<<"油门："<<thrust;
    m_manualX = static_cast<int16_t>(pitch * 1000);
    m_manualY = static_cast<int16_t>(roll * 1000);
    m_manualZ = static_cast<int16_t>(thrust * 1000);
    m_manualR = static_cast<int16_t>(yaw * 1000);
}

#endif

void MainWindow::initGamepad()
{
    m_gamepad = new SDL2Gamepad(this);
    connect(m_gamepad, &SDL2Gamepad::connected, this, [this]() {
        DEBUG << "SDL2游戏手柄已连接:" << m_gamepad->name();
        m_gamepadConnected = true;
        if (m_virtualJoystick) m_virtualJoystick->setVisible(false);
    });
    connect(m_gamepad, &SDL2Gamepad::disconnected, this, [this]() {
        DEBUG<< "SDL2游戏手柄已断开";
        m_gamepadConnected = false;
        const bool customButtonWasPressed = (m_manualButtons & kCustomButtonMask) != 0;
        m_manualButtons = static_cast<uint16_t>(
            m_manualButtons & static_cast<uint16_t>(~kCustomButtonMask));
        if (customButtonWasPressed) {
             DEBUG<<"3333333333:initGamepad";
            onManualControlTimer();
        }
        m_rearPanLeftPressed = false;
        m_rearPanRightPressed = false;
        if (m_virtualJoystick && m_virtualJoystickEnabled) m_virtualJoystick->setVisible(true);
    });
    connectGamepadSignals();
    m_gamepad->startPolling();
}

void MainWindow::connectGamepadSignals()
{
    if (!m_gamepad) return;

    // 与QGC一致(Mode 3 非左手模式): 左杆=throttle(Y)+yaw(X), 右杆=pitch(Y)+roll(X)
    // MANUAL_CONTROL: x=pitch, y=roll, z=throttle, r=yaw
    connect(m_gamepad, &SDL2Gamepad::axisLeftXChanged, this, [this](double value) {
        if (m_armingInProgress) return;
        m_leftStickX = value;
        m_manualR = static_cast<int16_t>(m_leftStickX * 1000);  // yaw
        DEBUG<<"m_leftStickX"<<m_leftStickX<<"-"<<m_manualR;
        if (!m_manualControlTimer->isActive())
        {
            m_manualControlTimer->start();
        }
    });
    connect(m_gamepad, &SDL2Gamepad::axisLeftYChanged, this, [this](double value) {
        if (m_armingInProgress) return;
        m_leftStickY = value;
        m_manualZ = static_cast<int16_t>((m_leftStickY + 1.0) * 500);  // throttle
         DEBUG<<"m_leftStickY"<<m_leftStickY<<"-"<<m_manualZ;
        if (!m_manualControlTimer->isActive())
        {
           m_manualControlTimer->start();
        }
    });

    connect(m_gamepad, &SDL2Gamepad::axisRightXChanged, this, [this](double value) {
        if (m_armingInProgress) return;
        m_rightStickX = value;
        m_manualY = static_cast<int16_t>(m_rightStickX * 1000);  // roll
        DEBUG<<"m_rightStickX"<<m_rightStickX<<"-"<<m_manualY;
        if (!m_manualControlTimer->isActive())
        {
            m_manualControlTimer->start();
        }
    });
    connect(m_gamepad, &SDL2Gamepad::axisRightYChanged, this, [this](double value) {
        if (m_armingInProgress) return;
        m_rightStickY = value;
        m_manualX = static_cast<int16_t>(m_rightStickY * 1000);  // pitch
         DEBUG<<"m_rightStickY"<<m_rightStickY<<"-"<<m_manualX;
        if (!m_manualControlTimer->isActive())
        {
            m_manualControlTimer->start();
        }
    });

    // D-Pad上(B12) - 前云台上, D-Pad下(B13) - 前云台下
    connect(m_gamepad, &SDL2Gamepad::buttonDpadUpChanged, this, [this](bool pressed) {
        m_tiltUpPressed = pressed;
        if (pressed) m_gimbalControlTimer->start();
    });
    connect(m_gamepad, &SDL2Gamepad::buttonDpadDownChanged, this, [this](bool pressed) {
        m_tiltDownPressed = pressed;
        if (pressed) m_gimbalControlTimer->start();
    });

    // D-Pad左(B14) - 前云台左, D-Pad右(B15) - 前云台右
    connect(m_gamepad, &SDL2Gamepad::buttonDpadLeftChanged, this, [this](bool pressed) {
        m_panLeftPressed = pressed;
        if (pressed) m_gimbalControlTimer->start();
    });
    connect(m_gamepad, &SDL2Gamepad::buttonDpadRightChanged, this, [this](bool pressed) {
        m_panRightPressed = pressed;
        if (pressed) m_gimbalControlTimer->start();
    });

    // Share(B8) - 解锁
    connect(m_gamepad, &SDL2Gamepad::buttonBackChanged, this, [this](bool pressed) {
        if (pressed && m_mavlinkManager) {
            if (m_emergencyStop) {
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setIcon(QMessageBox::Warning);
                msgBox->setWindowTitle("急停");
                msgBox->setText("紧急停止已激活，禁止解锁！\n松开急停按钮后可恢复。");
                msgBox->setStandardButtons(QMessageBox::Ok);
                msgBox->setDefaultButton(QMessageBox::Ok);
                msgBox->setWindowFlags(msgBox->windowFlags() | Qt::WindowStaysOnTopHint);
                msgBox->setAttribute(Qt::WA_DeleteOnClose);
                msgBox->show();
                QTimer::singleShot(3000, msgBox, &QMessageBox::accept);
                return;
            }
            if (!m_mavlinkManager->isArmed()) {
                m_mavlinkManager->sendArmCommand(true);
                qDebug() << "手柄: 解锁(Share)";
            }
        }
    });

    // Y(△) - 后云台上, A(✕) - 后云台下
    connect(m_gamepad, &SDL2Gamepad::buttonYChanged, this, [this](bool pressed) {
        m_rearTiltUpPressed = pressed;
        if (pressed) m_rearGimbalControlTimer->start();
    });
    connect(m_gamepad, &SDL2Gamepad::buttonAChanged, this, [this](bool pressed) {
        m_rearTiltDownPressed = pressed;
        if (pressed) m_rearGimbalControlTimer->start();
    });

    // B(○/BTN1) -> buttons bit 1(value 2) -> ArduSub Custom1 -> 角度+5°
    connect(m_gamepad, &SDL2Gamepad::buttonBChanged, this, [this](bool pressed) {
        setManualControlButton(kCustom1ButtonMask, pressed);
        DEBUG << "手柄: Custom1 角度+5°" << (pressed ? "按下" : "松开")
                 << "buttons=" << m_manualButtons;
    });

    // X(□/BTN2) -> buttons bit 2(value 4) -> ArduSub Custom2 -> 角度-5°
    connect(m_gamepad, &SDL2Gamepad::buttonXChanged, this, [this](bool pressed) {
        setManualControlButton(kCustom2ButtonMask, pressed);
        DEBUG<< "手柄: Custom2 角度-5°" << (pressed ? "按下" : "松开")
                 << "buttons=" << m_manualButtons;
    });

    // 后云台左右移到未占用的L1/R1，避免与Custom1/Custom2冲突
    connect(m_gamepad, &SDL2Gamepad::buttonL1Changed, this, [this](bool pressed) {
        m_rearPanLeftPressed = pressed;
        if (pressed) m_rearGimbalControlTimer->start();
    });
    connect(m_gamepad, &SDL2Gamepad::buttonR1Changed, this, [this](bool pressed) {
        m_rearPanRightPressed = pressed;
        if (pressed) m_rearGimbalControlTimer->start();
    });

    // Options(B9) - 上锁
    connect(m_gamepad, &SDL2Gamepad::buttonStartChanged, this, [this](bool pressed) {
        if (pressed && m_mavlinkManager) {
            if (m_mavlinkManager->isArmed()) {
                m_mavlinkManager->sendArmCommand(false);
                qDebug() << "手柄: 上锁(Options)";
            }
        }
    });

    // L2 - 前灯亮度(电位器0~1映射到0~100%)
    connect(m_gamepad, &SDL2Gamepad::buttonL2Changed, this, [this](double value) {
        qDebug() << "L2:" << value;
        if (m_mavlinkManager) {
            int val = qBound(0, static_cast<int>(value * 100), 100);
            ui->slider_frontLight->setValue(val);
        }
    });

    // R2 - 后灯亮度(电位器0~1映射到0~100%)
    connect(m_gamepad, &SDL2Gamepad::buttonR2Changed, this, [this](double value) {
        qDebug() << "R2:" << value;
        if (m_mavlinkManager) {
            int val = qBound(0, static_cast<int>(value * 100), 100);
            ui->slider_rearLight->setValue(val);
        }
    });

    // Guide(B16) - 急停(按下触发，松开解除)
    connect(m_gamepad, &SDL2Gamepad::buttonGuideChanged, this, [this](bool pressed) {
        DEBUG<<pressed;
        if (pressed)
        {
            onEmergencyStop();
        }
        else
        {
            m_emergencyStop = false;
            qDebug() << "急停解除";
        }
    });
}

// ========== 姿态旋转控制 ==========

void MainWindow::onRotationToCleanClicked()
{
    if (!m_mavlinkManager) return;
    if (m_mavlinkManager->isGuidedCleaning()) {
        m_mavlinkManager->abortGuidedClean();
        return;
    }
    if (!m_mavlinkManager->isArmed()) {
        QMessageBox::warning(this, "清洗失败", "请先解锁再启动一键清洗！");
        return;
    }
    m_mavlinkManager->startGuidedClean(80.0f, 0.0f, 60);
    QMessageBox::information(this, "一键清洗",
        "GUIDED清洗流程已启动：\n"
        "1. 切换GUIDED模式\n"
        "2. 下潜到位\n"
        "3. 抬头至80°清洗姿态\n"
        "4. 保持清洗60秒\n"
        "5. 恢复姿态，切回MANUAL\n\n"
        "再次点击可中止");
}

void MainWindow::onRotationToDiveClicked()
{
    if (!m_mavlinkManager) return;
    if (m_mavlinkManager->isGuidedCleaning()) {
        m_mavlinkManager->abortGuidedClean();
        return;
    }
    if (m_mavlinkManager->isArmed()) {
        QMessageBox::warning(this, "切换失败", "请先上锁再切换到下水模式！");
        return;
    }
    m_mavlinkManager->sendCleanerCommand(8);
    QMessageBox::information(this, "一键下水", "已发送下水指令");
}

void MainWindow::onRotationAbortClicked()
{
    if (!m_mavlinkManager) return;
    m_mavlinkManager->abortRotation();
}

void MainWindow::onRotationStateChanged(RotationState state)
{
    switch (state) {
    case ROTATION_TO_CLEAN:
    case ROTATION_TO_DIVE:
        m_manualX = 0;
        m_manualY = 0;
        m_manualR = 0;
        m_manualS = 0;
        m_manualT = 0;
        break;
    case ROTATION_IDLE:
    case ROTATION_COMPLETED:
    case ROTATION_ABORTED:
        break;
    }
}

void MainWindow::onMachineOrientationChanged(MachineOrientation orientation)
{
    Q_UNUSED(orientation);
}


void MainWindow::onFrameConfigChanged(int frameConfig)
{
    Q_UNUSED(frameConfig);
}

void MainWindow::onSwitchToCleanMode()
{
    if (!m_mavlinkManager) return;
    m_mavlinkManager->setNetCleanerMode(1);
}

void MainWindow::onSwitchToDiveMode()
{
    if (!m_mavlinkManager) return;
    m_mavlinkManager->setNetCleanerMode(0);
}


void MainWindow::onPwmButtonClicked()
{
    if (m_pwmDock->isHidden()) {
        m_pwmDock->show();
        m_pwmDock->raise();
        if (m_mavlinkManager) {
            for (int i = 1; i <= 6; ++i) {
                QString dirParam = QString("MOT_%1_DIRECTION").arg(i);
                m_mavlinkManager->sendParamRequestRead(dirParam.toUtf8().constData());
                QString funcParam = QString("SERVO%1_FUNCTION").arg(i);
                m_mavlinkManager->sendParamRequestRead(funcParam.toUtf8().constData());
            }
            static const char *pidParams[] = {
                "ATC_RAT_RLL_P", "ATC_RAT_RLL_I", "ATC_RAT_RLL_D",
                "ATC_RAT_PIT_P", "ATC_RAT_PIT_I", "ATC_RAT_PIT_D",
                "ATC_RAT_YAW_P", "ATC_RAT_YAW_I", "ATC_RAT_YAW_D",
                "ATC_SLEW_YAW"
            };
            for (const char *param : pidParams) {
                m_mavlinkManager->sendParamRequestRead(param);
            }
        }
    } else {
        m_pwmDock->hide();
    }
}

void MainWindow::onServoUpdated()
{
    if (!m_mavlinkManager) return;
    const VehicleState &state = m_mavlinkManager->getVehicleState();
    if (m_thrusterGauge)
        m_thrusterGauge->setPwmValues(state.servo.raw, state.servo.count);
    if (m_pwmDock->isHidden()) return;
    m_pwmDisplayWidget->updatePwmValues(state.servo.raw, state.servo.count);
    m_pwmDisplayWidget->setArmed(m_mavlinkManager->isArmed());
}

void MainWindow::onPwmReverseChanged(int channel, bool reversed)
{
    if (!m_mavlinkManager) return;
    if (m_mavlinkManager->isArmed()) {
        QMessageBox::warning(this, tr("操作失败"), tr("请在未解锁状态下设置电机正反转！"));
        return;
    }
    QString paramName = QString("MOT_%1_DIRECTION").arg(channel);
    m_mavlinkManager->sendParamSet(paramName.toUtf8().constData(),
                                   reversed ? -1.0f : 1.0f,
                                   MAV_PARAM_TYPE_REAL32);
    qDebug() << "设置" << paramName << "=" << (reversed ? -1 : 1);
}

void MainWindow::onPwmEnabledChanged(int channel, bool enabled)
{
    if (!m_mavlinkManager) return;
    if (m_mavlinkManager->isArmed()) {
        QMessageBox::warning(this, tr("操作失败"), tr("请在未解锁状态下设置电机启用/禁用！"));
        return;
    }
    QString paramName = QString("SERVO%1_FUNCTION").arg(channel);
    float funcVal = enabled ? (32.0f + channel) : 0.0f;
    m_mavlinkManager->sendParamSet(paramName.toUtf8().constData(),
                                   funcVal,
                                   MAV_PARAM_TYPE_REAL32);
    qDebug() << "设置" << paramName << "=" << funcVal << (enabled ? "(启用)" : "(禁用)");
}

void MainWindow::onParamValueReceived(const QString &paramId, float value)
{
    m_pwmDisplayWidget->onParamValueReceived(paramId, value);
}

void MainWindow::applyCrossGate(double x, double y, double &outX, double &outY)
{
    outX = x;
    outY = y;
}

void MainWindow::onEmergencyStop()
{
    if (!m_mavlinkManager) return;

    m_emergencyStop = true;
    if (m_emergencyButton)
        m_emergencyButton->setChecked(true);

    if (m_mavlinkManager->isArmed()) {
        m_mavlinkManager->sendArmCommand(false);
    }

    m_manualX = 0;
    m_manualY = 0;
    m_manualZ = 500;
    m_manualR = 0;

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Critical);
    msgBox->setWindowTitle("急停");
    msgBox->setText("紧急停止已激活！\n解锁功能已禁用，请检查设备状态。");
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setDefaultButton(QMessageBox::Ok);
    msgBox->setWindowFlags(msgBox->windowFlags() | Qt::WindowStaysOnTopHint);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->show();

    QTimer::singleShot(3000, msgBox, &QMessageBox::accept);
}

void MainWindow::initAttitudeLog()
{
    QString logDir = QCoreApplication::applicationDirPath() + "/log";
    QDir().mkpath(logDir);
    QString logFile = logDir + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "_attitude.csv";
    m_attitudeLogFile = new QFile(logFile, this);
    if (m_attitudeLogFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_attitudeLogStream = new QTextStream(m_attitudeLogFile);
        *m_attitudeLogStream << "timestamp,roll_deg,pitch_deg,yaw_deg,rollspeed_dps,pitchspeed_dps,yawspeed_dps,yaw_cumulative_deg\n";
        m_attitudeLogStream->flush();
        qDebug() << "姿态日志已启动:" << logFile;
    } else {
        qDebug() << "姿态日志文件创建失败:" << logFile;
        delete m_attitudeLogFile;
        m_attitudeLogFile = nullptr;
    }
}

void MainWindow::closeAttitudeLog()
{
    QMutexLocker locker(&m_logMutex);
    if (m_attitudeLogStream) {
        m_attitudeLogStream->flush();
        delete m_attitudeLogStream;
        m_attitudeLogStream = nullptr;
    }
    if (m_attitudeLogFile) {
        if (m_attitudeLogFile->isOpen()) {
            m_attitudeLogFile->close();
        }
        delete m_attitudeLogFile;
        m_attitudeLogFile = nullptr;
    }
}
