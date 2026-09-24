// mavlink_manager.cpp
#include "mavlink_manager.h"
#include <QDebug>
#include <QtMath>
#include <cmath>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>

// ArduSub 自定义模式定义
#define SUB_MODE_STABILIZE     0
#define SUB_MODE_ACRO          1
#define SUB_MODE_ALT_HOLD      2
#define SUB_MODE_AUTO          3
#define SUB_MODE_GUIDED        4
#define SUB_MODE_CIRCLE        7
#define SUB_MODE_SURFACE       9
#define SUB_MODE_POSHOLD       16
#define SUB_MODE_MANUAL        19
#define SUB_MODE_MOTOR_DETECT  20
#define DEBUG qDebug() << __FILE__ << __LINE__

// 分别建立心跳、姿态控制、清洗流程和日志采样定时器；
// 控制定时器在对应操作启动前保持停止状态。
MavlinkManager::MavlinkManager(QObject *parent)
    : QObject(parent)
{
    // 初始化心跳定时器 (1Hz)
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(1000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &MavlinkManager::onHeartbeatTimer);
    m_heartbeatTimer->start();

    // 初始化旋转控制定时器 (50ms = 20Hz)
    m_rotationControlTimer = new QTimer(this);
    m_rotationControlTimer->setInterval(50);
    connect(m_rotationControlTimer, &QTimer::timeout, this, &MavlinkManager::onRotationControlTimer);

    // 初始化GUIDED清洗定时器 (100ms = 10Hz)
    m_gcTimer = new QTimer(this);
    m_gcTimer->setInterval(100);
    connect(m_gcTimer, &QTimer::timeout, this, &MavlinkManager::onGuidedCleanTimer);

    //xyk 20260909能获取到的数据全部写入日志中
    m_AlllogTimer =new QTimer(this);
    m_AlllogTimer->setInterval(100);
    connect(m_AlllogTimer, &QTimer::timeout, this, &MavlinkManager::onAllLogTimer);
    qDebug() << "MavlinkManager initialized";
    m_AlllogTimer->start();
}

MavlinkManager::~MavlinkManager()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
    if (m_rotationControlTimer) {
        m_rotationControlTimer->stop();
    }
    if (m_gcTimer) {
        m_gcTimer->stop();
    }
    closeCommandLog();
    closeinitAllLog();
}

// 按字节解析 MAVLink 数据包，先排除本机回环，再按消息 ID 更新遥测。
// 未发现飞控前只用真实心跳建立目标，避免把其他设备误认为当前 ROV。
void MavlinkManager::parseMavlinkData(const QByteArray &data)
{
    mavlink_message_t msg;
    mavlink_status_t status{};

    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.constData());
    int len = data.size();

    for (int i = 0; i < len; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status))
        {
            m_msgCount++;

            // 绝不能把经网络回环回来的本机GCS消息当作Pixhawk遥测。
            if (msg.sysid == m_systemId && msg.compid == m_componentId) {
                continue;
            }

            // 建立真实飞控心跳前只接受HEARTBEAT；建立后只接受同一飞控系统的消息。
            // 这样即使UDP/串口桥配置成回显，也不会出现“虚假在线”和模拟遥测。
            if (msg.msgid != MAVLINK_MSG_ID_HEARTBEAT) {
                if (!m_targetDiscovered || msg.sysid != m_targetSystem) {
                    continue;
                }
            }

            // 根据消息 ID 分发处理
            switch (msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                handleHeartbeat(msg);
                break;
            case MAVLINK_MSG_ID_SYS_STATUS:
                handleSysStatus(msg);
                break;
            case MAVLINK_MSG_ID_GPS_RAW_INT:
                handleGpsRawInt(msg);
                break;
            case MAVLINK_MSG_ID_ATTITUDE:
                handleAttitude(msg);
                break;
            case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
                handleGlobalPositionInt(msg);
                break;
            case MAVLINK_MSG_ID_VFR_HUD:
                handleVfrHud(msg);
                break;
            case MAVLINK_MSG_ID_SCALED_PRESSURE:
                handleScaledPressure(msg);
                break;
            case MAVLINK_MSG_ID_SCALED_PRESSURE2:
                handleScaledPressure2(msg);
                break;
            case MAVLINK_MSG_ID_SCALED_PRESSURE3:
                handleScaledPressure3(msg);
                break;
            case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
                handleServoOutputRaw(msg);
                break;
            case MAVLINK_MSG_ID_RC_CHANNELS:
                handleRcChannels(msg);
                break;
            case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
                handleRcChannelsRaw(msg);
                break;
            case MAVLINK_MSG_ID_STATUSTEXT:
                handleStatustext(msg);
                break;
            case MAVLINK_MSG_ID_COMMAND_ACK:
                handleCommandAck(msg);
                break;
            case MAVLINK_MSG_ID_PARAM_VALUE:
                handleParamValue(msg);
                break;
            default:
                // 忽略未处理的消息
                break;
            }
        } else if (status.parse_state == MAVLINK_PARSE_STATE_GOT_BAD_CRC1) {
            m_parseErrors++;
        }
    }
}

// 心跳负责发现飞控并刷新在线状态；地面站心跳与其他系统的心跳均忽略。
void MavlinkManager::handleHeartbeat(const mavlink_message_t &msg)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&msg, &heartbeat);

    // MAV_AUTOPILOT_INVALID / MAV_TYPE_GCS 是地面站或伴随计算机心跳，不能用于
    // 判定Pixhawk在线。这里只接受真正的自动驾驶仪心跳。
    if (heartbeat.autopilot == MAV_AUTOPILOT_INVALID ||
        heartbeat.type == MAV_TYPE_GCS ||
        (msg.sysid == m_systemId && msg.compid == m_componentId)) {
        return;
    }

    if (m_targetDiscovered && msg.sysid != m_targetSystem) {
        return;
    }

    QMutexLocker locker(&m_stateMutex);

    if (!m_targetDiscovered) {
        m_targetSystem = msg.sysid;
        m_targetComponent = msg.compid;
        m_targetDiscovered = true;
        qDebug() << "Detected MAVLink vehicle:" << m_targetSystem << m_targetComponent;
    }

    m_vehicleState.systemStatus = heartbeat.system_status;
    m_vehicleState.baseMode = heartbeat.base_mode;
    m_vehicleState.customMode = heartbeat.custom_mode;
    m_vehicleState.autopilotType = heartbeat.autopilot;
    m_vehicleState.vehicleType = heartbeat.type;
    m_vehicleState.lastHeartbeatMs = QDateTime::currentMSecsSinceEpoch();

    static uint32_t lastCustomMode = 0xFFFFFFFF;
    static uint8_t lastBaseMode = 0xFF;
    if (heartbeat.custom_mode != lastCustomMode || heartbeat.base_mode != lastBaseMode) {
        lastCustomMode = heartbeat.custom_mode;
        lastBaseMode = heartbeat.base_mode;
        qDebug() << "Heartbeat - Mode:" << getModeString() << "Arm:" << getArmStatusString();
    }

    emit heartbeatReceived();
    emit vehicleStateUpdated();
}

// 将电池与机载传感器状态保存为最近一帧快照；电压原始单位为毫伏。
void MavlinkManager::handleSysStatus(const mavlink_message_t &msg)
{
    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&msg, &sysStatus);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.sysStatus.voltageBattery = sysStatus.voltage_battery;
    m_vehicleState.sysStatus.currentBattery = sysStatus.current_battery;
    m_vehicleState.sysStatus.batteryRemaining = sysStatus.battery_remaining;
    m_vehicleState.sysStatus.dropRateComm = sysStatus.drop_rate_comm;
    m_vehicleState.sysStatus.errorsComm = sysStatus.errors_comm;
    m_vehicleState.sysStatus.sensorsPresent = sysStatus.onboard_control_sensors_present;
    m_vehicleState.sysStatus.sensorsEnabled = sysStatus.onboard_control_sensors_enabled;
    m_vehicleState.sysStatus.sensorsHealth = sysStatus.onboard_control_sensors_health;
    m_vehicleState.sysStatus.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    float voltage = sysStatus.voltage_battery / 1000.0f;
    emit batteryUpdated(voltage, sysStatus.battery_remaining);
    emit vehicleStateUpdated();
}

// GPS 原始纬经度按 1e7 缩放后发给界面，状态快照仍保留协议原值。
void MavlinkManager::handleGpsRawInt(const mavlink_message_t &msg)
{
    mavlink_gps_raw_int_t gps;
    mavlink_msg_gps_raw_int_decode(&msg, &gps);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.gps.fixType = gps.fix_type;
    m_vehicleState.gps.lat = gps.lat;
    m_vehicleState.gps.lon = gps.lon;
    m_vehicleState.gps.alt = gps.alt;
    m_vehicleState.gps.vel = gps.vel;
    m_vehicleState.gps.cog = gps.cog;
    m_vehicleState.gps.satellites = gps.satellites_visible;
    m_vehicleState.gps.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    emit gpsUpdated(gps.fix_type, gps.satellites_visible,
                    gps.lat / 1e7, gps.lon / 1e7);
    emit vehicleStateUpdated();
}

