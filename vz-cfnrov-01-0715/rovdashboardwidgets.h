#ifndef ROVDASHBOARDWIDGETS_H
#define ROVDASHBOARDWIDGETS_H

#include <QWidget>
#include <array>
#include <QDebug>
#include <QCoreApplication>
#include<QTimer>
#define DEBUG qDebug() << __FILE__ << __LINE__
class ROVAttitudeGauge : public QWidget
{
    Q_OBJECT
public:
    enum GaugeType { Pitch, Roll, Heading };
    /// @brief 创建并初始化 ROVAttitudeGauge 对象。
    explicit ROVAttitudeGauge(GaugeType type, QWidget *parent = nullptr);
    /// @brief 执行“datainit”对应的业务操作。
    void datainit();
    /// @brief 设置数值并同步相关状态。
    void setValue(float value);
    /// @brief 设置数值、横滚并同步相关状态。
    void setValue_roll(float value);
    /// @brief 设置数值、俯仰并同步相关状态。
    void setValue_pitch(float value);
    /// @brief 设置数值、航向并同步相关状态。
    void setValue_yaw(float value);
    /// @brief 设置数值、深度并同步相关状态。
    void setValue_Depth(float depthMeters);
    /// @brief 设置数值、速度并同步相关状态。
    void setValue_Speed(float depthMeters);
    //姿态仪
    /// @brief 绘制俯仰。
    void drawPicAis_pitch(QPainter *painter);//pitch
    /// @brief 绘制横滚。
    void drawPicAis_roll(QPainter *painter);//roll
    /// @brief 绘制横滚。
    void drawPicAis_rollScale(QPainter *painter);//roll刻度标
    //航向表
    /// @brief 绘制“PiHeadingbg”。
    void drawPiHeadingbg(QPainter *painter);//航向表背景
    /// @brief 执行“countDisceStep”对应的业务操作。
    double countDisceStep(double maxValue,  double minValue,double scalLength,double drawValue);
    //高度和速度表
    /// @brief 绘制速度。
    void drawAltSpeedContent(QPainter *painter);
protected:
    /// @brief 处理“paint”事件。
    void paintEvent(QPaintEvent *) override;
public slots:
    /// @brief 根据最新数据更新数值。
    void updateValue();
private:
    GaugeType m_type;
    float m_value = 0.0f;
    //姿态仪
    QPixmap picAisbg;              //背景图
    QPixmap picAis2;            //水平仪
    QPixmap picAisroll;         //刻度图
    QPixmap picAis3;           //指针图
    //航向表
    QPixmap picHeadingbg;
    double minValue;           //最小值
    double maxValue;
    double precision =0;                 //精确度,小数点后几位
    double startAngle = 0;                //开始旋转角度
    double endAngle   = 0;                //结束旋转角度
    double  m_currentRoll=0;             //当前roll
    double  m_currentPitch=0;            //当前pitch
    double  m_currentYaw=0;              //当前值yaw
    double  m_currentDepth=0;              //当前值高度（深度）
    double  m_currentSpeed=0;            //当前值速度
    int picscal=2;
    QTimer *timer;
    double widgetscal=280;
};

class ROVDepthGauge : public QWidget
{
    Q_OBJECT
public:
    /// @brief 创建并初始化 ROVDepthGauge 对象。
    explicit ROVDepthGauge(QWidget *parent = nullptr);
    /// @brief 设置深度并同步相关状态。
    void setDepth(float depthMeters);
protected:
    /// @brief 处理“paint”事件。
    void paintEvent(QPaintEvent *) override;
private:
    float m_depth = 0.0f;
};

class ROVThrusterGauge : public QWidget
{
    Q_OBJECT
public:
    /// @brief 创建并初始化 ROVThrusterGauge 对象。
    explicit ROVThrusterGauge(QWidget *parent = nullptr);
    /// @brief 设置PWM并同步相关状态。
    void setPwmValues(const uint16_t values[16], uint8_t count);
protected:
    /// @brief 处理“paint”事件。
    void paintEvent(QPaintEvent *) override;
private:
    std::array<int, 6> m_percent{{0, 0, 0, 0, 0, 0}};
};

#endif
