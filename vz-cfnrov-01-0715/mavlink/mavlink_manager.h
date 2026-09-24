// mavlink_manager.h
#ifndef MAVLINK_MANAGER_H
#define MAVLINK_MANAGER_H

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include "mavlink.h"

// 车辆状态结构体
struct VehicleState {
    // 心跳状态
    uint8_t systemStatus = MAV_STATE_UNINIT;
    uint8_t baseMode = 0;
    uint32_t customMode = 0;
    uint8_t autopilotType = MAV_AUTOPILOT_INVALID;
    uint8_t vehicleType = MAV_TYPE_GENERIC;
    qint64 lastHeartbeatMs = 0;

    // 系统状态
    struct {
        uint16_t voltageBattery = 0;      // mV
        int16_t currentBattery = 0;       // 10mA
        int8_t batteryRemaining = -1;     // %
        uint16_t dropRateComm = 0;
        uint16_t errorsComm = 0;
        uint32_t sensorsPresent = 0;
        uint32_t sensorsEnabled = 0;
        uint32_t sensorsHealth = 0;
        uint32_t lastUpdateMs = 0;
    } sysStatus;

    // GPS 状态
    struct {
        uint8_t fixType = 0;
        int32_t lat = 0;          // 1e7 degrees
        int32_t lon = 0;          // 1e7 degrees
        int32_t alt = 0;          // mm
        uint16_t vel = 0;         // cm/s
        uint16_t cog = 0;         // cdeg
        uint8_t satellites = 0;
        uint32_t lastUpdateMs = 0;
    } gps;

    // 姿态
    struct {
        float roll = 0;           // rad
        float pitch = 0;          // rad
        float yaw = 0;            // rad
        float rollspeed = 0;      // rad/s
        float pitchspeed = 0;     // rad/s
        float yawspeed = 0;       // rad/s
        float yawCumulative = 0;  // rad (累计偏航角，无跳变)
        uint32_t lastUpdateMs = 0;
    } attitude;

    // 全局位置
    struct {
        int32_t lat = 0;          // 1e7 degrees
        int32_t lon = 0;          // 1e7 degrees
        int32_t alt = 0;          // mm
        int32_t relativeAlt = 0;  // mm
        uint16_t hdg = 0;         // cdeg
        uint32_t lastUpdateMs = 0;
    } globalPos;

    // VFR HUD
    struct {
        float airspeed = 0;       // m/s
        float groundspeed = 0;    // m/s
        int16_t heading = 0;      // deg
        uint16_t throttle = 0;    // %
        float alt = 0;            // m
        float climbRate = 0;      // m/s
        uint32_t lastUpdateMs = 0;
    } vfrHud;

    // 深度信息 (ArduSub)
    struct {
        float depth = 0;          // m
        float depthRate = 0;      // m/s
        float pressure = 0;       // hPa
        float temperature = 0;    // °C
        uint32_t lastUpdateMs = 0;
    } depth;

    // 舵机输出
    struct {
        uint32_t timeUsec = 0;
        uint8_t port = 0;
        uint16_t raw[16] = {0};
        uint8_t count = 0;
        uint32_t lastUpdateMs = 0;
    } servo;

    // 遥控器输入
    struct {
        uint16_t channels[18] = {1500};
        uint8_t rssi = 0;
        uint32_t lastUpdateMs = 0;
    } rcChannels;
};

// 控制模式枚举
enum ControlMode {
    MODE_VEHICLE = 0,   // 车辆运动控制
    MODE_CAMERA,        // 摄像头控制
    MODE_LIGHT,         // 灯光控制
    MODE_DEPTH          // 深度控制
};

// 姿态旋转状态枚举
enum RotationState {
    ROTATION_IDLE = 0,      // 空闲，未在旋转
    ROTATION_TO_CLEAN,      // 正在旋转至清洗姿态（逆时针90度）
    ROTATION_TO_DIVE,       // 正在旋转至下潜姿态（顺时针90度）
    ROTATION_COMPLETED,     // 旋转完成
    ROTATION_ABORTED        // 旋转中止（超时或手动停止）
};

// 机器姿态枚举
enum MachineOrientation {
    ORIENTATION_DIVE = 0,   // 下潜姿态
    ORIENTATION_CLEAN       // 清洗姿态
};