// MAVLink 姿态角使用弧度；界面信号改用角度。
// 连续偏航角在跨越 ±π 时消除跳变，供连续旋转控制使用。
void MavlinkManager::handleAttitude(const mavlink_message_t &msg)
{
    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&msg, &attitude);

    QMutexLocker locker(&m_stateMutex);

    const bool hadPreviousAttitude = m_vehicleState.attitude.lastUpdateMs != 0;
    const float prevYaw = m_vehicleState.attitude.yaw;

    m_vehicleState.attitude.roll = attitude.roll;
    m_vehicleState.attitude.pitch = attitude.pitch;
    m_vehicleState.attitude.yaw = attitude.yaw;
    m_vehicleState.attitude.rollspeed = attitude.rollspeed;
    m_vehicleState.attitude.pitchspeed = attitude.pitchspeed;
    m_vehicleState.attitude.yawspeed = attitude.yawspeed;
    m_vehicleState.attitude.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    // 累计偏航角：检测yaw跳变并累积。首帧以当前真实偏航角为起点。
    if (!hadPreviousAttitude) {
        m_vehicleState.attitude.yawCumulative = attitude.yaw;
    } else {
        float deltaYaw = attitude.yaw - prevYaw;
        while (deltaYaw > M_PI) deltaYaw -= 2.0f * M_PI;
        while (deltaYaw < -M_PI) deltaYaw += 2.0f * M_PI;
        m_vehicleState.attitude.yawCumulative += deltaYaw;
    }

    // 转换为角度
    float rollDeg = attitude.roll * 180.0f / M_PI;
    float pitchDeg = attitude.pitch * 180.0f / M_PI;
    float yawDeg = attitude.yaw * 180.0f / M_PI;

    float yawCumDeg = m_vehicleState.attitude.yawCumulative * 180.0f / M_PI;

    float rollspeedDeg = attitude.rollspeed * 180.0f / M_PI;
    float pitchspeedDeg = attitude.pitchspeed * 180.0f / M_PI;
    float yawspeedDeg = attitude.yawspeed * 180.0f / M_PI;

    emit attitudeUpdated(rollDeg, pitchDeg, yawDeg, rollspeedDeg, pitchspeedDeg, yawspeedDeg, yawCumDeg);
    emit vehicleStateUpdated();
}

void MavlinkManager::handleGlobalPositionInt(const mavlink_message_t &msg)
{
    mavlink_global_position_int_t pos;
    mavlink_msg_global_position_int_decode(&msg, &pos);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.globalPos.lat = pos.lat;
    m_vehicleState.globalPos.lon = pos.lon;
    m_vehicleState.globalPos.alt = pos.alt;
    m_vehicleState.globalPos.relativeAlt = pos.relative_alt;
    m_vehicleState.globalPos.hdg = pos.hdg;
    m_vehicleState.globalPos.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    emit vehicleStateUpdated();
}

void MavlinkManager::handleVfrHud(const mavlink_message_t &msg)
{
    mavlink_vfr_hud_t vfr;
    mavlink_msg_vfr_hud_decode(&msg, &vfr);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.vfrHud.airspeed = vfr.airspeed;
    m_vehicleState.vfrHud.groundspeed = vfr.groundspeed;
    m_vehicleState.vfrHud.heading = vfr.heading;
    m_vehicleState.vfrHud.throttle = vfr.throttle;
    m_vehicleState.vfrHud.alt = vfr.alt;
    m_vehicleState.vfrHud.climbRate = vfr.climb;
    m_vehicleState.vfrHud.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    emit vehicleStateUpdated();
}

void MavlinkManager::handleScaledPressure(const mavlink_message_t &msg)
{
    mavlink_scaled_pressure_t pressure;
    mavlink_msg_scaled_pressure_decode(&msg, &pressure);

    updateDepthFromPressure(pressure.press_abs, pressure.temperature);
}

void MavlinkManager::handleScaledPressure2(const mavlink_message_t &msg)
{
    mavlink_scaled_pressure2_t pressure;
    mavlink_msg_scaled_pressure2_decode(&msg, &pressure);

    updateDepthFromPressure(pressure.press_abs, pressure.temperature);
}

void MavlinkManager::handleScaledPressure3(const mavlink_message_t &msg)
{
    mavlink_scaled_pressure3_t pressure;
    mavlink_msg_scaled_pressure3_decode(&msg, &pressure);

    updateDepthFromPressure(pressure.press_abs, pressure.temperature);
}

// 将有效绝对压力换算成非负深度，并保存温度与采样时间。
// 压力基准取标准大气压；此处未进行现场水面压力校准。
void MavlinkManager::updateDepthFromPressure(float pressureHpa, int16_t temperatureCentiDeg)
{
    // 无效/未初始化压力值不覆盖最近一帧有效的深度遥测。
    if (!std::isfinite(pressureHpa) || pressureHpa <= 0.0f) {
        return;
    }

    QMutexLocker locker(&m_stateMutex);

    // 计算深度 (压力转换为深度，约 1 hPa = 1 cm 水深)
    // 标准大气压约 1013.25 hPa
    m_vehicleState.depth.pressure = pressureHpa;
    m_vehicleState.depth.depth = qMax(0.0f, (pressureHpa - 1013.25f) * 0.01f);
    m_vehicleState.depth.temperature = temperatureCentiDeg / 100.0f;
    m_vehicleState.depth.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    emit depthUpdated(m_vehicleState.depth.depth);
    emit vehicleStateUpdated();
}

// 心跳超时后清除当前目标与在线标记，允许后续真实心跳重新发现飞控。
void MavlinkManager::resetVehicleConnection()
{
    QMutexLocker locker(&m_stateMutex);
    m_targetDiscovered = false;
    m_targetSystem = 1;
    m_targetComponent = 1;
    m_vehicleState.lastHeartbeatMs = 0;
    m_vehicleState.baseMode = 0;
    m_vehicleState.customMode = 0;
    m_vehicleState.systemStatus = MAV_STATE_UNINIT;
}

void MavlinkManager::handleServoOutputRaw(const mavlink_message_t &msg)
{
    mavlink_servo_output_raw_t servo;
    mavlink_msg_servo_output_raw_decode(&msg, &servo);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.servo.timeUsec = servo.time_usec;
    m_vehicleState.servo.port = servo.port;
    m_vehicleState.servo.raw[0] = servo.servo1_raw;
    m_vehicleState.servo.raw[1] = servo.servo2_raw;
    m_vehicleState.servo.raw[2] = servo.servo3_raw;
    m_vehicleState.servo.raw[3] = servo.servo4_raw;
    m_vehicleState.servo.raw[4] = servo.servo5_raw;
    m_vehicleState.servo.raw[5] = servo.servo6_raw;
    m_vehicleState.servo.raw[6] = servo.servo7_raw;
    m_vehicleState.servo.raw[7] = servo.servo8_raw;
    m_vehicleState.servo.raw[8] = servo.servo9_raw;
    m_vehicleState.servo.raw[9] = servo.servo10_raw;
    m_vehicleState.servo.raw[10] = servo.servo11_raw;
    m_vehicleState.servo.raw[11] = servo.servo12_raw;
    m_vehicleState.servo.raw[12] = servo.servo13_raw;
    m_vehicleState.servo.raw[13] = servo.servo14_raw;
    m_vehicleState.servo.raw[14] = servo.servo15_raw;
    m_vehicleState.servo.raw[15] = servo.servo16_raw;
    m_vehicleState.servo.count = 16;
    m_vehicleState.servo.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();
    // 更新云台位置状态（从舵机反馈中读取）
    if (servo.port == 1)
    {  // 主舵机输出端口
        m_frontPanPosition = servo.servo13_raw;
        m_frontTiltPosition = servo.servo14_raw;
        m_rearPanPosition = servo.servo15_raw;
        m_rearTiltPosition = servo.servo16_raw;

        emit frontGimbalUpdated(m_frontPanPosition, m_frontTiltPosition);
        emit rearGimbalUpdated(m_rearPanPosition, m_rearTiltPosition);
    }
    emit servoUpdated();
    emit vehicleStateUpdated();
}

void MavlinkManager::handleRcChannels(const mavlink_message_t &msg)
{
    mavlink_rc_channels_t rc;
    mavlink_msg_rc_channels_decode(&msg, &rc);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.rcChannels.channels[0] = rc.chan1_raw;
    m_vehicleState.rcChannels.channels[1] = rc.chan2_raw;
    m_vehicleState.rcChannels.channels[2] = rc.chan3_raw;
    m_vehicleState.rcChannels.channels[3] = rc.chan4_raw;
    m_vehicleState.rcChannels.channels[4] = rc.chan5_raw;
    m_vehicleState.rcChannels.channels[5] = rc.chan6_raw;
    m_vehicleState.rcChannels.channels[6] = rc.chan7_raw;
    m_vehicleState.rcChannels.channels[7] = rc.chan8_raw;
    m_vehicleState.rcChannels.channels[8] = rc.chan9_raw;
    m_vehicleState.rcChannels.channels[9] = rc.chan10_raw;
    m_vehicleState.rcChannels.channels[10] = rc.chan11_raw;
    m_vehicleState.rcChannels.channels[11] = rc.chan12_raw;
    m_vehicleState.rcChannels.channels[12] = rc.chan13_raw;
    m_vehicleState.rcChannels.channels[13] = rc.chan14_raw;
    m_vehicleState.rcChannels.channels[14] = rc.chan15_raw;
    m_vehicleState.rcChannels.channels[15] = rc.chan16_raw;
    m_vehicleState.rcChannels.channels[16] = rc.chan17_raw;
    m_vehicleState.rcChannels.channels[17] = rc.chan18_raw;
    m_vehicleState.rcChannels.rssi = rc.rssi;
    m_vehicleState.rcChannels.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    emit rcChannelsUpdated();
}

