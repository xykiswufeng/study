/***************************************************************************//**
 * @file example/MainWindow.h
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
#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QKeyEvent>
#include <QLabel>
#include <QComboBox>
#include <QTime>
#include <QTimer>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDockWidget>
#include <QPushButton>
#include "activemap.h"
#include "pwmdisplaywidget.h"
#include "attitude3dwidget.h"
#include "rovdashboardwidgets.h"

//摄像头
#include <QCameraImageCapture>
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QCamera>

//UDP
#include <QHostAddress>
#include <QUdpSocket>
#include "ffmpegrtspplayer.h"
#include "udphandler.h"
// mavlink
#include "mavlink/mavlink_manager.h"
// 虚拟摇杆
#include  "virtualjoystick.h"
// 游戏手柄
#include "sdl2gamepad.h"
#include "slidetounlock.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow{
    Q_OBJECT
    
public:
    /// @brief 创建并初始化 MainWindow 对象。
    explicit MainWindow(QWidget* parent = nullptr);
    /// @brief 停止后台任务并释放 MainWindow 占用的资源。
    ~MainWindow();

    ActiveMap actMap;

protected:
    //void timerEvent( QTimerEvent *event );
    /// @brief 处理主窗口状态变化事件。
    void changeEvent(QEvent *event);
    /// @brief 处理主窗口关闭事件并执行安全退出流程。
    void closeEvent(QCloseEvent *event);
    /// @brief 处理窗口尺寸变化并重新调整子控件。
    void resizeEvent(QResizeEvent *event);
    /// @brief 处理键盘按下事件并更新控制指令。
    void keyPressEvent(QKeyEvent *event);
    /// @brief 处理键盘释放事件并停止对应控制指令。
    void keyReleaseEvent(QKeyEvent *event);
    /// @brief 过滤目标控件事件，并转交自定义绘制或交互逻辑。
    bool eventFilter(QObject *obj, QEvent *event) override;
private slots:
    /// @brief 读取并处理套接字中等待接收的 UDP 数据报。
    void ReadPendingDataframs();//读取UDP消息

    /// @brief 处理按钮、全屏、点击对应的事件或信号回调。
    void on_pushButton_fullscreen_clicked();
    /// @brief 处理按钮、点击对应的事件或信号回调。
    void on_pushButton_close_clicked();
    /// @brief 处理按钮、设置、点击对应的事件或信号回调。
    void on_pushButton_setup_clicked();

    // UDP 槽函数
    /// @brief 处理UDP、点击对应的事件或信号回调。
    void onUdpConnectClicked();
    /// @brief 处理UDP、点击对应的事件或信号回调。
    void onUdpDisconnectClicked();
    /// @brief 处理UDP、点击对应的事件或信号回调。
    void onUdpSendClicked();
    /// @brief 通知订阅者已收到UDP、数据。
    void onUdpDataReceived(const QByteArray &data, const QHostAddress &senderAddr, quint16 senderPort);
    /// @brief 处理UDP对应的事件或信号回调。
    void onUdpConnected();
    /// @brief 处理UDP对应的事件或信号回调。
    void onUdpDisconnected();
    /// @brief 通知订阅者发生UDP错误。
    void onUdpError(const QString &error);

    // RTSP 槽函数
    /// @brief 处理播放、点击对应的事件或信号回调。
    void onRtspPlayClicked();
    /// @brief 处理点击对应的事件或信号回调。
    void onRtspStopClicked();
    /// @brief 处理点击对应的事件或信号回调。
    void onRtspCaptureClicked();
    /// @brief 处理“RtspStarted”对应的事件或信号回调。
    void onRtspStarted();
    /// @brief 处理“RtspStopped”对应的事件或信号回调。
    void onRtspStopped();
    /// @brief 通知订阅者已收到画面帧。
    void onNewFrameReceived(const QImage &image);
    /// @brief 通知订阅者视频、尺寸已经变化。
    void onVideoSizeChanged(const QSize &size);
    /// @brief 通知订阅者发生“onRtsp”错误。
    void onRtspError(const QString &error);
    /// @brief 通知订阅者帧率已经更新。
    void onRtspFpsUpdated(double fps);
    /// @brief 处理云台、控制对应的事件或信号回调。
    void onGimbalControlTimer();
    /// @brief 将前部、云台恢复到中位。
    void centerFrontGimbal();           // 前云台回中
    /// @brief 设置前部、云台并同步相关状态。
    void setFrontGimbalPreset(int preset); // 预设位置

    // 后摄像头槽函数
    /// @brief 处理后部对应的事件或信号回调。
    void onRearRtspStarted();
    /// @brief 处理后部对应的事件或信号回调。
    void onRearRtspStopped();
    /// @brief 通知订阅者发生后部错误。
    void onRearRtspError(const QString &error);
    /// @brief 通知订阅者已收到后部、画面帧。
    void onRearNewFrameReceived(const QImage &image);
    /// @brief 通知订阅者后部、帧率已经更新。
    void onRearRtspFpsUpdated(double fps);
    /// @brief 通知订阅者后部、视频、尺寸已经变化。
    void onRearVideoSizeChanged(const QSize &size);
    /// @brief 处理前部、重试、超时对应的事件或信号回调。
    void onFrontCamRetryTimeout();
    /// @brief 处理后部、重试、超时对应的事件或信号回调。
    void onRearCamRetryTimeout();
    /// @brief 处理心跳、检查、超时对应的事件或信号回调。
    void onHeartbeatCheckTimeout();
    /// @brief 处理前部、画面帧、检查、超时对应的事件或信号回调。
    void onFrontFrameCheckTimeout();
    /// @brief 处理后部、画面帧、检查、超时对应的事件或信号回调。
    void onRearFrameCheckTimeout();

    /// @brief 处理后部、云台、控制对应的事件或信号回调。
    void onRearGimbalControlTimer();


private:
    /// @brief 应用主界面的统一主题和控件样式属性。
    void applyProfessionalTheme();
    /// @brief 安装仪表盘自绘控件并连接面板交互信号。
    void setupFourRowDashboard();
    /// @brief 按安全顺序停止定时器、网络和视频后台任务。
    void stopBackgroundServices();
    /// @brief 设置状态、标识并同步相关状态。
    void setStatusBadge(QLabel *label, const QString &text, const char *state);
    /// @brief 绘制视频、控件。
    void paintVideoWidget(QPaintEvent *event);
    /// @brief 绘制后部、视频、控件。
    void paintRearVideoWidget(QPaintEvent *event);
    /// @brief 绘制摄像头、控件。
    void paintCameraWidget(QWidget *target, bool rearCamera);
    /// @brief 打开或激活指定摄像头的独立放大预览窗口。
    void showCameraPreview(bool rearCamera);
    /// @brief 根据最新数据更新摄像头、放大、按钮。
    void updateCameraExpandButtonPositions();
    /// @brief 根据 ROV 地址生成前摄像头 RTSP 地址。
    QString frontCameraUrl() const;
    /// @brief 根据 ROV 地址生成后摄像头 RTSP 地址。
    QString rearCameraUrl() const;
    /// @brief 使用最新配置重新连接前后摄像头视频流。
    void restartCameraStreams();
    /// @brief 切换视频、录像的启用状态。
    void toggleVideoRecording(FFmpegRtspPlayer *player,
                              const QString &cameraTag,
                              const QString &cameraName);
    /// @brief 根据最新数据更新录像、按钮。
    void updateRecordingButton(QPushButton *button, bool recording);
    /// @brief 初始化手柄所需的对象和连接。
    void initGamepad();
    /// @brief 建立手柄所需的信号连接。
    void connectGamepadSignals();
    /// @brief 设置手动控制、控制、按钮并同步相关状态。
    void setManualControlButton(uint16_t mask, bool pressed);
    /// @brief 根据最新数据更新手动控制、控制。
    void updateKeyboardManualControl();
    /// @brief 请求“MavlinkStreams”。
    void requestMavlinkStreams();
    /// @brief 根据最新数据更新云台、状态。
    void updateGimbalStatus();
    Ui::MainWindow *ui{};

    // 状态栏信息
    QLabel *labCell_0 = nullptr;
    QLabel *labCell_1 = nullptr;
    QLabel *labCell_2 = nullptr;

    QLabel *labCell_rpy = nullptr;
    QLabel *m_depthValueLabel = nullptr;
    QLabel *m_speedValueLabel = nullptr;
    QLabel *m_headingValueLabel = nullptr;
    QLabel *m_pressureValueLabel = nullptr;
    QLabel *m_connectionStateValue = nullptr;
    QLabel *m_connectionAddressValue = nullptr;
    ROVAttitudeGauge *m_pitchGauge = nullptr;
    ROVAttitudeGauge *m_rollGauge = nullptr;
    ROVAttitudeGauge *m_headingGauge = nullptr;
    ROVDepthGauge *m_depthGauge = nullptr;
    ROVThrusterGauge *m_thrusterGauge = nullptr;
    QPushButton *m_emergencyButton = nullptr;
    bool m_closeAccepted = false;
    bool m_shutdownComplete = false;


    // 创建摄像头对象
    QCamera *m_camera = nullptr;
    QCameraViewfinder *m_vf = nullptr;
    QCameraImageCapture *m_capture = nullptr;

    //UDP
    QHostAddress m_groupAddress;
    QUdpSocket *m_udpSocket = nullptr;

    bool m_frontLightOn = false;   // 前灯状态
    bool m_rearLightOn = false;    // 后灯状态
    int m_frontLightBrightness = 0; // 前灯亮度 0-100
    int m_rearLightBrightness = 0;  // 后灯亮度 0-100

    /// @brief 执行显示、设置对应的业务操作。
    void showSetupDlg();

    /// @brief 初始化UDP所需的对象和连接。
    void initUdpSocket();

    //bool bInited = false;

    int m_timerId{};
    int m_steps{};

    float m_realTime{};

    QTime m_time;
    // UDP 相关
    UdpHandler *m_udpHandler;
    // RTSP 相关
    FFmpegRtspPlayer *m_rtspPlayer;
    FFmpegRtspPlayer *m_rtspPlayerRear;
    QPushButton *m_frontRecordButton = nullptr;
    QPushButton *m_rearRecordButton = nullptr;
    QPushButton *m_frontExpandButton = nullptr;
    QPushButton *m_rearExpandButton = nullptr;
    QWidget *m_frontPreviewWindow = nullptr;
    QWidget *m_rearPreviewWindow = nullptr;
    QWidget *m_frontPreviewCanvas = nullptr;
    QWidget *m_rearPreviewCanvas = nullptr;
    double m_frontVideoFps = 0.0;
    double m_rearVideoFps = 0.0;
    QString m_frontVideoStatus = QString::fromUtf8("等待连接前摄像头");
    QString m_rearVideoStatus = QString::fromUtf8("等待连接后摄像头");
    QMutex m_countMutex;
    // 当前视频帧
    QImage m_currentFrame;
    QImage m_currentFrameRear;
    QMutex m_frameMutex;
    // 视频画面横滚补偿状态。
    bool m_videoLevelReferenceValid = false;//是否已经记录初始角度
    float m_videoLevelReferenceRollDeg = 0.0f;//首次收到的roll,作为画面初始方向
    float m_videoLevelCompensationDeg = 0.0f;//当前roll,相对初始roll的变化量
    unsigned int m_udpDataCount;
    // 摄像头重连定时器
    QTimer *m_frontCamRetryTimer = nullptr;
    QTimer *m_rearCamRetryTimer = nullptr;

    // 网络心跳超时检测
    QTimer *m_heartbeatCheckTimer = nullptr;
    bool m_mavlinkConnected = false;

    // 视频帧超时检测
    QTimer *m_frontFrameCheckTimer = nullptr;
    QTimer *m_rearFrameCheckTimer = nullptr;

    // 云台控制状态
    bool m_panLeftPressed = false;
    bool m_panRightPressed = false;
    bool m_tiltUpPressed = false;
    bool m_tiltDownPressed = false;

    // 后云台控制状态
    bool m_rearPanLeftPressed = false;
    bool m_rearPanRightPressed = false;
    bool m_rearTiltUpPressed = false;
    bool m_rearTiltDownPressed = false;

    // 云台控制步长
    static constexpr int PAN_STEP = 10;
    static constexpr int TILT_STEP = 10;

    // 云台控制定时器
    QTimer *m_gimbalControlTimer = nullptr;
    QTimer *m_rearGimbalControlTimer = nullptr;
    /// @brief 获取命令。
    QString getCommandName(uint16_t command);

private:
    MavlinkManager *m_mavlinkManager = nullptr;
    Attitude3DWidget *m_attitude3DWidget = nullptr;

    QFile *m_attitudeLogFile = nullptr;
    QTextStream *m_attitudeLogStream = nullptr;
    QMutex m_logMutex;
    /// @brief 初始化姿态、日志所需的对象和连接。
    void initAttitudeLog();
    /// @brief 关闭并释放姿态、日志相关资源。
    void closeAttitudeLog();

    // PWM显示
    QDockWidget *m_pwmDock = nullptr;
    PwmDisplayWidget *m_pwmDisplayWidget = nullptr;

    // 游戏手柄
    SDL2Gamepad *m_gamepad = nullptr;
    bool m_gamepadConnected = false;

    // MANUAL_CONTROL字段 (与QGC一致: x=pitch, y=roll, z=throttle, r=yaw, s=pitchExt, t=rollExt)
    int16_t m_manualX = 0;      // x = pitch
    int16_t m_manualY = 0;      // y = roll
    int16_t m_manualZ = 500;    // z = throttle [0,1000], 500=中位
    int16_t m_manualR = 0;      // r = yaw
    int16_t m_manualS = 0;      // s = pitch extension
    int16_t m_manualT = 0;      // t = roll extension
    uint16_t m_manualButtons = 0;
    bool m_manualControlActive = false;
    bool m_armingInProgress = false;
    bool m_emergencyStop = false;
    bool m_keyYawLeft = false;
    bool m_keyYawRight = false;
    bool m_keyPitchUp = false;
    bool m_keyPitchDown = false;
    bool m_keyRollLeft = false;
    bool m_keyRollRight = false;
    QTimer *m_manualControlTimer = nullptr;
    SlideToUnlock *m_slideArm = nullptr;
    SlideToUnlock *m_slideDisarm = nullptr;

    double m_leftStickX = 0;
    double m_leftStickY = 0;
    double m_rightStickX = 0;
    double m_rightStickY = 0;
    /// @brief 执行“applyCrossGate”对应的业务操作。
    void applyCrossGate(double x, double y, double &outX, double &outY);
    // 虚拟摇杆
    VirtualJoystick *m_virtualJoystick;
    bool m_virtualJoystickEnabled;

    // 摇杆控制定时器
    QTimer *m_joystickTimer;



    // 添加槽函数声明
private slots:
    // MAVLink 槽函数
    /// @brief 通知订阅者已收到心跳。
    void onHeartbeatReceived();
    /// @brief 通知订阅者姿态已经更新。
    void onAttitudeUpdated(float roll, float pitch, float yaw,
                           float rollspeed, float pitchspeed, float yawspeed,
                           float yawCumulative);
    /// @brief 通知订阅者GPS已经更新。
    void onGpsUpdated(uint8_t fixType, uint8_t satellites, double lat, double lon);
    /// @brief 通知订阅者已收到状态、文本。
    void onStatusTextReceived(const QString &text, uint8_t severity);
    /// @brief 通知订阅者已收到命令、应答。
    void onCommandAckReceived(uint16_t command, uint8_t result);

    /// @brief 通知订阅者模式已经变化。
    void onFlightModeChanged(int index);
    /// @brief 处理解锁状态对应的事件或信号回调。
    void onSlideArmTriggered();
    /// @brief 处理“SlideDisarmTriggered”对应的事件或信号回调。
    void onSlideDisarmTriggered();

    // PFD 数据更新槽函数
    /// @brief 根据最新数据更新“PfdFromMavlink”。
    void updatePfdFromMavlink();
    /// @brief 根据最新数据更新姿态。
    void updatePfdAttitude(float roll, float pitch, float yaw);
    /// @brief 根据最新数据更新“PfdClimbRate”。
    void updatePfdClimbRate(float climbRate);
    // 虚拟摇杆槽函数
    /// @brief 处理摇杆对应的事件或信号回调。
    void onVirtualJoystickToggled(bool enabled);
    /// @brief 通知订阅者摇杆、数值已经变化。
    void onJoystickValueChanged(qreal roll, qreal pitch, qreal yaw, qreal thrust);

#if 0
    // 控制按钮槽函数
    /// @brief 处理解锁状态、点击对应的事件或信号回调。
    void on_btn_arm_clicked();
    /// @brief 处理点击对应的事件或信号回调。
    void on_btn_disarm_clicked();
    /// @brief 处理引导清洗、点击对应的事件或信号回调。
    void on_btn_guided_clicked();
    /// @brief 处理手动控制、点击对应的事件或信号回调。
    void on_btn_manual_clicked();
    /// @brief 处理点击对应的事件或信号回调。
    void on_btn_stabilize_clicked();
    /// @brief 处理点击对应的事件或信号回调。
    void on_btn_poshold_clicked();

    // 运动控制槽函数
    /// @brief 处理“_btn_forward_pressed”对应的事件或信号回调。
    void on_btn_forward_pressed();
    /// @brief 处理“_btn_forward_released”对应的事件或信号回调。
    void on_btn_forward_released();
    /// @brief 处理“_btn_backward_pressed”对应的事件或信号回调。
    void on_btn_backward_pressed();
    /// @brief 处理“_btn_backward_released”对应的事件或信号回调。
    void on_btn_backward_released();
    /// @brief 处理“_btn_left_pressed”对应的事件或信号回调。
    void on_btn_left_pressed();
    /// @brief 处理“_btn_left_released”对应的事件或信号回调。
    void on_btn_left_released();
    /// @brief 处理“_btn_right_pressed”对应的事件或信号回调。
    void on_btn_right_pressed();
    /// @brief 处理“_btn_right_released”对应的事件或信号回调。
    void on_btn_right_released();
    /// @brief 处理“_btn_up_pressed”对应的事件或信号回调。
    void on_btn_up_pressed();
    /// @brief 处理“_btn_up_released”对应的事件或信号回调。
    void on_btn_up_released();
    /// @brief 处理“_btn_down_pressed”对应的事件或信号回调。
    void on_btn_down_pressed();
    /// @brief 处理“_btn_down_released”对应的事件或信号回调。
    void on_btn_down_released();
    /// @brief 处理航向对应的事件或信号回调。
    void on_btn_yaw_left_pressed();
    /// @brief 处理航向对应的事件或信号回调。
    void on_btn_yaw_left_released();
    /// @brief 处理航向对应的事件或信号回调。
    void on_btn_yaw_right_pressed();
    /// @brief 处理航向对应的事件或信号回调。
    void on_btn_yaw_right_released();
    /// @brief 处理点击对应的事件或信号回调。
    void on_btn_stop_clicked();
#endif
    // 手动控制定时器
    /// @brief 处理手动控制、控制对应的事件或信号回调。
    void onManualControlTimer();

    /// @brief 通知订阅者前部、补光灯已经变化。
    void onFrontLightBrightnessChanged(int value);
    /// @brief 通知订阅者后部、补光灯已经变化。
    void onRearLightBrightnessChanged(int value);

    /// @brief 处理PWM、按钮、点击对应的事件或信号回调。
    void onPwmButtonClicked();
    /// @brief 通知订阅者舵机已经更新。
    void onServoUpdated();
    /// @brief 通知订阅者PWM已经变化。
    void onPwmReverseChanged(int channel, bool reversed);
    /// @brief 通知订阅者PWM已经变化。
    void onPwmEnabledChanged(int channel, bool enabled);
    /// @brief 通知订阅者已收到参数、数值。
    void onParamValueReceived(const QString &paramId, float value);

    // 姿态旋转槽函数
    /// @brief 处理旋转、清洗、点击对应的事件或信号回调。
    void onRotationToCleanClicked();
    /// @brief 处理旋转、点击对应的事件或信号回调。
    void onRotationToDiveClicked();
    /// @brief 处理旋转、点击对应的事件或信号回调。
    void onRotationAbortClicked();
    /// @brief 通知订阅者旋转、状态已经变化。
    void onRotationStateChanged(RotationState state);
    /// @brief 通知订阅者机器姿态已经变化。
    void onMachineOrientationChanged(MachineOrientation orientation);

    /// @brief 通知订阅者画面帧、配置已经变化。
    void onFrameConfigChanged(int frameConfig);

    /// @brief 处理清洗、模式对应的事件或信号回调。
    void onSwitchToCleanMode();
    /// @brief 处理模式对应的事件或信号回调。
    void onSwitchToDiveMode();

    /// @brief 处理“EmergencyStop”对应的事件或信号回调。
    void onEmergencyStop();

};

#endif
