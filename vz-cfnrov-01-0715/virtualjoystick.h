#ifndef VIRTUALJOYSTICK_HPP
#define VIRTUALJOYSTICK_HPP

#include <QWidget>
#include <QHBoxLayout>
#include <QTimer>
#include "joystickthumbpad.h"

class MavlinkManager;

class VirtualJoystick : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool autoCenterThrottle READ autoCenterThrottle WRITE setAutoCenterThrottle)
    Q_PROPERTY(bool leftHandedMode READ leftHandedMode WRITE setLeftHandedMode)

public:
    /// @brief 创建并初始化 VirtualJoystick 对象。
    explicit VirtualJoystick(bool bLeftHand,QWidget *parent = nullptr);
    /// @brief 停止后台任务并释放 VirtualJoystick 占用的资源。
    ~VirtualJoystick();

    /// @brief 执行“autoCenterThrottle”对应的业务操作。
    bool autoCenterThrottle() const { return m_autoCenterThrottle; }
    /// @brief 执行模式对应的业务操作。
    bool leftHandedMode() const { return m_leftHandedMode; }

    /// @brief 设置“AutoCenterThrottle”并同步相关状态。
    void setAutoCenterThrottle(bool enabled);
    /// @brief 设置模式并同步相关状态。
    void setLeftHandedMode(bool enabled);
    /// @brief 设置“MavlinkManager”并同步相关状态。
    void setMavlinkManager(MavlinkManager *manager);
    /// @brief 设置“InitialConnectComplete”并同步相关状态。
    void setInitialConnectComplete(bool complete);

    // 获取摇杆值（范围 -1 到 1，油门可能是 0 到 1）
    /// @brief 获取横滚。
    float getRoll() const;
    /// @brief 获取俯仰。
    float getPitch() const;
    /// @brief 获取航向。
    float getYaw() const;
    /// @brief 获取“Thrust”。
    float getThrust() const;

    /// @brief 执行“reCenterAll”对应的业务操作。
    void reCenterAll();
    /// @brief 设置“Visible”并同步相关状态。
    void setVisible(bool visible) override;
    /// @brief 判断“ThrottleAtCenter”是否处于有效状态。
    bool isThrottleAtCenter(float tolerance = 0.05f) const;
    /// @brief 执行“suppressOutput”对应的业务操作。
    void suppressOutput(int durationMs = 500);
    /// 获取当前油门摇杆
    /// @brief 获取“ThrottleStick”。
    JoystickThumbPad* getThrottleStick() const;
    /// @brief 获取“LeftStick”。
    JoystickThumbPad* getLeftStick() const { return m_leftStick; }
    /// @brief 获取“RightStick”。
    JoystickThumbPad* getRightStick() const { return m_rightStick; }

signals:
    /// 摇杆值变化信号
    /// @param roll  横滚 [-1, 1]
    /// @param pitch 俯仰 [-1, 1]
    /// @param yaw   偏航 [-1, 1]
    /// @param thrust 油门 [-1, 1] 或 [0, 1]
    /// @brief 通知订阅者摇杆、数值已经变化。
    void joystickValueChanged(float roll, float pitch, float yaw, float thrust);

    /// @brief 执行“leftStickMoved”对应的业务操作。
    void leftStickMoved(qreal x, qreal y);
    /// @brief 执行“rightStickMoved”对应的业务操作。
    void rightStickMoved(qreal x, qreal y);
    /// @brief 通知订阅者状态已经变化。
    void throttleCenterStateChanged(bool atCenter);

protected:
    /// @brief 处理窗口尺寸变化并重新调整子控件。
    void resizeEvent(QResizeEvent *event) override;
    /// @brief 处理显示事件。
    void showEvent(QShowEvent *event) override;
    /// @brief 处理“hide”事件。
    void hideEvent(QHideEvent *event) override;

private slots:
    /// @brief 处理超时对应的事件或信号回调。
    void onTimerTimeout();
    /// @brief 处理“LeftStickMoved”对应的事件或信号回调。
    void onLeftStickMoved(qreal x, qreal y);
    /// @brief 处理“RightStickMoved”对应的事件或信号回调。
    void onRightStickMoved(qreal x, qreal y);
    /// @brief 通知订阅者ROV 状态已经变化。
    void onActiveVehicleChanged();

private:
    JoystickThumbPad *m_leftStick;
    JoystickThumbPad *m_rightStick;
    QHBoxLayout *m_layout;
    QTimer *m_updateTimer;

    MavlinkManager *m_mavlinkManager;
    bool m_autoCenterThrottle;
    bool m_leftHandedMode;
    bool m_initialConnectComplete;
    qreal m_leftYAxisValue;

    // 当前姿态值
    float m_roll;
    float m_pitch;
    float m_yaw;
    float m_thrust;

    /// @brief 设置“upUi”并同步相关状态。
    void setupUi();
    /// @brief 根据最新数据更新“StickProperties”。
    void updateStickProperties();
    /// @brief 封装并发送摇杆。
    void sendJoystickValues();
};

#endif // VIRTUALJOYSTICK_HPP