void MavlinkManager::handleRcChannelsRaw(const mavlink_message_t &msg)
{
    mavlink_rc_channels_raw_t rc;
    mavlink_msg_rc_channels_raw_decode(&msg, &rc);

    QMutexLocker locker(&m_stateMutex);

    m_vehicleState.rcChannels.channels[0] = rc.chan1_raw;
    m_vehicleState.rcChannels.channels[1] = rc.chan2_raw;
    m_vehicleState.rcChannels.channels[2] = rc.chan3_raw;
    m_vehicleState.rcChannels.channels[3] = rc.chan4_raw;
    m_vehicleState.rcChannels.channels[4] = rc.chan5_raw;
    m_vehicleState.rcChannels.channels[5] = rc.chan6_raw;
    m_vehicleState.rcChannels.channels[6] = rc.chan7_raw;
    m_vehicleState.rcChannels.channels[7] = rc.chan8_raw;
    m_vehicleState.rcChannels.rssi = rc.rssi;
    m_vehicleState.rcChannels.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    emit rcChannelsUpdated();
}

void MavlinkManager::handleStatustext(const mavlink_message_t &msg)
{
    mavlink_statustext_t statustext;
    mavlink_msg_statustext_decode(&msg, &statustext);

    // 确保字符串以 null 结尾
    char text[51];
    strncpy(text, statustext.text, 50);
    text[50] = '\0';

    emit statusTextReceived(QString::fromUtf8(text), statustext.severity);

    qDebug() << "StatusText [" << statustext.severity << "]:" << text;
}

void MavlinkManager::handleCommandAck(const mavlink_message_t &msg)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&msg, &ack);

    emit commandAckReceived(ack.command, ack.result);

    qDebug() << "Command ACK - CMD:" << ack.command << "Result:" << ack.result;

    {
        QMutexLocker locker(&m_cmdLogMutex);
        if (m_cmdLogStream) {
            *m_cmdLogStream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
                            << ",RECV_ACK," << mavlinkCmdName(ack.command)
                            << "," << ack.command
                            << "," << mavlinkAckResultString(ack.result)
                            << "," << (int)ack.result
                            << "," << (int)ack.progress
                            << "," << (int)ack.result_param2
                            << "\n";
            m_cmdLogStream->flush();
        }
    }
}

void MavlinkManager::handleParamValue(const mavlink_message_t &msg)
{
    mavlink_param_value_t param;
    mavlink_msg_param_value_decode(&msg, &param);

    char paramId[17];
    strncpy(paramId, param.param_id, 16);
    paramId[16] = '\0';

    qDebug() << "Param:" << paramId << "=" << param.param_value;

    QString paramName(paramId);
    emit paramValueReceived(paramName, param.param_value);

    if (paramName == "FRAME_CONFIG") {
        int newFrameConfig = qRound(param.param_value);
        if (newFrameConfig != m_frameConfig) {
            m_frameConfig = newFrameConfig;
            emit frameConfigChanged(m_frameConfig);
            qDebug() << "FRAME_CONFIG changed to:" << m_frameConfig;
        }
    } else if (paramName == "MOT_NET_MODE") {
        int newMode = qRound(param.param_value);
        if (newMode != m_netCleanerMode) {
            m_netCleanerMode = newMode;
            emit netCleanerModeChanged(m_netCleanerMode);
            qDebug() << "MOT_NET_MODE changed to:" << m_netCleanerMode;
        }
    }
}

void MavlinkManager::onHeartbeatTimer()
{
    sendHeartbeat();
}

// 以地面站身份发送心跳，让飞控识别本程序的控制端身份。
void MavlinkManager::sendHeartbeat()
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(
        m_systemId,
        m_componentId,
        &msg,
        MAV_TYPE_GCS,
        MAV_AUTOPILOT_INVALID,
        MAV_MODE_MANUAL_ARMED,
        0,
        MAV_STATE_ACTIVE
    );

    sendMavlinkMessage(msg);
}

// 通过 COMMAND_LONG 下发解锁或上锁命令；force 映射到第二参数。
void MavlinkManager::sendArmCommand(bool arm, bool force)
{
    mavlink_message_t msg;
    float armParam = arm ? 1.0f : 0.0f;
    float forceParam = force ? 1.0f : 0.0f;

    mavlink_msg_command_long_pack(
        m_systemId,
        m_componentId,
        &msg,
        m_targetSystem,
        m_targetComponent,
        MAV_CMD_COMPONENT_ARM_DISARM,
        1,  // 需要确认
        armParam,
        forceParam,
        0, 0, 0, 0, 0
    );

    sendMavlinkMessage(msg);
    qDebug() << "Sent arm command:" << (arm ? "ARM" : "DISARM") << "force:" << force;
}

void MavlinkManager::sendSetModeCommand(uint8_t baseMode, uint32_t customMode)
{
    sendCommandLong(MAV_CMD_DO_SET_MODE, baseMode, customMode);
    qDebug() << "Sent set mode - base:" << baseMode << "custom:" << customMode;
}

// MANUAL_CONTROL字段 (与QGC Vehicle.cc一致):
// x = pitch, y = roll, z = throttle, r = yaw
// s = pitchExtension, t = rollExtension
// ArduSub maneuver模式: x→forward, y→lateral, z→throttle, r→yaw, s→pitch, t→roll
void MavlinkManager::sendManualControl(int16_t x, int16_t y, int16_t z, int16_t r, uint16_t buttons,
                                       int16_t s, int16_t t, bool enableExtensions)
{
    mavlink_message_t msg;
    memset(&msg, 0, sizeof(msg));

    uint8_t extensions = 0;
    if (enableExtensions) {
        extensions |= 0x01;
        extensions |= 0x02;
    }

    mavlink_msg_manual_control_pack(
        m_systemId,
        m_componentId,
        &msg,
        m_targetSystem,
        x, y, z, r,
        buttons,
        0, extensions,
        s, t,
        0, 0, 0, 0, 0, 0
    );

    sendMavlinkMessage(msg);
}

void MavlinkManager::sendRcChannelsOverride(uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4,
                                              uint16_t ch5, uint16_t ch6, uint16_t ch7, uint16_t ch8)
{
    mavlink_message_t msg;
    mavlink_msg_rc_channels_override_pack(
        m_systemId, m_componentId, &msg,
        m_targetSystem, m_targetComponent,
        ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    );
    sendMavlinkMessage(msg);
}

// 在机体 NED 坐标系下发送速度与偏航角速度目标；掩码忽略位置、加速度和绝对航向。
void MavlinkManager::sendMovementCommand(float forward, float lateral, float vertical, float yawRate)
{
    mavlink_message_t msg;

    // 类型掩码：忽略位置和加速度，只使用速度
    uint16_t typeMask =
        POSITION_TARGET_TYPEMASK_X_IGNORE |
        POSITION_TARGET_TYPEMASK_Y_IGNORE |
        POSITION_TARGET_TYPEMASK_Z_IGNORE |
        POSITION_TARGET_TYPEMASK_AX_IGNORE |
        POSITION_TARGET_TYPEMASK_AY_IGNORE |
        POSITION_TARGET_TYPEMASK_AZ_IGNORE |
        POSITION_TARGET_TYPEMASK_YAW_IGNORE;

    mavlink_msg_set_position_target_local_ned_pack(
        m_systemId,
        m_componentId,
        &msg,
        0,  // time_boot_ms
        m_targetSystem,
        m_targetComponent,
        MAV_FRAME_BODY_NED,
        typeMask,
        0, 0, 0,           // x, y, z
        forward, lateral, vertical,  // vx, vy, vz
        0, 0, 0,           // afx, afy, afz
        0,                 // yaw
        yawRate            // yaw_rate
    );

    sendMavlinkMessage(msg);
}

void MavlinkManager::sendSetServoCommand(uint8_t channel, uint16_t pwm)
{
    mavlink_message_t msg;

    mavlink_msg_command_long_pack(
        m_systemId,
        m_componentId,
        &msg,
        m_targetSystem,
        m_targetComponent,
        MAV_CMD_DO_SET_SERVO,
        0,  // 不需要确认
        (float)channel,
        (float)pwm,
        0, 0, 0, 0, 0
    );

    sendMavlinkMessage(msg);
    DEBUG << "Set servo" << channel << "to" << pwm;
}

void MavlinkManager::sendRequestDataStream(uint8_t streamId, uint16_t rate, uint8_t startStop)
{
    mavlink_message_t msg;

    mavlink_msg_request_data_stream_pack(
        m_systemId,
        m_componentId,
        &msg,
        m_targetSystem,
        m_targetComponent,
        streamId,
        rate,
        startStop
    );

    sendMavlinkMessage(msg);
    qDebug() << "Request data stream" << streamId << "rate:" << rate;
}

void MavlinkManager::sendSetMessageInterval(uint16_t msgId, uint32_t intervalUs)
{
    mavlink_message_t msg;

    mavlink_msg_command_long_pack(
        m_systemId,
        m_componentId,
        &msg,
        m_targetSystem,
        m_targetComponent,
        MAV_CMD_SET_MESSAGE_INTERVAL,
        1,  // 需要确认
        (float)msgId,
        (float)intervalUs,
        0, 0, 0, 0, 0
    );

    sendMavlinkMessage(msg);
}

void MavlinkManager::sendCommandLong(uint16_t command, float param1, float param2,
                                      float param3, float param4, float param5,
                                      float param6, float param7)
{
    mavlink_message_t msg;

    mavlink_msg_command_long_pack(
        m_systemId,
        m_componentId,
        &msg,
        m_targetSystem,
        m_targetComponent,
        command,
        1,  // 需要确认
        param1, param2, param3, param4, param5, param6, param7
    );

    sendMavlinkMessage(msg);
    qDebug() << "Sent command long:" << command;
}