// GUIDED清洗状态机
enum GuidedCleanState {
    GC_IDLE = 0,            // 空闲
    GC_SWITCHING_MODE,      // 正在切换GUIDED模式
    GC_DIVING,              // 下潜到位
    GC_ROTATING,            // 姿态变换(抬头至清洗角度)
    GC_CLEANING,            // 清洗中(保持姿态)
    GC_RECOVERING,          // 恢复姿态(低头回下潜角度)
    GC_DONE,                // 完成
    GC_ABORTED              // 中止
};

class MavlinkManager : public QObject
{
    Q_OBJECT

public:
    /// @brief 创建并初始化 MavlinkManager 对象。
    explicit MavlinkManager(QObject *parent = nullptr);
    /// @brief 停止后台任务并释放 MavlinkManager 占用的资源。
    ~MavlinkManager();

    // 解析接收到的 MAVLink 数据
    /// @brief 解析数据并分发有效数据。
    void parseMavlinkData(const QByteArray &data);

    // 获取车辆状态
    /// @brief 获取ROV 状态、状态。
    const VehicleState& getVehicleState() const { return m_vehicleState; }
    /// @brief 判断是否存在有效的ROV 状态、心跳。
    bool hasVehicleHeartbeat() const { return m_targetDiscovered && m_vehicleState.lastHeartbeatMs > 0; }
    /// @brief 重置ROV 状态、连接。
    void resetVehicleConnection();

    // 控制模式
    /// @brief 设置控制、模式并同步相关状态。
    void setControlMode(ControlMode mode);
    /// @brief 获取控制、模式。
    ControlMode getControlMode() const { return m_controlMode; }

    // 发送 MAVLink 消息
    /// @brief 封装并发送心跳。
    void sendHeartbeat();
    /// @brief 封装并发送解锁状态、命令。
    void sendArmCommand(bool arm, bool force = false);
    /// @brief 封装并发送模式、命令。
    void sendSetModeCommand(uint8_t baseMode, uint32_t customMode);
    /// @brief 封装并发送手动控制、控制。
    void sendManualControl(int16_t x, int16_t y, int16_t z, int16_t r, uint16_t buttons = 0,
                           int16_t s = 0, int16_t t = 0, bool enableExtensions = false);
    /// @brief 封装并发送运动、命令。
    void sendMovementCommand(float forward, float lateral, float vertical, float yawRate);
    /// @brief 封装并发送遥控器、通道。
    void sendRcChannelsOverride(uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4,
                                 uint16_t ch5 = 0, uint16_t ch6 = 0, uint16_t ch7 = 0, uint16_t ch8 = 0);
    /// @brief 封装并发送舵机、命令。
    void sendSetServoCommand(uint8_t channel, uint16_t pwm);
    /// @brief 封装并发送数据、数据流。
    void sendRequestDataStream(uint8_t streamId, uint16_t rate, uint8_t startStop);
    /// @brief 封装并发送消息。
    void sendSetMessageInterval(uint16_t msgId, uint32_t intervalUs);
    /// @brief 封装并发送命令。
    void sendCommandLong(uint16_t command, float param1 = 0, float param2 = 0,
                         float param3 = 0, float param4 = 0, float param5 = 0,
                         float param6 = 0, float param7 = 0);

    /// @brief 封装并发送参数。
    void sendParamSet(const char *paramId, float value, uint8_t paramType = MAV_PARAM_TYPE_REAL32);
    /// @brief 封装并发送参数。
    void sendParamRequestRead(const char *paramId);

    /// @brief 设置画面帧、配置并同步相关状态。
    void setFrameConfig(int frameConfig);
    /// @brief 获取画面帧、配置。
    int getFrameConfig() const { return m_frameConfig; }
    /// @brief 判断“Armed”是否处于有效状态。
    bool isArmed() const { return (m_vehicleState.baseMode & MAV_MODE_FLAG_SAFETY_ARMED) != 0; }

    // Net Cleaner: runtime mode switching via MOT_NET_MODE (no reboot/disarm needed)
    /// @brief 设置网衣清洗、模式并同步相关状态。
    void setNetCleanerMode(int mode);        // 0=Diving, 1=Cleaning
    /// @brief 获取网衣清洗、模式。
    int getNetCleanerMode() const { return m_netCleanerMode; }
    /// @brief 封装并发送模式、网衣清洗。
    void sendDoSetModeForNetCleaner(int mode); // 0=Dive, 1=Clean, 2=Toggle via DO_SET_MODE

    // One-key cleaning: send COMMAND_LONG ID=31000 to firmware state machine
    /// @brief 封装并发送命令。
    void sendCleanerCommand(int mode);  // mode=9: dive->work, mode=8: work->dive

