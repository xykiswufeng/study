#ifndef JOYSTICKTHUMBPAD_HPP
#define JOYSTICKTHUMBPAD_HPP

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

class JoystickThumbPad : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal xAxis READ xAxis NOTIFY xAxisChanged)
    Q_PROPERTY(qreal yAxis READ yAxis NOTIFY yAxisChanged)
    Q_PROPERTY(bool yAxisPositiveRangeOnly READ yAxisPositiveRangeOnly WRITE setYAxisPositiveRangeOnly)
    Q_PROPERTY(bool yAxisReCenter READ yAxisReCenter WRITE setYAxisReCenter)
    Q_PROPERTY(bool lightColors READ lightColors WRITE setLightColors)
    Q_PROPERTY(QColor fgColor READ fgColor WRITE setFgColor)
    Q_PROPERTY(QColor bgColor READ bgColor WRITE setBgColor)
    Q_PROPERTY(qreal xPos READ xPos WRITE setXPos)
    Q_PROPERTY(qreal yPos READ yPos WRITE setYPos)
    Q_PROPERTY(bool leftHandedMode READ leftHandedMode WRITE setLeftHandedMode)
    Q_PROPERTY(bool isLeftStick READ isLeftStick WRITE setIsLeftStick)

public:
    /// @brief 创建并初始化 JoystickThumbPad 对象。
    explicit JoystickThumbPad(QWidget *parent = nullptr);
    /// @brief 停止后台任务并释放 JoystickThumbPad 占用的资源。
    ~JoystickThumbPad();

    // 属性访问器
    /// @brief 执行“xAxis”对应的业务操作。
    qreal xAxis() const { return m_xAxis; }
    /// @brief 执行“yAxis”对应的业务操作。
    qreal yAxis() const { return m_yAxis; }
    /// @brief 执行“yAxisPositiveRangeOnly”对应的业务操作。
    bool yAxisPositiveRangeOnly() const { return m_yAxisPositiveRangeOnly; }
    /// @brief 执行“yAxisReCenter”对应的业务操作。
    bool yAxisReCenter() const { return m_yAxisReCenter; }
    /// @brief 执行补光灯对应的业务操作。
    bool lightColors() const { return m_lightColors; }
    /// @brief 执行颜色对应的业务操作。
    QColor fgColor() const { return m_fgColor; }
    /// @brief 执行颜色对应的业务操作。
    QColor bgColor() const { return m_bgColor; }
    /// @brief 执行模式对应的业务操作。
    bool leftHandedMode() const { return m_leftHandedMode; }
    /// @brief 判断“LeftStick”是否处于有效状态。
    bool isLeftStick() const { return m_isLeftStick; }

    /// @brief 设置“YAxisPositiveRangeOnly”并同步相关状态。
    void setYAxisPositiveRangeOnly(bool enabled);
    /// @brief 设置“YAxisReCenter”并同步相关状态。
    void setYAxisReCenter(bool enabled);
    /// @brief 设置补光灯并同步相关状态。
    void setLightColors(bool light);
    /// @brief 设置颜色并同步相关状态。
    void setFgColor(const QColor &color);
    /// @brief 设置颜色并同步相关状态。
    void setBgColor(const QColor &color);
    /// @brief 设置模式并同步相关状态。
    void setLeftHandedMode(bool enabled);
    /// @brief 设置“IsLeftStick”并同步相关状态。
    void setIsLeftStick(bool isLeft);

    // 公共方法
    /// @brief 执行“reCenter”对应的业务操作。
    void reCenter(bool animated = true);
    /// @brief 执行摇杆对应的业务操作。
    void resizeJoystick(qreal yPositionAfterResize);
    /// @brief 判断“Pressed”是否处于有效状态。
    bool isPressed() const { return m_processTouchPoints || m_touchActive; }
    /// @brief 设置“XPos”并同步相关状态。
    void setXPos(qreal x);
    /// @brief 设置“YPos”并同步相关状态。
    void setYPos(qreal y);
    /// @brief 执行“xPos”对应的业务操作。
    qreal xPos() const { return m_stickPosition.x(); }
    /// @brief 执行“yPos”对应的业务操作。
    qreal yPos() const { return m_stickPosition.y(); }

public slots:
    /// @brief 设置位置并同步相关状态。
    void setStickPosition(const QPointF &pos);