void MavlinkManager::sendParamSet(const char *paramId, float value, uint8_t paramType)
{
    mavlink_message_t msg;
    mavlink_msg_param_set_pack(
        m_systemId, m_componentId, &msg,
        m_targetSystem, m_targetComponent,
        paramId, value, paramType
    );
    sendMavlinkMessage(msg);
    qDebug() << "Sent PARAM_SET:" << paramId << "=" << value;
}

void MavlinkManager::sendParamRequestRead(const char *paramId)
{
    mavlink_message_t msg;
    mavlink_msg_param_request_read_pack(
        m_systemId, m_componentId, &msg,
        m_targetSystem, m_targetComponent,
        paramId, -1
    );
    sendMavlinkMessage(msg);
    qDebug() << "Sent PARAM_REQUEST_READ:" << paramId;
}

void MavlinkManager::setFrameConfig(int frameConfig)
{
    if (isArmed()) {
        qDebug() << "Cannot change FRAME_CONFIG while armed!";
        return;
    }
    m_frameConfig = frameConfig;
    sendParamSet("FRAME_CONFIG", (float)frameConfig, MAV_PARAM_TYPE_REAL32);

    int ahrsOrientation = 0;
    if (frameConfig == 8) {
        ahrsOrientation = 24;  // PITCH_90: Cleaner 下水时FC箭头朝下
    }
    sendParamSet("AHRS_ORIENTATION", (float)ahrsOrientation, MAV_PARAM_TYPE_REAL32);

    sendParamRequestRead("FRAME_CONFIG");
    sendParamRequestRead("AHRS_ORIENTATION");
    qDebug() << "Set FRAME_CONFIG to:" << frameConfig << "AHRS_ORIENTATION to:" << ahrsOrientation;
}

// Runtime switch using MOT_NET_MODE (no reboot or disarm required!)
void MavlinkManager::setNetCleanerMode(int mode)
{
    if (mode < 0 || mode > 1) return;
    m_netCleanerMode = mode;
    sendParamSet("MOT_NET_MODE", (float)mode, MAV_PARAM_TYPE_INT8);
    sendParamRequestRead("MOT_NET_MODE");
    qDebug() << "Set MOT_NET_MODE to:" << mode << (mode == 0 ? "(Diving)" : "(Cleaning)");
}

// One-key toggle via MAV_CMD_DO_SET_MODE (custom_mode 100=Dive, 101=Clean, 102=Toggle)
void MavlinkManager::sendDoSetModeForNetCleaner(int mode)
{
    // mode: 0 = Dive, 1 = Clean, 2 = Toggle
    uint32_t customMode = 100 + mode;  // 100=Dive, 101=Clean, 102=Toggle
    sendCommandLong(MAV_CMD_DO_SET_MODE,
                    (float)MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,  // param1: base_mode
                    (float)customMode,                          // param2: custom_mode
                    0, 0, 0, 0, 0);
    qDebug() << "Sent DO_SET_MODE custom_mode=" << customMode;
}

// One-key cleaning: send COMMAND_LONG ID=31000 to firmware state machine
void MavlinkManager::sendCleanerCommand(int mode)
{
    // mode=9: start transition (dive -> work)
    // mode=8: return to dive (work -> dive)
    sendCommandLong(31000, (float)mode, 0, 0, 0, 0, 0, 0);
    qDebug() << "Sent cleaner command: mode=" << mode << (mode == 9 ? "(->WORK)" : "(->DIVE)");
}

void MavlinkManager::sendMavlinkMessage(mavlink_message_t &msg)
{
    logCommandSend(msg);

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);

    QByteArray data(reinterpret_cast<const char*>(buffer), len);
    emit sendData(data);
}

void MavlinkManager::setControlMode(ControlMode mode)
{
    m_controlMode = mode;
    qDebug() << "Control mode changed to:" << mode;
}

