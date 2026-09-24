#ifndef ACTIVEMAP_H
#define ACTIVEMAP_H

#include <QString>

struct ActiveMap{
    QString addr = "";
    int port = 0;

    int padLT_up = 0;
    int padLT_down = 0;
    int padLT_left = 0;
    int padLT_right = 0;
    QString sPadLT_up = "";
    QString sPadLT_down = "";
    QString sPadLT_left = "";
    QString sPadLT_right = "";

    int padLB_up = 0;
    int padLB_down = 0;
    int padLB_left = 0;
    int padLB_right = 0;
    QString sPadLB_up = "";
    QString sPadLB_down = "";
    QString sPadLB_left = "";
    QString sPadLB_right = "";

    int padRT_up = 0;
    int padRT_down = 0;
    int padRT_left = 0;
    int padRT_right = 0;
    QString sPadRT_up = "";
    QString sPadRT_down = "";
    QString sPadRT_left = "";
    QString sPadRT_right = "";

    int padRB_up = 0;
    int padRB_down = 0;
    int padRB_left = 0;
    int padRB_right = 0;
    QString sPadRB_up = "";
    QString sPadRB_down = "";
    QString sPadRB_left = "";
    QString sPadRB_right = "";

    int frontCamera = 0;
    int frontLed0 = 0;
    int frontLed1 = 0;
    QString sFrontCamera = "";
    QString sFrontLed0 = "";
    QString sFrontLed1 = "";

    int backCamera = 0;
    int backLed0 = 0;
    int backLed1 = 0;
    QString sBackCamera = "";
    QString sBackLed0 = "";
    QString sBackLed1 = "";
};

struct Attitude{
    float heading = 0.0;
    float pitch = 0.0;
    float roll = 0.0;

    float altitude = 0.0;

    float speed_heading = 0.0;
    float speed_altitude = 0.0;

    int staYaw = 0;     // Y轴旋转状态，+：左转；-：右转；
    int staRoll = 0;    // Z轴翻滚状态，+：左转；-：右转；
    int staPitch = 0;   // X轴翻滚状态，+：上翻；-：下翻；

    int staSurge = 0;   // Z轴前后移动状态，+：forward;-:backward；
    int staSway = 0;    // X轴左右移动状态，+：左移；-：右移；
    int staHeave = 0;   // Y轴上下移动状态，+：上移；-：下移；
};

#endif // ACTIVEMAP_H