    // GUIDED模式一键清洗流程
    /// @brief 启动引导清洗、清洗。
    void startGuidedClean(float cleanPitchDeg = 80.0f, float cleanDepthM = 0.0f, int cleanDurationSec = 60);
    /// @brief 中止引导清洗、清洗。
    void abortGuidedClean();
    /// @brief 获取引导清洗、清洗、状态。
    GuidedCleanState getGuidedCleanState() const { return m_gcState; }
    /// @brief 判断引导清洗是否处于有效状态。
    bool isGuidedCleaning() const { return m_gcState != GC_IDLE && m_gcState != GC_DONE && m_gcState != GC_ABORTED; }

    // SET_ATTITUDE_TARGET 发送
    /// @brief 封装并发送姿态、目标。
    void sendSetAttitudeTarget(float q[4], float thrust, float rollRate = 0.0f, float pitchRate = 0.0f, float yawRate = 0.0f);

    // 设置系统 ID
    /// @brief 设置系统并同步相关状态。
    void setSystemId(uint8_t sysId) { m_systemId = sysId; }
    /// @brief 设置组件并同步相关状态。
    void setComponentId(uint8_t compId) { m_componentId = compId; }
    /// @brief 设置目标、系统并同步相关状态。
    void setTargetSystem(uint8_t targetSys) { m_targetSystem = targetSys; }
    /// @brief 设置目标、组件并同步相关状态。
    void setTargetComponent(uint8_t targetComp) { m_targetComponent = targetComp; }

    // 获取格式化状态字符串
    /// @brief 获取模式。
    QString getModeString() const;
    /// @brief 获取模式。
    uint32_t getCustomMode() const { return m_vehicleState.customMode; }
    /// @brief 获取解锁状态、状态。
    QString getArmStatusString() const;
    /// @brief 获取“FixTypeString”。
    QString getFixTypeString() const;
    /// @brief 获取系统、状态。
    QString getSystemStatusString() const;
    /// @brief 获取数据包、丢失率、百分比。
    float getPacketLossPercent() const { return m_vehicleState.sysStatus.dropRateComm / 10.0f; }

    // 灯控制
    /// @brief 设置前部、补光灯并同步相关状态。
    void setFrontLight(uint16_t brightness);    // 前灯 PWM 13+14
    /// @brief 设置后部、补光灯并同步相关状态。
    void setRearLight(uint16_t brightness);     // 后灯 PWM 15+16
    /// @brief 切换前部、补光灯的启用状态。
    void toggleFrontLight();                     // 切换前灯开关
    /// @brief 切换后部、补光灯的启用状态。
    void toggleRearLight();                      // 切换后灯开关
    /// @brief 设置“BothLights”并同步相关状态。
    void setBothLights(uint16_t frontBrightness, uint16_t rearBrightness);
    /// @brief 设置前部、补光灯、百分比并同步相关状态。
    void setFrontLightPercent(uint8_t percent);
    /// @brief 设置后部、补光灯、百分比并同步相关状态。
    void setRearLightPercent(uint8_t percent);

    // 云台控制常量
    static constexpr uint8_t PWM_CHANNEL_FRONT_PAN = 13;   // 前摄像头左右旋转
    static constexpr uint8_t PWM_CHANNEL_FRONT_TILT = 14; // 前摄像头上下旋转
    static constexpr uint8_t PWM_CHANNEL_REAR_PAN = 15;   // 后摄像头左右旋转（预留）
    static constexpr uint8_t PWM_CHANNEL_REAR_TILT = 16;  // 后摄像头上下旋转（预留）

    static constexpr uint16_t PWM_GIMBAL_PAN_MIN = 1000;      // 水平最小PWM值
    static constexpr uint16_t PWM_GIMBAL_PAN_MAX = 2000;      // 水平最大PWM值

    static constexpr uint16_t PWM_GIMBAL_TILT_MIN = 1250;      // 上下最小PWM值
    static constexpr uint16_t PWM_GIMBAL_TILT_MAX = 1750;      // 上下最大PWM值
    static constexpr uint16_t PWM_GIMBAL_CENTER = 1500;        // 中位PWM值