QString MavlinkManager::getModeString() const
{
    uint32_t customMode = m_vehicleState.customMode;
    bool armed = (m_vehicleState.baseMode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;

    QString modeStr;
    switch (customMode) {
    case SUB_MODE_STABILIZE: modeStr = "STABILIZE"; break;
    case SUB_MODE_ACRO: modeStr = "ACRO"; break;
    case SUB_MODE_ALT_HOLD: modeStr = "ALT_HOLD"; break;
    case SUB_MODE_AUTO: modeStr = "AUTO"; break;
    case SUB_MODE_GUIDED: modeStr = "GUIDED"; break;
    case SUB_MODE_CIRCLE: modeStr = "CIRCLE"; break;
    case SUB_MODE_SURFACE: modeStr = "SURFACE"; break;
    case SUB_MODE_POSHOLD: modeStr = "POSHOLD"; break;
    case SUB_MODE_MANUAL: modeStr = "MANUAL"; break;
    case SUB_MODE_MOTOR_DETECT: modeStr = "MOTOR_DETECT"; break;
    default: modeStr = QString("MODE_%1").arg(customMode); break;
    }

    return armed ? modeStr : modeStr + " (DISARMED)";
}

QString MavlinkManager::getArmStatusString() const
{
    bool armed = (m_vehicleState.baseMode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
    return armed ? "ARMED" : "DISARMED";
}

QString MavlinkManager::getFixTypeString() const
{
    switch (m_vehicleState.gps.fixType) {
    case 0:
    case 1: return "NO FIX";
    case 2: return "2D FIX";
    case 3: return "3D FIX";
    case 4: return "DGPS";
    case 5: return "RTK FLOAT";
    case 6: return "RTK FIXED";
    default: return "UNKNOWN";
    }
}

QString MavlinkManager::getSystemStatusString() const
{
    switch (m_vehicleState.systemStatus) {
    case MAV_STATE_UNINIT: return "UNINIT";
    case MAV_STATE_BOOT: return "BOOT";
    case MAV_STATE_CALIBRATING: return "CALIBRATING";
    case MAV_STATE_STANDBY: return "STANDBY";
    case MAV_STATE_ACTIVE: return "ACTIVE";
    case MAV_STATE_CRITICAL: return "CRITICAL";
    case MAV_STATE_EMERGENCY: return "EMERGENCY";
    case MAV_STATE_POWEROFF: return "POWEROFF";
    case MAV_STATE_FLIGHT_TERMINATION: return "FLIGHT_TERM";
    default: return QString("STATE_%1").arg(m_vehicleState.systemStatus);
    }
}

// 在 mavlink_manager.cpp 中添加以下实现

// 灯控制常量定义
#define PWM_CHANNEL_FRONT_LIGHT  9
#define PWM_CHANNEL_FRONT_LIGHT2 10
#define PWM_CHANNEL_REAR_LIGHT   11
#define PWM_CHANNEL_REAR_LIGHT2  12
#define PWM_LIGHT_OFF            1000
#define PWM_LIGHT_ON             2000
#define PWM_LIGHT_MIN            1120
#define PWM_LIGHT_MAX            2000

void MavlinkManager::setFrontLight(uint16_t brightness)
{
    if (brightness < PWM_LIGHT_MIN) brightness = PWM_LIGHT_MIN;
    if (brightness > PWM_LIGHT_MAX) brightness = PWM_LIGHT_MAX;

    sendSetServoCommand(PWM_CHANNEL_FRONT_LIGHT, brightness);
    sendSetServoCommand(PWM_CHANNEL_FRONT_LIGHT2, brightness);
}

void MavlinkManager::setRearLight(uint16_t brightness)
{
    if (brightness < PWM_LIGHT_MIN) brightness = PWM_LIGHT_MIN;
    if (brightness > PWM_LIGHT_MAX) brightness = PWM_LIGHT_MAX;

    sendSetServoCommand(PWM_CHANNEL_REAR_LIGHT, brightness);
    sendSetServoCommand(PWM_CHANNEL_REAR_LIGHT2, brightness);
}

void MavlinkManager::toggleFrontLight()
{
    QMutexLocker locker(&m_stateMutex);

    // 从舵机状态中获取当前前灯PWM值
    uint16_t currentPwm = PWM_LIGHT_OFF;
    if (PWM_CHANNEL_FRONT_LIGHT - 1 < m_vehicleState.servo.count) {
        currentPwm = m_vehicleState.servo.raw[PWM_CHANNEL_FRONT_LIGHT - 1];
    }

    locker.unlock();

    // 切换开关状态（简单二值切换）
    if (currentPwm > PWM_LIGHT_MIN + (PWM_LIGHT_MAX - PWM_LIGHT_MIN) / 2) {
        setFrontLight(PWM_LIGHT_OFF);
    } else {
        setFrontLight(PWM_LIGHT_ON);
    }
}

void MavlinkManager::toggleRearLight()
{
    QMutexLocker locker(&m_stateMutex);

    // 从舵机状态中获取当前后灯PWM值
    uint16_t currentPwm = PWM_LIGHT_OFF;
    if (PWM_CHANNEL_REAR_LIGHT - 1 < m_vehicleState.servo.count) {
        currentPwm = m_vehicleState.servo.raw[PWM_CHANNEL_REAR_LIGHT - 1];
    }

    locker.unlock();

    // 切换开关状态
    if (currentPwm > PWM_LIGHT_MIN + (PWM_LIGHT_MAX - PWM_LIGHT_MIN) / 2) {
        setRearLight(PWM_LIGHT_OFF);
    } else {
        setRearLight(PWM_LIGHT_ON);
    }
}

void MavlinkManager::setBothLights(uint16_t frontBrightness, uint16_t rearBrightness)
{
    setFrontLight(frontBrightness);
    setRearLight(rearBrightness);
}


void MavlinkManager::setFrontLightPercent(uint8_t percent)
{
    DEBUG<<percent;
    if (percent > 100) percent = 100;
    uint16_t pwm = PWM_LIGHT_MIN + (uint16_t)((PWM_LIGHT_MAX - PWM_LIGHT_MIN) * percent / 100.0f);
    DEBUG<<pwm;
    setFrontLight(pwm);
}

void MavlinkManager::setRearLightPercent(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint16_t pwm = PWM_LIGHT_MIN + (uint16_t)((PWM_LIGHT_MAX - PWM_LIGHT_MIN) * percent / 100.0f);
    setRearLight(pwm);
}

// ========== 前摄像头云台控制 ==========

void MavlinkManager::setFrontCameraPan(uint16_t pwm)
{
    // 限制PWM范围
    if (pwm < PWM_GIMBAL_PAN_MIN) pwm = PWM_GIMBAL_PAN_MIN;
    if (pwm > PWM_GIMBAL_PAN_MAX) pwm = PWM_GIMBAL_PAN_MAX;

    m_frontPanPosition = pwm;
    sendSetServoCommand(PWM_CHANNEL_FRONT_PAN, pwm);

    qDebug() << "前摄像头云台左右:" << pwm;
    emit frontGimbalUpdated(m_frontPanPosition, m_frontTiltPosition);
}

void MavlinkManager::setFrontCameraTilt(uint16_t pwm)
{
    // 限制PWM范围
    if (pwm < PWM_GIMBAL_TILT_MIN) pwm = PWM_GIMBAL_TILT_MIN;
    if (pwm > PWM_GIMBAL_TILT_MAX) pwm = PWM_GIMBAL_TILT_MAX;

    m_frontTiltPosition = pwm;
    sendSetServoCommand(PWM_CHANNEL_FRONT_TILT, pwm);

    qDebug() << "前摄像头云台上下:" << pwm;
    emit frontGimbalUpdated(m_frontPanPosition, m_frontTiltPosition);
}

void MavlinkManager::setFrontCameraGimbal(uint16_t pan, uint16_t tilt)
{
    setFrontCameraPan(pan);
    setFrontCameraTilt(tilt);
}

void MavlinkManager::centerFrontCameraGimbal()
{
    setFrontCameraGimbal(PWM_GIMBAL_CENTER, PWM_GIMBAL_CENTER);
    qDebug() << "前摄像头云台已回中";
}

void MavlinkManager::stopFrontCameraGimbal()
{
    // 发送停止命令（通常发送当前位置值可以停止运动）
    sendSetServoCommand(PWM_CHANNEL_FRONT_PAN, m_frontPanPosition);
    sendSetServoCommand(PWM_CHANNEL_FRONT_TILT, m_frontTiltPosition);
    qDebug() << "前摄像头云台已停止";
}

// ========== 后摄像头云台控制（预留） ==========

void MavlinkManager::setRearCameraPan(uint16_t pwm)
{
    if (pwm < PWM_GIMBAL_PAN_MIN) pwm = PWM_GIMBAL_PAN_MIN;
    if (pwm > PWM_GIMBAL_PAN_MAX) pwm = PWM_GIMBAL_PAN_MAX;

    m_rearPanPosition = pwm;
    sendSetServoCommand(PWM_CHANNEL_REAR_PAN, pwm);

    qDebug() << "后摄像头云台左右:" << pwm;
    emit rearGimbalUpdated(m_rearPanPosition, m_rearTiltPosition);
}

void MavlinkManager::setRearCameraTilt(uint16_t pwm)
{
    if (pwm < PWM_GIMBAL_TILT_MIN) pwm = PWM_GIMBAL_TILT_MIN;
    if (pwm > PWM_GIMBAL_TILT_MAX) pwm = PWM_GIMBAL_TILT_MAX;

    m_rearTiltPosition = pwm;
    sendSetServoCommand(PWM_CHANNEL_REAR_TILT, pwm);

    qDebug() << "后摄像头云台上下:" << pwm;
    emit rearGimbalUpdated(m_rearPanPosition, m_rearTiltPosition);
}

void MavlinkManager::setRearCameraGimbal(uint16_t pan, uint16_t tilt)
{
    setRearCameraPan(pan);
    setRearCameraTilt(tilt);
}

void MavlinkManager::centerRearCameraGimbal()
{
    setRearCameraGimbal(PWM_GIMBAL_CENTER, PWM_GIMBAL_CENTER);
    qDebug() << "后摄像头云台已回中";
}

void MavlinkManager::stopRearCameraGimbal()
{
    sendSetServoCommand(PWM_CHANNEL_REAR_PAN, m_rearPanPosition);
    sendSetServoCommand(PWM_CHANNEL_REAR_TILT, m_rearTiltPosition);
    qDebug() << "后摄像头云台已停止";
}

// ========== 增量控制 ==========

void MavlinkManager::moveFrontPan(int16_t delta)
{
    int32_t newPosition = static_cast<int32_t>(m_frontPanPosition) + delta;
    if (newPosition < PWM_GIMBAL_PAN_MIN) newPosition = PWM_GIMBAL_PAN_MIN;
    if (newPosition > PWM_GIMBAL_PAN_MAX) newPosition = PWM_GIMBAL_PAN_MAX;

    setFrontCameraPan(static_cast<uint16_t>(newPosition));
}

void MavlinkManager::moveFrontTilt(int16_t delta)
{
    int32_t newPosition = static_cast<int32_t>(m_frontTiltPosition) + delta;
    if (newPosition < PWM_GIMBAL_TILT_MIN) newPosition = PWM_GIMBAL_TILT_MIN;
    if (newPosition > PWM_GIMBAL_TILT_MAX) newPosition = PWM_GIMBAL_TILT_MAX;

    setFrontCameraTilt(static_cast<uint16_t>(newPosition));
}

void MavlinkManager::moveRearPan(int16_t delta)
{
    int32_t newPosition = static_cast<int32_t>(m_rearPanPosition) + delta;
    if (newPosition < PWM_GIMBAL_PAN_MIN) newPosition = PWM_GIMBAL_PAN_MIN;
    if (newPosition > PWM_GIMBAL_PAN_MAX) newPosition = PWM_GIMBAL_PAN_MAX;

    setRearCameraPan(static_cast<uint16_t>(newPosition));
}

void MavlinkManager::moveRearTilt(int16_t delta)
{
    int32_t newPosition = static_cast<int32_t>(m_rearTiltPosition) + delta;
    if (newPosition < PWM_GIMBAL_TILT_MIN) newPosition = PWM_GIMBAL_TILT_MIN;
    if (newPosition > PWM_GIMBAL_TILT_MAX) newPosition = PWM_GIMBAL_TILT_MAX;

    setRearCameraTilt(static_cast<uint16_t>(newPosition));
}

// ========== 姿态旋转控制 ==========

void MavlinkManager::startRotationToClean()
{
    if (isRotating()) {
        qDebug() << "旋转正在进行中，忽略请求";
        return;
    }
    if (m_machineOrientation == ORIENTATION_CLEAN) {
        qDebug() << "已在清洗姿态，无需旋转";
        return;
    }

    // 切换到MANUAL模式以允许大角度俯仰
    sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 19);
    qDebug() << "已切换到MANUAL模式";

    m_rotationStartYaw = m_vehicleState.attitude.pitch;
    // 抬头旋转90度至清洗姿态: 目标pitch = 起始pitch + π/2
    m_rotationTargetYaw = m_rotationStartYaw + ROTATION_TARGET_ANGLE;
    m_rotationState = ROTATION_TO_CLEAN;
    m_rotationStartTimeMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());

    m_rotationControlTimer->start();
    emit rotationStateChanged(m_rotationState);
    qDebug() << "开始旋转至清洗姿态: 起始pitch=" << qRadiansToDegrees(m_rotationStartYaw)
             << "度, 目标pitch=" << qRadiansToDegrees(m_rotationTargetYaw) << "度";
}

void MavlinkManager::startRotationToDive()
{
    if (isRotating()) {
        qDebug() << "旋转正在进行中，忽略请求";
        return;
    }
    if (m_machineOrientation == ORIENTATION_DIVE) {
        qDebug() << "已在下潜姿态，无需旋转";
        return;
    }

    sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 19);
    qDebug() << "已切换到MANUAL模式";

    m_rotationStartYaw = m_vehicleState.attitude.pitch;
    // 低头旋转90度至下潜姿态: 目标pitch = 起始pitch - π/2
    m_rotationTargetYaw = m_rotationStartYaw - ROTATION_TARGET_ANGLE;
    m_rotationState = ROTATION_TO_DIVE;
    m_rotationStartTimeMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());

    m_rotationControlTimer->start();
    emit rotationStateChanged(m_rotationState);
    qDebug() << "开始旋转至下潜姿态: 起始pitch=" << qRadiansToDegrees(m_rotationStartYaw)
             << "度, 目标yaw=" << qRadiansToDegrees(m_rotationTargetYaw) << "度";
}

void MavlinkManager::abortRotation()
{
    if (!isRotating()) return;

    m_rotationControlTimer->stop();
    m_rotationState = ROTATION_ABORTED;
    emit rotationStateChanged(m_rotationState);
    qDebug() << "旋转已中止";

    // 发送停止指令
    sendManualControl(0, 0, 500, 0, 0, 0, 0, true);

    // 重置为中止状态
    m_rotationState = ROTATION_IDLE;
    emit rotationStateChanged(m_rotationState);
}

