#ifndef SLIDETOUNLOCK_H
#define SLIDETOUNLOCK_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPropertyAnimation>

class SlideToUnlock : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int sliderPos READ sliderPos WRITE setSliderPos NOTIFY sliderPosChanged)

public:
    enum Action { Arm, Disarm };

    /// @brief 创建并初始化 SlideToUnlock 对象。
    explicit SlideToUnlock(Action action, QWidget *parent = nullptr);

    /// @brief 执行“sliderPos”对应的业务操作。
    int sliderPos() const { return m_sliderPos; }
    /// @brief 设置“SliderPos”并同步相关状态。
    void setSliderPos(int pos);
    /// @brief 设置“AlignY”并同步相关状态。
    void setAlignY(int y);

signals:
    /// @brief 通知订阅者“sliderPos”已经变化。
    void sliderPosChanged(int pos);
    /// @brief 执行“actionTriggered”对应的业务操作。
    void actionTriggered();

protected:
    /// @brief 处理“paint”事件。
    void paintEvent(QPaintEvent *event) override;
    /// @brief 处理“mousePress”事件。
    void mousePressEvent(QMouseEvent *event) override;
    /// @brief 处理“mouseMove”事件。
    void mouseMoveEvent(QMouseEvent *event) override;
    /// @brief 处理“mouseRelease”事件。
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Action m_action;
    int m_sliderPos;
    int m_startX;
    bool m_dragging;
    QPropertyAnimation *m_anim;

    /// @brief 执行“sliderWidth”对应的业务操作。
    int sliderWidth() const { return height() - 4; }
    /// @brief 执行“maxPos”对应的业务操作。
    int maxPos() const { return width() - sliderWidth() - 2; }
};

#endif
