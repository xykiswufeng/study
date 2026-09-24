#include "slidetounlock.h"
#include <QPainter>
#include <QFontMetrics>
#include <QDebug>
#define DEBUG qDebug() << __FILE__ << __LINE__
// 解锁与上锁共用一个滑动控件，通过 Action 决定颜色和提示文案。
SlideToUnlock::SlideToUnlock(Action action, QWidget *parent)
    : QWidget(parent)
    , m_action(action)
    , m_sliderPos(2)
    , m_startX(0)
    , m_dragging(false)
{
    setFixedHeight(56);
    setMinimumWidth(180);
    setMaximumWidth(16777215);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);

    m_anim = new QPropertyAnimation(this, "sliderPos");
    m_anim->setDuration(200);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
}

// 将滑块位置限制在轨道内，并通知动画与绘制逻辑刷新。
void SlideToUnlock::setSliderPos(int pos)
{
    pos = qMax(2, qMin(pos, maxPos()));
    if (m_sliderPos != pos) {
        m_sliderPos = pos;
        emit sliderPosChanged(pos);
        update();
    }
}

void SlideToUnlock::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int r = h / 2;

    QLinearGradient background(0, 0, 0, h);
    background.setColorAt(0, QColor(17, 37, 49));
    background.setColorAt(1, QColor(8, 20, 29));
    p.setPen(QPen(QColor(48, 76, 92), 1));
    p.setBrush(background);
    p.drawRoundedRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0), r, r);

    const QColor actionColor = (m_action == Arm) ? QColor(35, 164, 122)
                                                  : QColor(190, 67, 80);
    QColor progressColor = actionColor;
    progressColor.setAlpha(m_sliderPos >= maxPos() ? 190 : 75);
    p.setPen(Qt::NoPen);
    p.setBrush(progressColor);
    p.drawRoundedRect(3, 3, m_sliderPos + sliderWidth() - 3, h - 6, r - 3, r - 3);

    QLinearGradient sliderGradient(0, 2, 0, h - 2);
    sliderGradient.setColorAt(0, actionColor.lighter(125));
    sliderGradient.setColorAt(1, actionColor.darker(115));
    p.setPen(QPen(actionColor.lighter(150), 1));
    p.setBrush(sliderGradient);
    p.drawRoundedRect(m_sliderPos, 3, sliderWidth(), h - 6, r - 3, r - 3);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    int cx = m_sliderPos + sliderWidth() / 2;
    int cy = h / 2;
    int as = 8;
    int aw = as * 2 / 3;
    QPolygon arrowHead;
    arrowHead << QPoint(cx - aw, cy - as)
              << QPoint(cx + aw, cy)
              << QPoint(cx - aw, cy + as);
    p.drawPolygon(arrowHead);
    int shaftW = aw * 2;
    int shaftH = as * 2 / 3;
    p.drawRect(cx - shaftW - aw, cy - shaftH / 2, shaftW, shaftH);

    p.setPen(QColor(205, 222, 231));
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(h * 0.20);
    font.setWeight(QFont::DemiBold);
    p.setFont(font);
    QString title = (m_action == Arm) ? QString::fromUtf8("滑动解锁") : QString::fromUtf8("滑动锁定");
    QString hint = QString::fromUtf8("  向右确认");
    QRect textRect(sliderWidth() + 10, 0, w - sliderWidth() * 2 - 20, h);
    p.drawText(textRect, Qt::AlignCenter, title + hint);
}

// 只有从滑块本体按下左键才开始拖动，防止点击轨道直接触发操作。
void SlideToUnlock::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int sw = sliderWidth();
        DEBUG<<height() - 4;
        DEBUG<<sw;
        DEBUG<<m_sliderPos;
        QRect sliderRect(m_sliderPos, 2, sw, height() - 4);
        if (sliderRect.contains(event->pos())) {
            m_dragging = true;
            m_startX = event->pos().x() - m_sliderPos;
            DEBUG<<m_startX;
        }
    }
}

void SlideToUnlock::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        int newPos = event->pos().x() - m_startX;
        setSliderPos(newPos);
    }
}

// 释放时仅在抵达终点后发出动作信号，然后把滑块动画复位。
void SlideToUnlock::mouseReleaseEvent(QMouseEvent *)
{
    if (m_dragging) {
        m_dragging = false;
        if (m_sliderPos >= maxPos()) {
            emit actionTriggered();
        }
        m_anim->stop();
        m_anim->setStartValue(m_sliderPos);
        m_anim->setEndValue(2);
        m_anim->start();
    }
}