signals:
    /// @brief 通知订阅者“xAxis”已经变化。
    void xAxisChanged(qreal value);
    /// @brief 通知订阅者“yAxis”已经变化。
    void yAxisChanged(qreal value);
    /// @brief 执行“stickMoved”对应的业务操作。
    void stickMoved(qreal x, qreal y);
    /// @brief 执行“stickPressed”对应的业务操作。
    void stickPressed();
    /// @brief 执行“stickReleased”对应的业务操作。
    void stickReleased();

protected:
    /// @brief 处理“paint”事件。
    void paintEvent(QPaintEvent *event) override;
    /// @brief 处理窗口尺寸变化并重新调整子控件。
    void resizeEvent(QResizeEvent *event) override;

    // 鼠标事件
    /// @brief 处理“mousePress”事件。
    void mousePressEvent(QMouseEvent *event) override;
    /// @brief 处理“mouseMove”事件。
    void mouseMoveEvent(QMouseEvent *event) override;
    /// @brief 处理“mouseRelease”事件。
    void mouseReleaseEvent(QMouseEvent *event) override;

    // 触摸事件
    /// @brief 执行事件对应的业务操作。
    bool event(QEvent *event) override;
    /// @brief 处理“touch”事件。
    void touchEvent(QTouchEvent *event);

private:
    // 属性
    qreal m_xAxis;
    qreal m_yAxis;
    bool m_yAxisPositiveRangeOnly;
    bool m_yAxisReCenter;
    bool m_lightColors;
    QColor m_fgColor;
    QColor m_bgColor;

    // 状态
    QPointF m_stickPosition;
    QPointF m_centerPosition;
    qreal m_hatWidth;
    qreal m_hatWidthHalf;
    bool m_processTouchPoints;
    bool m_alreadyCreated;
    bool m_calculateYAxisMutex;
    bool m_leftHandedMode = false;
    bool m_isLeftStick = false;  // 标识是否为左摇杆

    // 触摸点
    QPointF m_touchPoint;
    bool m_touchActive;

    // 动画
    QPropertyAnimation *m_xAnimation;
    QPropertyAnimation *m_yAnimation;
    QParallelAnimationGroup *m_animationGroup;
    QPointF m_animStickPosition;
    Q_PROPERTY(QPointF animStickPosition READ animStickPosition WRITE setAnimStickPosition)

    /// @brief 执行位置对应的业务操作。
    QPointF animStickPosition() const { return m_animStickPosition; }
    /// @brief 设置位置并同步相关状态。
    void setAnimStickPosition(const QPointF &pos);

    // 图片资源
    QPixmap m_bezelImage;          // 外圈图片
    QPixmap m_arrowUpImage;        // 上箭头
    QPixmap m_arrowDownImage;      // 下箭头
    QPixmap m_arrowLeftImage;      // 左箭头（逆时针）
    QPixmap m_arrowRightImage;     // 右箭头（顺时针）

    // 辅助方法
    /// @brief 执行“loadImages”对应的业务操作。
    void loadImages();
    /// @brief 计算“XAxis”。
    void calculateXAxis();
    /// @brief 计算“YAxis”。
    void calculateYAxis();
    /// @brief 执行“yAxisReCentered”对应的业务操作。
    qreal yAxisReCentered();
    /// @brief 根据最新数据更新位置。
    void updateStickPosition(const QPointF &pos, bool forceUpdate = false);
    /// @brief 执行位置对应的业务操作。
    QPointF clampStickPosition(const QPointF &pos);
    /// @brief 绘制“Bezel”。
    void drawBezel(QPainter &painter);
    /// @brief 绘制后台。
    void drawBackground(QPainter &painter);
    /// @brief 绘制“Arrows”。
    void drawArrows(QPainter &painter);
    /// @brief 绘制“Hat”。
    void drawHat(QPainter &painter);
    /// @brief 绘制“Arrow”。
    void drawArrow(QPainter &painter, const QPointF &center, qreal size, qreal rotation);
    /// @brief 绘制图像。
    void drawColoredImage(QPainter &painter, const QPixmap &pixmap, const QPointF &center, qreal size);

    // 常量
    static constexpr qreal ANIMATION_DURATION = 150;
};

#endif // JOYSTICKTHUMBPAD_HPP
