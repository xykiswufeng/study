#include "virtualjoystick.h"
#include "mavlink/mavlink_manager.h"
#include <QDebug>
#include <cmath>

VirtualJoystick::VirtualJoystick(bool bLeftHand, QWidget *parent)
    : QWidget(parent)
    , m_leftStick(nullptr)
    , m_rightStick(nullptr)
    , m_layout(nullptr)
    , m_updateTimer(nullptr)
    , m_mavlinkManager(nullptr)
    , m_autoCenterThrottle(true)
    , m_leftHandedMode(bLeftHand)
    , m_initialConnectComplete(false)
    , m_leftYAxisValue(0)
    , m_roll(0.0f)
    , m_pitch(0.0f)
    , m_yaw(0.0f)
    , m_thrust(0.0f)
{
    setupUi();

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(40); // 25Hz, 与QGC VirtualJoystick.qml一致
    connect(m_updateTimer, &QTimer::timeout, this, &VirtualJoystick::onTimerTimeout);
    setStyleSheet("VirtualJoystick { background-color: rgba(30, 30, 30, 150); border-radius: 10px; }");
    setVisible(true);
    updateStickProperties();
}

VirtualJoystick::~VirtualJoystick()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void VirtualJoystick::setupUi()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(10);

    m_leftStick = new JoystickThumbPad(this);
    m_leftStick->setIsLeftStick(true);
    m_leftStick->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_leftStick, &JoystickThumbPad::stickMoved, this, &VirtualJoystick::onLeftStickMoved);
    connect(m_leftStick, &JoystickThumbPad::stickPressed, this, [this]() {});
    connect(m_leftStick, &JoystickThumbPad::stickReleased, this, [this]() {});

    m_rightStick = new JoystickThumbPad(this);
    m_rightStick->setIsLeftStick(false);
    m_rightStick->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_rightStick, &JoystickThumbPad::stickMoved, this, &VirtualJoystick::onRightStickMoved);
    connect(m_rightStick, &JoystickThumbPad::stickPressed, this, [this]() {});
    connect(m_rightStick, &JoystickThumbPad::stickReleased, this, [this]() {});

    m_layout->addWidget(m_leftStick);
    m_layout->addWidget(m_rightStick);
}
//true
void VirtualJoystick::setAutoCenterThrottle(bool enabled)
{
    if (m_autoCenterThrottle != enabled) {
        m_autoCenterThrottle = enabled;
        updateStickProperties();
    }
}
//false
void VirtualJoystick::setLeftHandedMode(bool enabled)
{
    if (m_leftHandedMode != enabled) {
        m_leftHandedMode = enabled;
        updateStickProperties();
    }
}

void VirtualJoystick::setMavlinkManager(MavlinkManager *manager)
{
    m_mavlinkManager = manager;
}

void VirtualJoystick::setInitialConnectComplete(bool complete)
{
    m_initialConnectComplete = complete;
}

void VirtualJoystick::updateStickProperties()
{
    if (!m_leftStick || !m_rightStick) {
        return;
    }

    // 与QGC VirtualJoystick.qml完全一致:
    //   leftStick:  yAxisPositiveRangeOnly = !rover && !leftHandedMode
    //               yAxisReCenter = autoCenterThrottle
    //   rightStick: yAxisPositiveRangeOnly = !rover && leftHandedMode
    //               yAxisReCenter = true
    //
    // ArduSub非rover, 非左手模式(默认):
    //   左杆: yAxisPositiveRangeOnly=true  → Y轴[0,1]油门, X轴[-1,1]yaw
    //   右杆: yAxisPositiveRangeOnly=false → Y轴[-1,1]pitch, X轴[-1,1]roll

    m_leftStick->setYAxisPositiveRangeOnly(!m_leftHandedMode);
    m_leftStick->setYAxisReCenter(m_autoCenterThrottle);
    m_rightStick->setYAxisPositiveRangeOnly(m_leftHandedMode);
    m_rightStick->setYAxisReCenter(true);

    m_leftStick->setLeftHandedMode(m_leftHandedMode);
    m_rightStick->setLeftHandedMode(m_leftHandedMode);

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        QTimer::singleShot(200, this, [this]() {
            if (m_leftStick && m_rightStick && m_leftStick->width() > 50 && m_rightStick->width() > 50) {
                m_leftStick->reCenter(false);
                m_rightStick->reCenter(false);
                qDebug() << "Joystick initialized with size:" << m_leftStick->width() << m_leftStick->height();
            }
        });
    }
}

void VirtualJoystick::reCenterAll()
{
    if (m_leftStick) {
        m_leftStick->reCenter();
    }
    if (m_rightStick) {
        m_rightStick->reCenter();
    }
    m_roll = 0.0f;
    m_pitch = 0.0f;
    m_yaw = 0.0f;
    m_thrust = 0.0f;
}

void VirtualJoystick::setVisible(bool visible)
{
    QWidget::setVisible(visible);

    if (visible) {
        m_updateTimer->start();
    } else {
        m_updateTimer->stop();
    }
}

