#include "sdl2gamepad.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#define DEBUG qDebug() << __FILE__ << __LINE__
static const double DEADBAND = 0.05;
static const double TRIGGER_THRESHOLD = 0.15;
static const int POLL_INTERVAL_MS = 5;

static double applyDeadband(double value, double deadband)
{
    if (qAbs(value) < deadband) return 0.0;
    if (value > 0.0) return (value - deadband) / (1.0 - deadband);
    return (value + deadband) / (1.0 - deadband);
}

SDL2Gamepad::SDL2Gamepad(QObject *parent)
    : QThread(parent)
{
    //允许 SDL 在程序窗口不在前台的时候，仍然接收手柄 / 摇杆事件
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        qWarning() << "SDL2 init failed:" << SDL_GetError();
        return;
    }
    //将规则加入sdl
    loadGameControllerMappings();
    //SDL_NumJoysticks：返回当前系统识别到的**所有摇杆 / 手柄设备总数
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        //是否支持 GameController 标准化映射
        if (SDL_IsGameController(i))
        {
            m_index = i;
            break;
        }
    }
}

SDL2Gamepad::~SDL2Gamepad()
{
    stopPolling();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
}

bool SDL2Gamepad::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    return m_controller != nullptr;
}

QString SDL2Gamepad::name() const
{
    QMutexLocker locker(&m_mutex);
    if (m_controller) 
    {
        return QString(SDL_GameControllerName(m_controller));
    }
    return QString();
}

void SDL2Gamepad::startPolling()
{
    if (!isRunning())
    {
        m_exitThread = false;
        start();
    }
}

void SDL2Gamepad::stopPolling()
{
    m_exitThread = true;
    if (isRunning())
    {
        wait(3000);
    }
}

void SDL2Gamepad::loadGameControllerMappings()
{
    QFile file(":/gamecontrollerdb.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        file.setFileName("gamecontrollerdb.txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "No gamecontrollerdb.txt found, using SDL defaults";
            return;
        }
    }

    QTextStream in(&file);
    int count = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        if (SDL_GameControllerAddMapping(line.toUtf8().constData()) == 1) {
            count++;
        }
    }
    DEBUG<<count;
    qDebug() << "Loaded" << count << "game controller mappings";
}

void SDL2Gamepad::openController(int index)
{
    QMutexLocker locker(&m_mutex);
    if (m_controller)
    {
        //关闭旧的
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }
    //重新打开新的
    m_controller = SDL_GameControllerOpen(index);
    if (m_controller)
    {
        DEBUG<< "SDL2手柄已连接:" << SDL_GameControllerName(m_controller);
        emit connected();
    }
}

void SDL2Gamepad::closeController()
{
    QMutexLocker locker(&m_mutex);
    if (m_controller)
    {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
        emit disconnected();
    }
}

void SDL2Gamepad::run()
{
    if (m_index >= 0)
    {
        openController(m_index);
    }

    while (!m_exitThread)
    {
        handleEvents();

        m_mutex.lock();
        if (m_controller)
        {
            SDL_GameControllerUpdate();
            m_mutex.unlock();
            handleAxis();
            handleButtons();
        }
        else
        {
            m_mutex.unlock();
        }

        QThread::msleep(POLL_INTERVAL_MS);
    }

    closeController();
}

void SDL2Gamepad::handleEvents()
{
    SDL_Event event;
    // 循环取出SDL事件队列中所有待处理事件
    while (SDL_PollEvent(&event))
    {
        //GameController插入事件（优先触发）
        if (event.type == SDL_CONTROLLERDEVICEADDED)
        {
            //是否处于连接状态
            if (!isConnected())
            {
                //打开手柄 SDL 设备索引编号
                openController(event.cdevice.which);
            }
        }
        //设备移除
        else if (event.type == SDL_CONTROLLERDEVICEREMOVED)
        {
            QMutexLocker locker(&m_mutex);
            if (m_controller)
            {
                //获取底层原始摇杆对象；
                SDL_Joystick *joy = SDL_GameControllerGetJoystick(m_controller);
                if (joy && SDL_JoystickInstanceID(joy) == event.cdevice.which)
                {
                    locker.unlock();
                    qDebug() << "SDL2手柄已断开";
                    closeController();
                }
            }
        }
        //Joystick插入事件
        else if (event.type == SDL_JOYDEVICEADDED)
        {
            if (!isConnected() && SDL_IsGameController(event.jdevice.which))
            {
                openController(event.jdevice.which);
            }
        }
    }
}