void MavlinkManager::onRotationControlTimer()
{
    if (!isRotating()) {
        m_rotationControlTimer->stop();
        return;
    }

    // 检查超时
    uint32_t elapsedMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()) - m_rotationStartTimeMs;
    if (elapsedMs > ROTATION_TIMEOUT_MS) {
        qDebug() << "旋转超时，中止";
        m_rotationControlTimer->stop();
        m_rotationState = ROTATION_ABORTED;
        emit rotationStateChanged(m_rotationState);
        sendManualControl(0, 0, 500, 0, 0, 0, 0, true);
        m_rotationState = ROTATION_IDLE;
        emit rotationStateChanged(m_rotationState);
        return;
    }

    // 计算当前pitch与目标pitch的角度差
    float currentPitch = m_vehicleState.attitude.pitch;
    float pitchError = m_rotationTargetYaw - currentPitch;

    // 归一化到 [-π, π]
    while (pitchError > M_PI) pitchError -= 2.0f * M_PI;
    while (pitchError < -M_PI) pitchError += 2.0f * M_PI;

    // 检查是否到达目标角度
    if (fabsf(pitchError) < ROTATION_COMPLETE_THRESHOLD) {
        m_rotationControlTimer->stop();

        // 发送停止指令
        sendManualControl(0, 0, 500, 0, 0, 0, 0, true);

        // 更新姿态状态
        if (m_rotationState == ROTATION_TO_CLEAN) {
            m_machineOrientation = ORIENTATION_CLEAN;
            emit machineOrientationChanged(m_machineOrientation);
            qDebug() << "旋转完成: 已到达清洗姿态";
            // 固件支持运行时切换，无需DISARM→ARM
            setNetCleanerMode(1);  // MOT_NET_MODE=1 (Cleaning)
            qDebug() << "清洗模式切换完成（运行时切换，无需重启）";
        } else if (m_rotationState == ROTATION_TO_DIVE) {
            m_machineOrientation = ORIENTATION_DIVE;
            emit machineOrientationChanged(m_machineOrientation);
            qDebug() << "旋转完成: 已到达下潜姿态";
            // 固件支持运行时切换，无需DISARM→ARM
            setNetCleanerMode(0);  // MOT_NET_MODE=0 (Diving)
            qDebug() << "下水模式切换完成（运行时切换，无需重启）";
        }

        m_rotationState = ROTATION_COMPLETED;
        emit rotationStateChanged(m_rotationState);

        // 重置为空闲
        m_rotationState = ROTATION_IDLE;
        emit rotationStateChanged(m_rotationState);
        return;
    }

    // 旋转指令: pitch通过s字段传递(启用扩展), maneuver模式下s→pitch
    int16_t pitchCommand = 0;
    if (m_rotationState == ROTATION_TO_CLEAN) {
        pitchCommand = ROTATION_YAW_RATE;
    } else if (m_rotationState == ROTATION_TO_DIVE) {
        pitchCommand = -ROTATION_YAW_RATE;
    }

    float speedFactor = fabsf(pitchError) / ROTATION_TARGET_ANGLE;
    if (speedFactor < 0.3f) speedFactor = 0.3f;
    pitchCommand = static_cast<int16_t>(pitchCommand * speedFactor);

    sendManualControl(0, 0, 500, 0, 0, pitchCommand, 0, true);
}

// ========== GUIDED模式一键清洗流程 ==========

void MavlinkManager::eulerToQuaternion(float roll, float pitch, float yaw, float q[4])
{
    float cr = cosf(roll / 2.0f);
    float sr = sinf(roll / 2.0f);
    float cp = cosf(pitch / 2.0f);
    float sp = sinf(pitch / 2.0f);
    float cy = cosf(yaw / 2.0f);
    float sy = sinf(yaw / 2.0f);

    q[0] = cr * cp * cy + sr * sp * sy;
    q[1] = sr * cp * cy - cr * sp * sy;
    q[2] = cr * sp * cy + sr * cp * sy;
    q[3] = cr * cp * sy - sr * sp * cy;
}

void MavlinkManager::sendSetAttitudeTarget(float q[4], float thrust, float rollRate, float pitchRate, float yawRate)
{
    mavlink_message_t msg;
    uint8_t typeMask = 0;
    if (rollRate == 0.0f && pitchRate == 0.0f && yawRate == 0.0f) {
        typeMask = ATTITUDE_TARGET_TYPEMASK_BODY_ROLL_RATE_IGNORE |
                   ATTITUDE_TARGET_TYPEMASK_BODY_PITCH_RATE_IGNORE |
                   ATTITUDE_TARGET_TYPEMASK_BODY_YAW_RATE_IGNORE;
    }

    float qNormalized[4];
    float norm = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    if (norm > 1e-6f) {
        qNormalized[0] = q[0] / norm;
        qNormalized[1] = q[1] / norm;
        qNormalized[2] = q[2] / norm;
        qNormalized[3] = q[3] / norm;
    } else {
        qNormalized[0] = 1.0f; qNormalized[1] = 0.0f;
        qNormalized[2] = 0.0f; qNormalized[3] = 0.0f;
    }

    mavlink_msg_set_attitude_target_pack(
        m_systemId, m_componentId, &msg,
        0,
        m_targetSystem, m_targetComponent,
        typeMask,
        qNormalized,
        rollRate, pitchRate, yawRate,
        thrust
    );
    sendMavlinkMessage(msg);
}

void MavlinkManager::setGuidedCleanState(GuidedCleanState state)
{
    m_gcState = state;
    m_gcStateEntryTimeMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());
    emit guidedCleanStateChanged(state);

    const char *names[] = {"IDLE", "SWITCHING_MODE", "DIVING", "ROTATING", "CLEANING", "RECOVERING", "DONE", "ABORTED"};
    int idx = static_cast<int>(state);
    qDebug() << "GUIDED清洗状态:" << (idx >= 0 && idx <= 7 ? names[idx] : "UNKNOWN");
}

void MavlinkManager::startGuidedClean(float cleanPitchDeg, float cleanDepthM, int cleanDurationSec)
{
    if (isGuidedCleaning()) {
        qDebug() << "GUIDED清洗正在进行中，忽略请求";
        return;
    }
    if (!isArmed()) {
        qDebug() << "请先解锁再启动GUIDED清洗";
        return;
    }

    m_gcCleanPitchDeg = cleanPitchDeg;
    m_gcCleanDepthM = cleanDepthM;
    m_gcCleanDurationSec = cleanDurationSec;

    sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, SUB_MODE_GUIDED);
    setGuidedCleanState(GC_SWITCHING_MODE);

    m_gcTimer->start();
    qDebug() << "启动GUIDED清洗: pitch=" << cleanPitchDeg << "度, depth=" << cleanDepthM
             << "m, duration=" << cleanDurationSec << "秒";
}

void MavlinkManager::abortGuidedClean()
{
    if (m_gcState == GC_IDLE) return;

    m_gcTimer->stop();
    setGuidedCleanState(GC_ABORTED);

    sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, SUB_MODE_MANUAL);
    qDebug() << "GUIDED清洗已中止，切回MANUAL模式";

    setGuidedCleanState(GC_IDLE);
}

void MavlinkManager::onGuidedCleanTimer()
{
    uint32_t nowMs = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());
    uint32_t elapsedMs = nowMs - m_gcStateEntryTimeMs;

    switch (m_gcState) {
    case GC_SWITCHING_MODE: {
        if (m_vehicleState.customMode == SUB_MODE_GUIDED) {
            float diveQ[4];
            eulerToQuaternion(0.0f, 0.0f, m_vehicleState.attitude.yaw, diveQ);
            m_gcThrust = 0.5f;
            memcpy(m_gcTargetQ, diveQ, sizeof(m_gcTargetQ));
            sendSetAttitudeTarget(m_gcTargetQ, m_gcThrust);
            setGuidedCleanState(GC_DIVING);
        } else if (elapsedMs > 5000) {
            qDebug() << "切换GUIDED模式超时，中止";
            abortGuidedClean();
        }
        break;
    }

    case GC_DIVING: {
        sendSetAttitudeTarget(m_gcTargetQ, m_gcThrust);

        bool depthOk = true;
        if (m_gcCleanDepthM > 0.0f) {
            depthOk = (m_vehicleState.depth.depth >= m_gcCleanDepthM - GC_DEPTH_THRESHOLD_M);
        }

        if (depthOk) {
            float cleanPitchRad = m_gcCleanPitchDeg * M_PI / 180.0f;
            float cleanQ[4];
            eulerToQuaternion(0.0f, cleanPitchRad, m_vehicleState.attitude.yaw, cleanQ);
            memcpy(m_gcTargetQ, cleanQ, sizeof(m_gcTargetQ));
            setGuidedCleanState(GC_ROTATING);
        } else if (elapsedMs > GC_DIVE_TIMEOUT_MS) {
            qDebug() << "下潜超时，中止";
            abortGuidedClean();
        }
        break;
    }

    case GC_ROTATING: {
        sendSetAttitudeTarget(m_gcTargetQ, m_gcThrust);

        float targetPitchRad = m_gcCleanPitchDeg * M_PI / 180.0f;
        float pitchError = fabsf(targetPitchRad - m_vehicleState.attitude.pitch);
        float pitchErrorDeg = pitchError * 180.0f / M_PI;

        if (pitchErrorDeg < GC_ATTITUDE_THRESHOLD_DEG) {
            m_gcCleanStartTimeMs = nowMs;
            setGuidedCleanState(GC_CLEANING);
        } else if (elapsedMs > GC_ROTATE_TIMEOUT_MS) {
            qDebug() << "姿态变换超时，当前误差" << pitchErrorDeg << "度，中止";
            abortGuidedClean();
        }
        break;
    }

    case GC_CLEANING: {
        sendSetAttitudeTarget(m_gcTargetQ, m_gcThrust);

        uint32_t cleaningMs = nowMs - m_gcCleanStartTimeMs;
        if (cleaningMs >= static_cast<uint32_t>(m_gcCleanDurationSec) * 1000) {
            float diveQ[4];
            eulerToQuaternion(0.0f, 0.0f, m_vehicleState.attitude.yaw, diveQ);
            memcpy(m_gcTargetQ, diveQ, sizeof(m_gcTargetQ));
            setGuidedCleanState(GC_RECOVERING);
        }
        break;
    }

    case GC_RECOVERING: {
        sendSetAttitudeTarget(m_gcTargetQ, m_gcThrust);

        float pitchError = fabsf(m_vehicleState.attitude.pitch);
        float pitchErrorDeg = pitchError * 180.0f / M_PI;

        if (pitchErrorDeg < GC_ATTITUDE_THRESHOLD_DEG) {
            m_gcTimer->stop();
            sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, SUB_MODE_MANUAL);
            setGuidedCleanState(GC_DONE);
            qDebug() << "GUIDED清洗完成，切回MANUAL模式";
            setGuidedCleanState(GC_IDLE);
        } else if (elapsedMs > GC_ROTATE_TIMEOUT_MS) {
            qDebug() << "恢复姿态超时，强制切回MANUAL";
            m_gcTimer->stop();
            sendSetModeCommand(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, SUB_MODE_MANUAL);
            setGuidedCleanState(GC_DONE);
            setGuidedCleanState(GC_IDLE);
        }
        break;
    }

    case GC_IDLE:
    case GC_DONE:
    case GC_ABORTED:
        m_gcTimer->stop();
        break;
    }
}

