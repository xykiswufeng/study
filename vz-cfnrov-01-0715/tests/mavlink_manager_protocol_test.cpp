#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>
#include <cmath>

#include "mavlink/mavlink_manager.h"

namespace {

QByteArray serializeMessage(const mavlink_message_t &message)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN] = {};
    const uint16_t length = mavlink_msg_to_send_buffer(buffer, &message);
    return QByteArray(reinterpret_cast<const char *>(buffer), length);
}

bool decodeSingleMessage(const QByteArray &bytes, mavlink_message_t &message)
{
    mavlink_status_t status{};
    for (char byte : bytes) {
        if (mavlink_parse_char(MAVLINK_COMM_1, static_cast<uint8_t>(byte),
                               &message, &status)) {
            return true;
        }
    }
    return false;
}

bool require(bool condition, const char *message)
{
    if (!condition) {
        qCritical().noquote() << "FAIL:" << message;
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    MavlinkManager manager;

    int heartbeatCount = 0;
    int attitudeCount = 0;
    QObject::connect(&manager, &MavlinkManager::heartbeatReceived,
                     [&heartbeatCount]() { ++heartbeatCount; });
    QObject::connect(&manager, &MavlinkManager::attitudeUpdated,
                     [&attitudeCount](float, float, float, float, float, float, float) {
        ++attitudeCount;
    });

    // 回环回来的本机GCS心跳不得被判定为Pixhawk在线。
    mavlink_message_t gcsHeartbeat{};
    mavlink_msg_heartbeat_pack(255, 190, &gcsHeartbeat,
                               MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID,
                               MAV_MODE_MANUAL_ARMED, 0, MAV_STATE_ACTIVE);
    manager.parseMavlinkData(serializeMessage(gcsHeartbeat));
    if (!require(heartbeatCount == 0, "GCS heartbeat was accepted as vehicle heartbeat") ||
        !require(!manager.hasVehicleHeartbeat(), "GCS heartbeat created a vehicle connection")) {
        return 1;
    }

    // 建链前的姿态回显同样必须忽略。
    mavlink_message_t earlyAttitude{};
    mavlink_msg_attitude_pack(255, 190, &earlyAttitude, 1,
                              0.1f, 0.2f, 0.3f, 0.0f, 0.0f, 0.0f);
    manager.parseMavlinkData(serializeMessage(earlyAttitude));
    if (!require(attitudeCount == 0, "telemetry was accepted before vehicle heartbeat")) {
        return 1;
    }

    // 真实ArduSub/Pixhawk心跳建立连接。
    mavlink_message_t vehicleHeartbeat{};
    mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &vehicleHeartbeat,
                               MAV_TYPE_SUBMARINE, MAV_AUTOPILOT_ARDUPILOTMEGA,
                               MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 19, MAV_STATE_STANDBY);
    manager.parseMavlinkData(serializeMessage(vehicleHeartbeat));
    if (!require(heartbeatCount == 1, "Pixhawk heartbeat was not accepted") ||
        !require(manager.hasVehicleHeartbeat(), "Pixhawk connection state was not established")) {
        return 1;
    }

    // 真实姿态和深度遥测应更新车辆状态。
    mavlink_message_t attitude{};
    mavlink_msg_attitude_pack(1, MAV_COMP_ID_AUTOPILOT1, &attitude, 2,
                              0.1f, -0.2f, 0.3f, 0.01f, 0.02f, 0.03f);
    manager.parseMavlinkData(serializeMessage(attitude));

    mavlink_message_t pressure{};
    mavlink_msg_scaled_pressure2_pack(1, MAV_COMP_ID_AUTOPILOT1, &pressure,
                                      3, 1113.25f, 0.0f, 2500, 0.0f);
    manager.parseMavlinkData(serializeMessage(pressure));
    const VehicleState &state = manager.getVehicleState();
    if (!require(attitudeCount == 1, "Pixhawk attitude was not emitted") ||
        !require(std::fabs(state.depth.depth - 1.0f) < 0.02f,
                 "SCALED_PRESSURE2 depth was not parsed")) {
        return 1;
    }

    // 连接锁定后，其他系统的心跳不能抢占目标飞控。
    mavlink_message_t otherVehicle{};
    mavlink_msg_heartbeat_pack(2, MAV_COMP_ID_AUTOPILOT1, &otherVehicle,
                               MAV_TYPE_SUBMARINE, MAV_AUTOPILOT_ARDUPILOTMEGA,
                               0, 0, MAV_STATE_ACTIVE);
    manager.parseMavlinkData(serializeMessage(otherVehicle));
    if (!require(heartbeatCount == 1, "another vehicle replaced the active target")) {
        return 1;
    }

    QByteArray lastSent;
    QObject::connect(&manager, &MavlinkManager::sendData,
                     [&lastSent](const QByteArray &data) { lastSent = data; });

    // Custom1: MANUAL_CONTROL buttons bit 1 = value 2。
    manager.sendManualControl(100, -200, 500, 300, 2);
    mavlink_message_t outgoing{};
    if (!require(decodeSingleMessage(lastSent, outgoing), "MANUAL_CONTROL could not be decoded") ||
        !require(outgoing.msgid == MAVLINK_MSG_ID_MANUAL_CONTROL,
                 "manual input is not MAVLink MANUAL_CONTROL")) {
        return 1;
    }
    mavlink_manual_control_t manual{};
    mavlink_msg_manual_control_decode(&outgoing, &manual);
    if (!require(manual.target == 1 && manual.buttons == 2 && manual.z == 500,
                 "MANUAL_CONTROL fields changed")) {
        return 1;
    }

    manager.sendArmCommand(true);
    if (!require(decodeSingleMessage(lastSent, outgoing), "arm command could not be decoded") ||
        !require(outgoing.msgid == MAVLINK_MSG_ID_COMMAND_LONG,
                 "arm command is not MAVLink COMMAND_LONG")) {
        return 1;
    }
    mavlink_command_long_t command{};
    mavlink_msg_command_long_decode(&outgoing, &command);
    if (!require(command.command == MAV_CMD_COMPONENT_ARM_DISARM && command.param1 == 1.0f,
                 "arm COMMAND_LONG fields changed")) {
        return 1;
    }

    manager.setFrontCameraPan(1600);
    if (!require(decodeSingleMessage(lastSent, outgoing), "gimbal command could not be decoded")) {
        return 1;
    }
    mavlink_msg_command_long_decode(&outgoing, &command);
    if (!require(command.command == MAV_CMD_DO_SET_SERVO &&
                 static_cast<int>(command.param1) == MavlinkManager::PWM_CHANNEL_FRONT_PAN &&
                 static_cast<int>(command.param2) == 1600,
                 "gimbal MAVLink servo mapping changed")) {
        return 1;
    }

    manager.resetVehicleConnection();
    if (!require(!manager.hasVehicleHeartbeat(), "reset did not clear vehicle connection")) {
        return 1;
    }

    qInfo().noquote() << "PASS: MAVLink connection filtering, telemetry and controls";
    return 0;
}