void SDL2Gamepad::handleAxis()
{
    QMutexLocker locker(&m_mutex);
    if (!m_controller) return;

    //归一化[-1,1]  l2 r2[0,1]
    double leftX = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0;
    double leftY = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0;
    double rightX = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0;
    double rightY = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0;
    double l2 = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0;
    double r2 = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0;

    leftX = applyDeadband(leftX, DEADBAND);
    leftY = applyDeadband(leftY, DEADBAND);
    rightX = applyDeadband(rightX, DEADBAND);
    rightY = applyDeadband(rightY, DEADBAND);
    leftY = -leftY;
    rightY = -rightY;
    if (l2 < TRIGGER_THRESHOLD) l2 = 0.0;
    if (r2 < TRIGGER_THRESHOLD) r2 = 0.0;

    if (qAbs(leftX - m_lastLeftX) > 0.01) { m_lastLeftX = leftX; emit axisLeftXChanged(leftX); }//偏航
    if (qAbs(leftY - m_lastLeftY) > 0.01) { m_lastLeftY = leftY; emit axisLeftYChanged(leftY); }//油门
    if (qAbs(rightX - m_lastRightX) > 0.01) { m_lastRightX = rightX; emit axisRightXChanged(rightX); }//前进
    if (qAbs(rightY - m_lastRightY) > 0.01) { m_lastRightY = rightY; emit axisRightYChanged(rightY); }//后退
    if (qAbs(l2 - m_lastL2) > 0.05) { m_lastL2 = l2; emit buttonL2Changed(l2); }
    if (qAbs(r2 - m_lastR2) > 0.05) { m_lastR2 = r2; emit buttonR2Changed(r2); }
}

void SDL2Gamepad::handleButtons()
{
    QMutexLocker locker(&m_mutex);
    if (!m_controller) return;

    bool a = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A);
    bool b = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B);
    bool x = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_X);
    bool y = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_Y);
    bool l1 = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    bool r1 = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    bool start = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START);
    bool back = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_BACK);
    bool guide = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_GUIDE);
    bool dpadUp = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
    bool dpadDown = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    bool dpadLeft = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    bool dpadRight = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
   // DEBUG<<"a"<<a<<"b"<<b<<"x"<<x<<"y"<<y<<"l1"<<l1<<"r1"<<r1<<"start"<<start<<"back"<<back<<"guide"<<guide<<"dpadUp"<<dpadUp<<"dpadDown"<<dpadDown<<"dpadLeft"<<dpadLeft<<"dpadRight"<<dpadRight;
    if (a != m_lastA) { m_lastA = a; emit buttonAChanged(a); }
    if (b != m_lastB) { m_lastB = b; emit buttonBChanged(b); }
    if (x != m_lastX) { m_lastX = x; emit buttonXChanged(x); }
    if (y != m_lastY) { m_lastY = y; emit buttonYChanged(y); }
    if (l1 != m_lastL1) { m_lastL1 = l1; emit buttonL1Changed(l1); }
    if (r1 != m_lastR1) { m_lastR1 = r1; emit buttonR1Changed(r1); }
    if (start != m_lastStart) { m_lastStart = start; emit buttonStartChanged(start); }
    if (back != m_lastBack) { m_lastBack = back; emit buttonBackChanged(back); }
    if (guide != m_lastGuide) { m_lastGuide = guide; emit buttonGuideChanged(guide); }
    if (dpadUp != m_lastDpadUp) { m_lastDpadUp = dpadUp; emit buttonDpadUpChanged(dpadUp); }
    if (dpadDown != m_lastDpadDown) { m_lastDpadDown = dpadDown; emit buttonDpadDownChanged(dpadDown); }
    if (dpadLeft != m_lastDpadLeft) { m_lastDpadLeft = dpadLeft; emit buttonDpadLeftChanged(dpadLeft); }
    if (dpadRight != m_lastDpadRight) { m_lastDpadRight = dpadRight; emit buttonDpadRightChanged(dpadRight); }
}