void MavlinkManager::onAllLogTimer()
{
    QMutexLocker locker(&m_AllLogMutex);
    //自动驾驶仪类型
    QString curautopilotType="";
    if(m_vehicleState.autopilotType==8)
    {
       curautopilotType="PX4";
    }
    else if(m_vehicleState.autopilotType==3)
    {
       curautopilotType="ArduPilot";
    }
    //载具类型
     QString cuvehicleType="";
    if(m_vehicleState.vehicleType==0)
    {
       cuvehicleType="通用";
    }
    else if(m_vehicleState.vehicleType==12)
    {
       cuvehicleType="水下机器人";
    }
    if (m_AllLogStream)
    {
         *m_AllLogStream<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
                    <<","
                    <<getSystemStatusString()
                    <<","
                    <<m_vehicleState.baseMode
                    <<","
                    <<getModeString()
                    <<","
                    <<curautopilotType
                    <<","
                    <<cuvehicleType
                    <<","
                    <<m_vehicleState.sysStatus.voltageBattery/ 1000.0f
                    <<","
                    <<m_vehicleState.sysStatus.currentBattery
                    <<","
                    <<m_vehicleState.sysStatus.batteryRemaining
                    <<","
                    <<m_vehicleState.sysStatus.dropRateComm
                    <<","
                    <<m_vehicleState.sysStatus.errorsComm
                    <<","
                    <<m_vehicleState.sysStatus.sensorsPresent
                    <<","
                    <<m_vehicleState.sysStatus.sensorsEnabled
                    <<","
                    <<m_vehicleState.sysStatus.sensorsHealth
                    <<","
                    <<getFixTypeString()
                    <<","
                    <<m_vehicleState.gps.lat
                    <<","
                    <<m_vehicleState.gps.lon
                    <<","
                    <<m_vehicleState.gps.alt
                    <<","
                    <<m_vehicleState.gps.vel
                    <<","
                    <<m_vehicleState.gps.cog//20
                    <<","

                    <<m_vehicleState.gps.satellites
                    <<","
                    <<m_vehicleState.attitude.roll
                    <<","
                    <<m_vehicleState.attitude.pitch
                    <<","
                    <<m_vehicleState.attitude.yaw
                    <<","
                    <<m_vehicleState.attitude.rollspeed
                    <<","
                    <<m_vehicleState.attitude.pitchspeed
                    <<","
                    <<m_vehicleState.attitude.yawspeed
                    <<","
                    <<m_vehicleState.attitude.yawCumulative
                    <<","
                    <<m_vehicleState.globalPos.lat
                    <<","
                    <<m_vehicleState.globalPos.lon
                    <<","
                    <<m_vehicleState.globalPos.alt
                    <<","
                    <<m_vehicleState.globalPos.relativeAlt
                    <<","
                    <<m_vehicleState.globalPos.hdg
                    <<","
                    <<m_vehicleState.vfrHud.airspeed
                    <<","
                    <<m_vehicleState.vfrHud.groundspeed
                    <<","
                    <<m_vehicleState.vfrHud.heading
                    <<","
                    <<m_vehicleState.vfrHud.throttle
                    <<","
                    <<m_vehicleState.vfrHud.alt
                    <<","
                    <<m_vehicleState.vfrHud.climbRate
                    <<","
                    <<m_vehicleState.depth.pressure//40
                    <<","

                    <<m_vehicleState.depth.depth
                    <<","
                    <<m_vehicleState.depth.temperature
                    <<","
                    <<m_vehicleState.servo.port
                    <<","
                    <<m_vehicleState.servo.raw[0]
                    <<","
                    <<m_vehicleState.servo.raw[1]
                    <<","
                    <<m_vehicleState.servo.raw[2]
                    <<","
                    <<m_vehicleState.servo.raw[3]
                    <<","
                    <<m_vehicleState.servo.raw[4]
                    <<","
                    <<m_vehicleState.servo.raw[5]
                    <<","
                    <<m_vehicleState.servo.raw[6]
                    <<","
                    <<m_vehicleState.servo.raw[7]
                    <<","
                    <<m_vehicleState.servo.raw[8]
                    <<","
                    <<m_vehicleState.servo.raw[9]
                    <<","
                    <<m_vehicleState.servo.raw[10]
                    <<","
                    <<m_vehicleState.servo.raw[11]
                    <<","
                    <<m_vehicleState.servo.raw[12]
                    <<","
                    <<m_vehicleState.servo.raw[13]
                    <<","
                    <<m_vehicleState.servo.raw[14]
                    <<","
                    <<m_vehicleState.servo.raw[15]
                    <<","
                    <<m_vehicleState.servo.count//60
                    <<","

                    <<m_vehicleState.rcChannels.channels[0] //18 路 RC 接收机原始 PWM 输入值（单位：微秒 μs）
                    <<","
                    <<m_vehicleState.rcChannels.channels[1]
                    <<","
                    <<m_vehicleState.rcChannels.channels[2]
                    <<","
                    <<m_vehicleState.rcChannels.channels[3]
                    <<","
                    <<m_vehicleState.rcChannels.channels[4]
                    <<","
                    <<m_vehicleState.rcChannels.channels[5]
                    <<","
                    <<m_vehicleState.rcChannels.channels[6]
                    <<","
                    <<m_vehicleState.rcChannels.channels[7]
                    <<","
                    <<m_vehicleState.rcChannels.channels[8]
                    <<","
                    <<m_vehicleState.rcChannels.channels[9]
                    <<","
                    <<m_vehicleState.rcChannels.channels[10]
                    <<","
                    <<m_vehicleState.rcChannels.channels[11]
                    <<","
                    <<m_vehicleState.rcChannels.channels[12]
                    <<","
                    <<m_vehicleState.rcChannels.channels[13]
                    <<","
                    <<m_vehicleState.rcChannels.channels[14]
                    <<","
                    <<m_vehicleState.rcChannels.channels[15]
                    <<","
                    <<m_vehicleState.rcChannels.channels[16]
                    <<","
                    <<m_vehicleState.rcChannels.channels[17]
                    <<","
                    <<m_vehicleState.rcChannels.rssi//79
                    << "\n";
            m_AllLogStream->flush();
    }

}

void MavlinkManager::initCommandLog()
{
    QString logDir = QCoreApplication::applicationDirPath() + "/log";
    QDir().mkpath(logDir);
    QString logFile = logDir + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "_command.csv";
    m_cmdLogFile = new QFile(logFile, this);
    if (m_cmdLogFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_cmdLogStream = new QTextStream(m_cmdLogFile);
        *m_cmdLogStream << "timestamp,direction,msg_name,msg_id,cmd_id,detail\n";
        m_cmdLogStream->flush();
        qDebug() << "命令日志已启动:" << logFile;
    } else {
        qDebug() << "命令日志文件创建失败:" << logFile;
        delete m_cmdLogFile;
        m_cmdLogFile = nullptr;
    }
}

void MavlinkManager::closeCommandLog()
{
    QMutexLocker locker(&m_cmdLogMutex);
    if (m_cmdLogStream) {
        m_cmdLogStream->flush();
        delete m_cmdLogStream;
        m_cmdLogStream = nullptr;
    }
    if (m_cmdLogFile) {
        if (m_cmdLogFile->isOpen()) {
            m_cmdLogFile->close();
        }
        delete m_cmdLogFile;
        m_cmdLogFile = nullptr;
    }
}

