#ifndef SDL2GAMEPAD_H
#define SDL2GAMEPAD_H

#include <QThread>
#include <QRecursiveMutex>

struct _SDL_GameController;
typedef _SDL_GameController SDL_GameController;

class SDL2Gamepad : public QThread
{
    Q_OBJECT

public:
    /// @brief 创建并初始化 SDL2Gamepad 对象。
    explicit SDL2Gamepad(QObject *parent = nullptr);
    /// @brief 停止后台任务并释放 SDL2Gamepad 占用的资源。
    ~SDL2Gamepad();

    /// @brief 判断“Connected”是否处于有效状态。
    bool isConnected() const;
    /// @brief 执行“name”对应的业务操作。
    QString name() const;

    /// @brief 启动“Polling”。
    void startPolling();
    /// @brief 停止“Polling”并收尾相关状态。
    void stopPolling();

signals:
    /// @brief 建立“ed”所需的信号连接。
    void connected();
    /// @brief 执行“disconnected”对应的业务操作。
    void disconnected();

    /// @brief 通知订阅者“axisLeftX”已经变化。
    void axisLeftXChanged(double value);
    /// @brief 通知订阅者“axisLeftY”已经变化。
    void axisLeftYChanged(double value);
    /// @brief 通知订阅者“axisRightX”已经变化。
    void axisRightXChanged(double value);
    /// @brief 通知订阅者“axisRightY”已经变化。
    void axisRightYChanged(double value);

    /// @brief 通知订阅者按钮已经变化。
    void buttonAChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonBChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonXChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonYChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonL1Changed(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonR1Changed(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonL2Changed(double value);
    /// @brief 通知订阅者按钮已经变化。
    void buttonR2Changed(double value);
    /// @brief 通知订阅者按钮已经变化。
    void buttonStartChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonBackChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonGuideChanged(bool pressed);

    /// @brief 通知订阅者按钮已经变化。
    void buttonDpadUpChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonDpadDownChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonDpadLeftChanged(bool pressed);
    /// @brief 通知订阅者按钮已经变化。
    void buttonDpadRightChanged(bool pressed);

protected:
    /// @brief 执行工作线程的主循环。
    void run() override;

private:
    /// @brief 执行“openController”对应的业务操作。
    void openController(int index);
    /// @brief 关闭并释放“Controller”相关资源。
    void closeController();
    /// @brief 执行“loadGameControllerMappings”对应的业务操作。
    void loadGameControllerMappings();
    /// @brief 解析并处理收到的“Axis”。
    void handleAxis();
    /// @brief 解析并处理收到的“Buttons”。
    void handleButtons();
    /// @brief 解析并处理收到的“Events”。
    void handleEvents();

    SDL_GameController *m_controller = nullptr;
    mutable QRecursiveMutex m_mutex;
    volatile bool m_exitThread = false;
    int m_index = -1;

    double m_lastLeftX = 0;
    double m_lastLeftY = 0;
    double m_lastRightX = 0;
    double m_lastRightY = 0;
    double m_lastL2 = 0;
    double m_lastR2 = 0;
    bool m_lastA = false;
    bool m_lastB = false;
    bool m_lastX = false;
    bool m_lastY = false;
    bool m_lastL1 = false;
    bool m_lastR1 = false;
    bool m_lastStart = false;
    bool m_lastBack = false;
    bool m_lastGuide = false;
    bool m_lastDpadUp = false;
    bool m_lastDpadDown = false;
    bool m_lastDpadLeft = false;
    bool m_lastDpadRight = false;
};

#endif // SDL2GAMEPAD_H