    // 前摄像头云台控制
    /// @brief 设置前部、摄像头、水平位置并同步相关状态。
    void setFrontCameraPan(uint16_t pwm);                  // 设置前摄像头左右角度
    /// @brief 设置前部、摄像头、俯仰位置并同步相关状态。
    void setFrontCameraTilt(uint16_t pwm);                 // 设置前摄像头上下角度
    /// @brief 设置前部、摄像头、云台并同步相关状态。
    void setFrontCameraGimbal(uint16_t pan, uint16_t tilt); // 同时设置前后摄像头云台角度
    /// @brief 将前部、摄像头、云台恢复到中位。
    void centerFrontCameraGimbal();                        // 前摄像头云台回中
    /// @brief 停止前部、摄像头、云台并收尾相关状态。
    void stopFrontCameraGimbal();                          // 停止前摄像头云台运动

    // 后摄像头云台控制（预留）
    /// @brief 设置后部、摄像头、水平位置并同步相关状态。
    void setRearCameraPan(uint16_t pwm);
    /// @brief 设置后部、摄像头、俯仰位置并同步相关状态。
    void setRearCameraTilt(uint16_t pwm);
    /// @brief 设置后部、摄像头、云台并同步相关状态。
    void setRearCameraGimbal(uint16_t pan, uint16_t tilt);
    /// @brief 将后部、摄像头、云台恢复到中位。
    void centerRearCameraGimbal();
    /// @brief 停止后部、摄像头、云台并收尾相关状态。
    void stopRearCameraGimbal();

    // 获取云台当前位置
    /// @brief 获取前部、水平位置、位置。
    uint16_t getFrontPanPosition() const { return m_frontPanPosition; }
    /// @brief 获取前部、俯仰位置、位置。
    uint16_t getFrontTiltPosition() const { return m_frontTiltPosition; }
    /// @brief 获取后部、水平位置、位置。
    uint16_t getRearPanPosition() const { return m_rearPanPosition; }
    /// @brief 获取后部、俯仰位置、位置。
    uint16_t getRearTiltPosition() const { return m_rearTiltPosition; }

    // 增量控制（相对移动）
    /// @brief 按给定增量移动前部、水平位置。
    void moveFrontPan(int16_t delta);      // 相对移动前摄像头左右
    /// @brief 按给定增量移动前部、俯仰位置。
    void moveFrontTilt(int16_t delta);     // 相对移动前摄像头上下
    /// @brief 按给定增量移动后部、水平位置。
    void moveRearPan(int16_t delta);       // 相对移动后摄像头左右
    /// @brief 按给定增量移动后部、俯仰位置。
    void moveRearTilt(int16_t delta);      // 相对移动后摄像头上下

    // 姿态旋转控制
    /// @brief 启动旋转、清洗。
    void startRotationToClean();           // 开始旋转至清洗姿态（逆时针90度）
    /// @brief 启动旋转。
    void startRotationToDive();            // 开始旋转至下潜姿态（顺时针90度）
    /// @brief 中止旋转。
    void abortRotation();                  // 中止旋转
    /// @brief 获取旋转、状态。
    RotationState getRotationState() const { return m_rotationState; }
    /// @brief 获取机器姿态。
    MachineOrientation getMachineOrientation() const { return m_machineOrientation; }
    /// @brief 判断“Rotating”是否处于有效状态。
    bool isRotating() const { return m_rotationState == ROTATION_TO_CLEAN || m_rotationState == ROTATION_TO_DIVE; }


signals:
    // 需要发送数据的信号（连接到 UdpHandler）
    /// @brief 封装并发送数据。
    void sendData(const QByteArray &data);

    // 状态更新信号
    /// @brief 通知订阅者ROV 状态、状态已经更新。
    void vehicleStateUpdated();
    /// @brief 通知订阅者已收到心跳。
    void heartbeatReceived();
    /// @brief 通知订阅者姿态已经更新。
    void attitudeUpdated(float roll, float pitch, float yaw,
                         float rollspeed, float pitchspeed, float yawspeed,
                         float yawCumulative);
    /// @brief 通知订阅者深度已经更新。
    void depthUpdated(float depth);
    /// @brief 通知订阅者GPS已经更新。
    void gpsUpdated(uint8_t fixType, uint8_t satellites, double lat, double lon);
    /// @brief 通知订阅者电池已经更新。
    void batteryUpdated(float voltage, int8_t remaining);
    /// @brief 通知订阅者舵机已经更新。
    void servoUpdated();
    /// @brief 通知订阅者遥控器、通道已经更新。
    void rcChannelsUpdated();

    // 消息接收信号
    /// @brief 通知订阅者已收到状态、文本。
    void statusTextReceived(const QString &text, uint8_t severity);
    /// @brief 通知订阅者已收到命令、应答。
    void commandAckReceived(uint16_t command, uint8_t result);