void MavlinkManager::initAllLog()
{
    QString logDir = QCoreApplication::applicationDirPath() + "/log";
    QDir().mkpath(logDir);
    QString logFile = logDir + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "_AllLog.csv";
    m_AllLogFile = new QFile(logFile, this);
    if (m_AllLogFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_AllLogStream = new QTextStream(m_AllLogFile);
        // 关键 1：强制 UTF-8 编码
           m_AllLogStream->setCodec("UTF-8");

            // 关键 2：写 UTF-8 BOM，让 Excel 正确识别
            m_AllLogStream->setGenerateByteOrderMark(true);
        //*m_AllLogStream << "timestamp,direction,msg_name,msg_id,cmd_id,detail\n";
           QString header = QStringLiteral("时间戳,系统状态,基础模式,自定义模式,自动驾驶仪类型,载具类型,电池总电压,电池电流,剩余电量百分比,通信丢包率,通信错误计数,在位传感器掩码,已启用传感器掩码,健康传感器掩码,gps定位类型,gps维度,gps经度,gps高度,gps地速,gps航迹向,可见卫星数,横滚角,俯仰角,偏航角,横滚角速度,俯仰角速度,偏航角速度,累计偏航角,纬度,经度,绝对海拔高度,相对起飞点高度,机头朝向,空速,地速,机头航向,油门行程,高度,爬升率,水压,计算后的深度,水温,输出端口编号,舵机1PWM,舵机2PWM,舵机3PWM,舵机4PWM,舵机5PWM,舵机6PWM,舵机7PWM,舵机8PWM,舵机9PWM,舵机10PWM,舵机11PWM,舵机12PWM,舵机13PWM,舵机14PWM,舵机15PWM,舵机15PWM,有效通道数,RC1,RC2,RC3,RC4,RC5,RC6,RC7,RC8,RC9,RC10,RC10,RC12,RC13,RC14,RC15,RC16,RC17,RC18,信号强度\n");

         *m_AllLogStream <<header;
        m_AllLogStream->flush();
        qDebug() << "全部日志已启动:" << logFile;
    } else {
        qDebug() << "全部日志文件创建失败:" << logFile;
        delete m_AllLogFile;
        m_AllLogFile = nullptr;
    }
}

void MavlinkManager::closeinitAllLog()
{
    QMutexLocker locker(&m_AllLogMutex);
    if (m_AllLogStream) {
        m_AllLogStream->flush();
        delete m_AllLogStream;
        m_AllLogStream = nullptr;
    }
    if (m_AllLogFile) {
        if (m_AllLogFile->isOpen()) {
            m_AllLogFile->close();
        }
        delete m_AllLogFile;
        m_AllLogFile = nullptr;
    }
}

void MavlinkManager::logCommandSend(const mavlink_message_t &msg)
{
    QMutexLocker locker(&m_cmdLogMutex);
    if (!m_cmdLogStream) return;

    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT || msg.msgid == MAVLINK_MSG_ID_MANUAL_CONTROL)
        return;

    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString msgName = mavlinkMsgName(msg.msgid);
    QString detail;

    switch (msg.msgid) {
    case MAVLINK_MSG_ID_COMMAND_LONG: {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(&msg, &cmd);
        detail = QString("%1,p1=%2,p2=%3,p3=%4,p4=%5,p5=%6,p6=%7,p7=%8")
                    .arg(mavlinkCmdName(cmd.command))
                    .arg(cmd.param1, 0, 'f', 2)
                    .arg(cmd.param2, 0, 'f', 2)
                    .arg(cmd.param3, 0, 'f', 2)
                    .arg(cmd.param4, 0, 'f', 2)
                    .arg(cmd.param5, 0, 'f', 2)
                    .arg(cmd.param6, 0, 'f', 2)
                    .arg(cmd.param7, 0, 'f', 2);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << "," << cmd.command << "," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_PARAM_SET: {
        mavlink_param_set_t ps;
        mavlink_msg_param_set_decode(&msg, &ps);
        char pid[17];
        strncpy(pid, ps.param_id, 16);
        pid[16] = '\0';
        detail = QString("param_id=%1,value=%2").arg(pid).arg(ps.param_value, 0, 'f', 6);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_PARAM_REQUEST_READ: {
        mavlink_param_request_read_t pr;
        mavlink_msg_param_request_read_decode(&msg, &pr);
        char pid[17];
        strncpy(pid, pr.param_id, 16);
        pid[16] = '\0';
        detail = QString("param_id=%1").arg(pid);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_MANUAL_CONTROL: {
        mavlink_manual_control_t mc;
        mavlink_msg_manual_control_decode(&msg, &mc);
        detail = QString("x=%1,y=%2,z=%3,r=%4,buttons=%5")
                    .arg(mc.x).arg(mc.y).arg(mc.z).arg(mc.r).arg(mc.buttons);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_SET_ATTITUDE_TARGET: {
        mavlink_set_attitude_target_t sat;
        mavlink_msg_set_attitude_target_decode(&msg, &sat);
        detail = QString("q=[%1,%2,%3,%4],thrust=%5,type_mask=%6")
                    .arg(sat.q[0], 0, 'f', 3).arg(sat.q[1], 0, 'f', 3)
                    .arg(sat.q[2], 0, 'f', 3).arg(sat.q[3], 0, 'f', 3)
                    .arg(sat.thrust, 0, 'f', 3).arg((int)sat.type_mask);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_REQUEST_DATA_STREAM: {
        mavlink_request_data_stream_t rds;
        mavlink_msg_request_data_stream_decode(&msg, &rds);
        detail = QString("stream_id=%1,rate=%2,start_stop=%3")
                    .arg((int)rds.req_stream_id).arg((int)rds.req_message_rate).arg((int)rds.start_stop);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE: {
        mavlink_rc_channels_override_t rco;
        mavlink_msg_rc_channels_override_decode(&msg, &rco);
        detail = QString("ch1=%1,ch2=%2,ch3=%3,ch4=%4,ch5=%5,ch6=%6,ch7=%7,ch8=%8")
                    .arg(rco.chan1_raw).arg(rco.chan2_raw).arg(rco.chan3_raw).arg(rco.chan4_raw)
                    .arg(rco.chan5_raw).arg(rco.chan6_raw).arg(rco.chan7_raw).arg(rco.chan8_raw);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    case MAVLINK_MSG_ID_SET_POSITION_TARGET_LOCAL_NED: {
        mavlink_set_position_target_local_ned_t spt;
        mavlink_msg_set_position_target_local_ned_decode(&msg, &spt);
        detail = QString("vx=%1,vy=%2,vz=%3,yaw_rate=%4,type_mask=%5")
                    .arg(spt.vx, 0, 'f', 2).arg(spt.vy, 0, 'f', 2)
                    .arg(spt.vz, 0, 'f', 2).arg(spt.yaw_rate, 0, 'f', 2)
                    .arg((int)spt.type_mask);
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0," << detail << "\n";
        break;
    }
    default: {
        *m_cmdLogStream << ts << ",SEND," << msgName << "," << msg.msgid
                        << ",0,\n";
        break;
    }
    }
    m_cmdLogStream->flush();
}

QString MavlinkManager::mavlinkMsgName(uint32_t msgid) const
{
    switch (msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: return "HEARTBEAT";
    case MAVLINK_MSG_ID_COMMAND_LONG: return "COMMAND_LONG";
    case MAVLINK_MSG_ID_COMMAND_ACK: return "COMMAND_ACK";
    case MAVLINK_MSG_ID_MANUAL_CONTROL: return "MANUAL_CONTROL";
    case MAVLINK_MSG_ID_PARAM_SET: return "PARAM_SET";
    case MAVLINK_MSG_ID_PARAM_VALUE: return "PARAM_VALUE";
    case MAVLINK_MSG_ID_PARAM_REQUEST_READ: return "PARAM_REQUEST_READ";
    case MAVLINK_MSG_ID_SET_ATTITUDE_TARGET: return "SET_ATTITUDE_TARGET";
    case MAVLINK_MSG_ID_REQUEST_DATA_STREAM: return "REQUEST_DATA_STREAM";
    case MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE: return "RC_CHANNELS_OVERRIDE";
    case MAVLINK_MSG_ID_SET_POSITION_TARGET_LOCAL_NED: return "SET_POSITION_TARGET_LOCAL_NED";
    default: return QString("MSG_%1").arg(msgid);
    }
}

QString MavlinkManager::mavlinkCmdName(uint16_t cmd) const
{
    switch (cmd) {
    case MAV_CMD_COMPONENT_ARM_DISARM: return "ARM_DISARM";
    case MAV_CMD_DO_SET_MODE: return "DO_SET_MODE";
    case MAV_CMD_DO_SET_SERVO: return "DO_SET_SERVO";
    case MAV_CMD_SET_MESSAGE_INTERVAL: return "SET_MESSAGE_INTERVAL";
    case MAV_CMD_PREFLIGHT_STORAGE: return "PREFLIGHT_STORAGE";
    case 31000: return "CLEANER_CMD";
    default: return QString("CMD_%1").arg(cmd);
    }
}

QString MavlinkManager::mavlinkAckResultString(uint8_t result) const
{
    switch (result) {
    case MAV_RESULT_ACCEPTED: return "ACCEPTED";
    case MAV_RESULT_DENIED: return "DENIED";
    case MAV_RESULT_TEMPORARILY_REJECTED: return "TEMP_REJECTED";
    case MAV_RESULT_UNSUPPORTED: return "UNSUPPORTED";
    case MAV_RESULT_FAILED: return "FAILED";
    case MAV_RESULT_IN_PROGRESS: return "IN_PROGRESS";
    default: return QString("RESULT_%1").arg(result);
    }
}