void VirtualJoystick::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int availableWidth = width();
    int availableHeight = height();
    int stickSize = qMin(availableHeight, (availableWidth - 20) / 2);
    stickSize = qMax(stickSize, 150);

    if (m_leftStick && m_rightStick) {
        m_leftStick->setFixedSize(stickSize, stickSize);
        m_rightStick->setFixedSize(stickSize, stickSize);
        qDebug() << "[VirtualJoystick] Resized sticks to:" << stickSize << "x" << stickSize;
    }
}

void VirtualJoystick::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_updateTimer->start();
}

void VirtualJoystick::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_updateTimer->stop();
}

void VirtualJoystick::onTimerTimeout()
{
    //mavlink和初始化连接完成
    if (m_mavlinkManager && m_initialConnectComplete)
    {
        static int forceCount = 0;
        forceCount++;

        if (forceCount % 50 == 0) {
            if (m_leftStick && !m_leftStick->isPressed()) {
                if (std::abs(m_leftStick->xAxis()) > 0.05 || std::abs(m_leftStick->yAxis()) > 0.05) {
                    m_leftStick->reCenter(true);
                }
            }
            if (m_rightStick && !m_rightStick->isPressed()) {
                if (std::abs(m_rightStick->xAxis()) > 0.05 || std::abs(m_rightStick->yAxis()) > 0.05) {
                    m_rightStick->reCenter(true);
                }
            }
        }

        sendJoystickValues();
    }
}

//发送虚拟按键值 左右的值 归一化怎么做 区分
void VirtualJoystick::sendJoystickValues()
{
    if (!m_leftStick || !m_rightStick) {
        return;
    }

    // 与QGC VirtualJoystick.qml第26行完全一致:
    //   非左手模式: virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
    //     → roll=rightX, pitch=rightY, yaw=leftX, thrust=leftY
    //   左手模式:   virtualTabletJoystickValue(leftStick.xAxis, leftStick.yAxis, rightStick.xAxis, rightStick.yAxis)
    //     → roll=leftX, pitch=leftY, yaw=rightX, thrust=rightY

    float roll, pitch, yaw, thrust;
    if (!m_leftHandedMode) {
        roll   = static_cast<float>(m_rightStick->xAxis());
        pitch  = static_cast<float>(m_rightStick->yAxis());
        yaw    = static_cast<float>(m_leftStick->xAxis());
        thrust = static_cast<float>(m_leftStick->yAxis());
    } else {
        roll   = static_cast<float>(m_leftStick->xAxis());
        pitch  = static_cast<float>(m_leftStick->yAxis());
        yaw    = static_cast<float>(m_rightStick->xAxis());
        thrust = static_cast<float>(m_rightStick->yAxis());
    }

    // 死区
    const float deadZone = 0.05f;
    if (std::abs(roll) < deadZone) roll = 0.0f;
    if (std::abs(pitch) < deadZone) pitch = 0.0f;
    if (std::abs(yaw) < deadZone) yaw = 0.0f;

    // 与QGC Vehicle.cc sendJoystickDataThreadSafe一致:
    //   throttleModeCenterZero=false(默认): thrust已经是[0,1], 直接使用
    //   z = thrust * 1000, 范围[0,1000], 500为中位
    float thrustMapped = thrust;

    m_roll = roll;
    m_pitch = pitch;
    m_yaw = yaw;
    m_thrust = thrustMapped;

    static bool lastCenterState = false;
    bool currentCenterState = (std::abs(thrustMapped - 0.5f) < 0.05f);
    if (currentCenterState != lastCenterState) {
        lastCenterState = currentCenterState;
        emit throttleCenterStateChanged(currentCenterState);
    }

    emit joystickValueChanged(roll, pitch, yaw, thrustMapped);
}

void VirtualJoystick::onLeftStickMoved(qreal x, qreal y)
{
    Q_UNUSED(x); Q_UNUSED(y);
}

void VirtualJoystick::onRightStickMoved(qreal x, qreal y)
{
    Q_UNUSED(x); Q_UNUSED(y);
}

void VirtualJoystick::onActiveVehicleChanged()
{
    updateStickProperties();
}

float VirtualJoystick::getRoll() const
{
    return m_roll;
}

float VirtualJoystick::getPitch() const
{
    return m_pitch;
}

float VirtualJoystick::getYaw() const
{
    return m_yaw;
}

float VirtualJoystick::getThrust() const
{
    return m_thrust;
}

bool VirtualJoystick::isThrottleAtCenter(float tolerance) const
{
    JoystickThumbPad* throttleStick = getThrottleStick();
    if (!throttleStick) {
        return false;
    }
    qreal throttleValue = throttleStick->yAxis();
    // yAxisPositiveRangeOnly=true时范围[0,1], 中位=0.5
    // yAxisPositiveRangeOnly=false时范围[-1,1], 中位=0
    if (throttleStick->yAxisPositiveRangeOnly()) {
        return std::abs(throttleValue - 0.5) <= tolerance;
    } else {
        return std::abs(throttleValue) <= tolerance;
    }
}

JoystickThumbPad* VirtualJoystick::getThrottleStick() const
{
    // 与QGC一致: 非左手模式油门在左杆, 左手模式油门在右杆
    return m_leftHandedMode ? m_rightStick : m_leftStick;
}