    /// @brief 通知订阅者已收到参数、数值。
    void paramValueReceived(const QString &paramId, float value);
    /// @brief 通知订阅者画面帧、配置已经变化。
    void frameConfigChanged(int frameConfig);
    /// @brief 通知订阅者网衣清洗、模式已经变化。
    void netCleanerModeChanged(int mode);

    // 错误信号
    /// @brief 通知订阅者发生“parse”错误。
    void parseError(const QString &error);

    // 云台状态更新信号
    /// @brief 通知订阅者前部、云台已经更新。
    void frontGimbalUpdated(uint16_t pan, uint16_t tilt);
    /// @brief 通知订阅者后部、云台已经更新。
    void rearGimbalUpdated(uint16_t pan, uint16_t tilt);

    // 姿态旋转状态更新信号
    /// @brief 通知订阅者旋转、状态已经变化。
    void rotationStateChanged(RotationState state);
    /// @brief 通知订阅者机器姿态已经变化。
    void machineOrientationChanged(MachineOrientation orientation);

    // GUIDED清洗状态更新信号
    /// @brief 通知订阅者引导清洗、清洗、状态已经变化。
    void guidedCleanStateChanged(GuidedCleanState state);

private slots:
    /// @brief 处理心跳对应的事件或信号回调。
    void onHeartbeatTimer();
    /// @brief 处理旋转、控制对应的事件或信号回调。
    void onRotationControlTimer();
    /// @brief 处理引导清洗、清洗对应的事件或信号回调。
    void onGuidedCleanTimer();
    /// @brief 处理日志对应的事件或信号回调。
    void onAllLogTimer();
private:
    // MAVLink 消息处理函数
    /// @brief 解析并处理收到的心跳。
    void handleHeartbeat(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的状态。
    void handleSysStatus(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的GPS、原始数据。
    void handleGpsRawInt(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的姿态。
    void handleAttitude(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的全局位置、位置。
    void handleGlobalPositionInt(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的飞行信息。
    void handleVfrHud(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的缩放数据、压力。
    void handleScaledPressure(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的缩放数据、压力。
    void handleScaledPressure2(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的缩放数据、压力。
    void handleScaledPressure3(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的舵机、原始数据。
    void handleServoOutputRaw(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的遥控器、通道。
    void handleRcChannels(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的遥控器、通道、原始数据。
    void handleRcChannelsRaw(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的“Statustext”。
    void handleStatustext(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的命令、应答。
    void handleCommandAck(const mavlink_message_t &msg);
    /// @brief 解析并处理收到的参数、数值。
    void handleParamValue(const mavlink_message_t &msg);

    // 打包并发送 MAVLink 消息
    template<typename T>
    /// @brief 使用指定打包函数生成并发送 MAVLink 消息。
    void packAndSend(void (*pack_func)(uint8_t, uint8_t, mavlink_message_t*, const T*), const T* msg_data) {
        mavlink_message_t msg;
        /// @brief 执行“pack_func”对应的业务操作。
        pack_func(m_systemId, m_componentId, &msg, msg_data);
        /// @brief 封装并发送消息。
        sendMavlinkMessage(msg);
    }

    /// @brief 封装并发送消息。
    void sendMavlinkMessage(mavlink_message_t &msg);

    // 辅助函数
    /// @brief 获取模式。
    QString getCustomModeString(uint32_t customMode) const;
    /// @brief 设置引导清洗、清洗、状态并同步相关状态。
    void setGuidedCleanState(GuidedCleanState state);
    /// @brief 根据最新数据更新深度、压力。
    void updateDepthFromPressure(float pressureHpa, int16_t temperatureCentiDeg);

    // 命令日志（内部使用）
    /// @brief 执行日志、命令对应的业务操作。
    void logCommandSend(const mavlink_message_t &msg);
    /// @brief 执行“mavlinkMsgName”对应的业务操作。
    QString mavlinkMsgName(uint32_t msgid) const;
    /// @brief 执行“mavlinkCmdName”对应的业务操作。
    QString mavlinkCmdName(uint16_t cmd) const;
    /// @brief 执行应答对应的业务操作。
    QString mavlinkAckResultString(uint8_t result) const;

public:
    /// @brief 将欧拉角转换为姿态目标使用的四元数。
    static void eulerToQuaternion(float roll, float pitch, float yaw, float q[4]);

    // 命令日志（外部调用）
    /// @brief 初始化命令、日志所需的对象和连接。
    void initCommandLog();
    /// @brief 关闭并释放命令、日志相关资源。
    void closeCommandLog();
    /// @brief 初始化日志所需的对象和连接。
    void initAllLog();
    /// @brief 关闭并释放日志相关资源。
    void closeinitAllLog();
private:
    VehicleState m_vehicleState;
    ControlMode m_controlMode = MODE_VEHICLE;

    // MAVLink 系统 ID
    uint8_t m_systemId = 255;        // GCS ID
    uint8_t m_componentId = 190;     // GCS Component ID
    uint8_t m_targetSystem = 1;      // 目标系统 ID
    uint8_t m_targetComponent = 1;   // 目标组件 ID
    bool m_targetDiscovered = false; // 仅由真实飞控心跳建立，拒绝GCS回环心跳

    // 命令日志
    QFile *m_cmdLogFile = nullptr;
    QTextStream *m_cmdLogStream = nullptr;
    QMutex m_cmdLogMutex;



    // 定时器
    QTimer *m_heartbeatTimer = nullptr;

    // 互斥锁
    QMutex m_stateMutex;

    // 统计
    uint32_t m_msgCount = 0;
    uint32_t m_parseErrors = 0;
    uint32_t m_crcErrors = 0;
    // 云台当前位置状态
    uint16_t m_frontPanPosition = PWM_GIMBAL_CENTER;
    uint16_t m_frontTiltPosition = PWM_GIMBAL_CENTER;
    uint16_t m_rearPanPosition = PWM_GIMBAL_CENTER;
    uint16_t m_rearTiltPosition = PWM_GIMBAL_CENTER;

    // 姿态旋转控制状态
    RotationState m_rotationState = ROTATION_IDLE;
    MachineOrientation m_machineOrientation = ORIENTATION_DIVE;
    float m_rotationStartYaw = 0.0f;          // 旋转起始偏航角(弧度)
    float m_rotationTargetYaw = 0.0f;         // 旋转目标偏航角(弧度)
    QTimer *m_rotationControlTimer = nullptr;  // 旋转控制定时器
    uint32_t m_rotationStartTimeMs = 0;       // 旋转开始时间
    static constexpr uint32_t ROTATION_TIMEOUT_MS = 15000;  // 旋转超时15秒
    static constexpr float ROTATION_TARGET_ANGLE = M_PI / 2.0f;  // 目标旋转角度90度
    static constexpr float ROTATION_COMPLETE_THRESHOLD = 0.087f;  // 完成阈值约5度
    static constexpr int16_t ROTATION_YAW_RATE = 500;  // 旋转偏航指令值(0-1000)

    int m_frameConfig = 8;  // FRAME_CONFIG参数值，默认8=Cleaner (对应固件SUB_FRAME_CLEANER)
    int m_netCleanerMode = 0;  // MOT_NET_MODE: 0=Diving, 1=Cleaning

    // GUIDED清洗状态机
    GuidedCleanState m_gcState = GC_IDLE;
    QTimer *m_gcTimer = nullptr;             // 持续发送SET_ATTITUDE_TARGET (10Hz)
    float m_gcTargetQ[4] = {1,0,0,0};       // 当前目标四元数
    float m_gcThrust = 0.5f;                 // 推力(0~1)
    float m_gcCleanPitchDeg = 80.0f;         // 清洗俯仰角(度)
    float m_gcCleanDepthM = 0.0f;            // 清洗目标深度(m)
    int m_gcCleanDurationSec = 60;           // 清洗持续时间(秒)
    uint32_t m_gcCleanStartTimeMs = 0;       // 清洗开始时间
    uint32_t m_gcStateEntryTimeMs = 0;       // 当前状态进入时间
    static constexpr uint32_t GC_DIVE_TIMEOUT_MS = 30000;    // 下潜超时30秒
    static constexpr uint32_t GC_ROTATE_TIMEOUT_MS = 15000;  // 姿态变换超时15秒
    static constexpr float GC_ATTITUDE_THRESHOLD_DEG = 5.0f; // 姿态到位阈值(度)
    static constexpr float GC_DEPTH_THRESHOLD_M = 0.3f;      // 深度到位阈值(m)

    //xyk 2026 0909 增加所有收到的数据并生成日志
    QTimer *m_AlllogTimer = nullptr;
    QFile *m_AllLogFile = nullptr;
    QTextStream *m_AllLogStream = nullptr;
    QMutex m_AllLogMutex;


};

#endif // MAVLINK_MANAGER_H
