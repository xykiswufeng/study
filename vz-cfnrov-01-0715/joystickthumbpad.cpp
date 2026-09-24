#include "joystickthumbpad.h"
#include <QTouchDevice>
#include <QPainterPath>
#include <QtMath>
#include <QDebug>
#include <QTimer>
#define DEBUG qDebug() << __FILE__ << __LINE__
JoystickThumbPad::JoystickThumbPad(QWidget *parent)
    : QWidget(parent)
    , m_xAxis(0)
    , m_yAxis(0)
    , m_yAxisPositiveRangeOnly(true)
    , m_yAxisReCenter(false)
    , m_lightColors(false)
    , m_fgColor(Qt::white)
    , m_bgColor(40, 40, 40)
    , m_processTouchPoints(false)
    , m_alreadyCreated(false)
    , m_calculateYAxisMutex(true)
    , m_touchActive(false)
    , m_animStickPosition(0, 0)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(100, 100);

    m_hatWidth = 40;
    m_hatWidthHalf = m_hatWidth / 2;
    loadImages();

    m_xAnimation = new QPropertyAnimation(this, "xPos");
    m_yAnimation = new QPropertyAnimation(this, "yPos");

    m_animationGroup = new QParallelAnimationGroup(this);
    m_animationGroup->addAnimation(m_xAnimation);
    m_animationGroup->addAnimation(m_yAnimation);

    connect(m_animationGroup, &QParallelAnimationGroup::finished, [this]() {
        m_stickPosition = QPointF(width() / 2.0, height() / 2.0);
        calculateXAxis();
        calculateYAxis();
        update();
    });

    m_stickPosition = QPointF(0, 0);
}

JoystickThumbPad::~JoystickThumbPad()
{
}

void JoystickThumbPad::loadImages()
{
    // 箭头图片 - QGC 使用的 SVG 图标
    m_arrowUpImage.load(":/btn/pic/chevron-up.svg");
    m_arrowDownImage.load(":/btn/pic/chevron-down.svg");
    m_arrowLeftImage.load(":/btn/pic/counter-clockwise-arrow.svg");
    m_arrowRightImage.load(":/btn/pic/clockwise-arrow.svg");

    // 如果图片加载失败，设置默认颜色模式
    if (m_bezelImage.isNull()) {
        DEBUG << "JoystickThumbPad: Failed to load bezel image, will draw manually";
    }
}

// 添加单独的 X 和 Y 属性
void JoystickThumbPad::setXPos(qreal x)
{
    m_animStickPosition.setX(x);
    m_stickPosition.setX(x);
    update();
}

void JoystickThumbPad::setYPos(qreal y)
{
    m_animStickPosition.setY(y);
    m_stickPosition.setY(y);
    update();
}

void JoystickThumbPad::setYAxisPositiveRangeOnly(bool enabled)
{
    if (m_yAxisPositiveRangeOnly != enabled) {
        m_yAxisPositiveRangeOnly = enabled;
        calculateYAxis();
        update();
    }
}

void JoystickThumbPad::setYAxisReCenter(bool enabled)
{
    if (m_yAxisReCenter != enabled) {
        m_yAxisReCenter = enabled;
        yAxisReCentered();
        update();
    }
}

void JoystickThumbPad::setLightColors(bool light)
{
    if (m_lightColors != light) {
        m_lightColors = light;
        update();
    }
}

void JoystickThumbPad::setFgColor(const QColor &color)
{
    m_fgColor = color;
    update();
}

void JoystickThumbPad::setBgColor(const QColor &color)
{
    m_bgColor = color;
    update();
}

void JoystickThumbPad::setLeftHandedMode(bool enabled)
{
    if (m_leftHandedMode != enabled) {
        m_leftHandedMode = enabled;
        update();  // 重绘以更新箭头位置
    }
}

void JoystickThumbPad::setIsLeftStick(bool isLeft)
{
    if (m_isLeftStick != isLeft) {
        m_isLeftStick = isLeft;
        update();  // 重绘以更新箭头显示
    }
}
//回中
void JoystickThumbPad::reCenter(bool animated)
{
    if (width() < 10 || height() < 10) {
        return;
    }

    m_processTouchPoints = false;
    m_centerPosition = QPointF(width() / 2.0, height() / 2.0);
    QPointF targetPos = m_centerPosition;

    if (animated) {
        // 停止当前动画
        if (m_animationGroup->state() == QAbstractAnimation::Running) {
            m_animationGroup->stop();
        }

        // 重置动画属性
        m_animStickPosition = m_stickPosition;

        // 重新创建动画以确保正确工作
        delete m_xAnimation;
        delete m_yAnimation;
        delete m_animationGroup;

        m_xAnimation = new QPropertyAnimation(this, "xPos");
        m_yAnimation = new QPropertyAnimation(this, "yPos");
        m_animationGroup = new QParallelAnimationGroup(this);

        m_xAnimation->setDuration(ANIMATION_DURATION);
        m_yAnimation->setDuration(ANIMATION_DURATION);
        m_xAnimation->setEasingCurve(QEasingCurve::OutCubic);
        m_yAnimation->setEasingCurve(QEasingCurve::OutCubic);

        // 设置动画值 - 注意：QPropertyAnimation 对 QPointF 需要分别设置 X 和 Y
        m_xAnimation->setStartValue(m_stickPosition.x());
        m_xAnimation->setEndValue(targetPos.x());
        m_yAnimation->setStartValue(m_stickPosition.y());
        m_yAnimation->setEndValue(targetPos.y());

        m_animationGroup->addAnimation(m_xAnimation);
        m_animationGroup->addAnimation(m_yAnimation);

        connect(m_animationGroup, &QParallelAnimationGroup::finished, this, [this]() {
            // 动画完成后确保位置精确在中心
            m_stickPosition = QPointF(width() / 2.0, height() / 2.0);
            calculateXAxis();
            calculateYAxis();
            update();
            emit stickMoved(m_xAxis, m_yAxis);
        });

        m_animationGroup->start();
    } else {
        // 直接设置位置
        m_stickPosition = targetPos;
        calculateXAxis();
        calculateYAxis();
        update();
        emit stickMoved(m_xAxis, m_yAxis);
    }
}

void JoystickThumbPad::resizeJoystick(qreal yPositionAfterResize)
{
    if (height() <= 0) {
            return;
        }

        m_calculateYAxisMutex = false;

        qreal fullRange = m_yAxisPositiveRangeOnly ? 1.0 : 2.0;
        qreal normalizedY = (yPositionAfterResize + (m_yAxisPositiveRangeOnly ? 0 : 1)) / fullRange;
        m_stickPosition.setY((1.0 - normalizedY) * height());
        m_stickPosition.setX(width() / 2.0);

        m_calculateYAxisMutex = true;
        calculateYAxis();
        update();
}

void JoystickThumbPad::setStickPosition(const QPointF &pos)
{
    DEBUG<<"第一个";
    updateStickPosition(pos, true);
}

qreal JoystickThumbPad::yAxisReCentered()
{
//    // 始终将摇杆设置在中心
//    m_yAxis = m_yAxisPositiveRangeOnly ? 0.5 : 0;
//    m_stickPosition = QPointF(width() / 2.0, height() / 2.0);

//    m_alreadyCreated = true;
//    emit yAxisChanged(m_yAxis);
//    update();

//    return m_yAxis;
    if (m_yAxisReCenter) {
            m_yAxis = m_yAxisPositiveRangeOnly ? 0.5 : 0;
            m_stickPosition = QPointF(width() / 2.0, height() / 2.0);
        }

        if (!m_alreadyCreated && !m_yAxisReCenter) {
            m_yAxis = m_yAxisPositiveRangeOnly ? 0 : -1;
            m_stickPosition = QPointF(width() / 2.0, height());
        }

        if (m_alreadyCreated && !m_yAxisReCenter) {
            m_yAxis = m_yAxisPositiveRangeOnly ? 0.5 : 0;
            m_stickPosition = QPointF(width() / 2.0, height() / 2.0);
        }

        m_alreadyCreated = true;
        emit yAxisChanged(m_yAxis);
        update();

        return m_yAxis;
}

void JoystickThumbPad::setAnimStickPosition(const QPointF &pos)
{
    if (m_animStickPosition != pos) {
        m_animStickPosition = pos;
        // 注意：这里应该直接更新位置，而不是再次触发计算
        // 因为动画会持续调用此函数
        m_stickPosition = pos;
        update();
    }
}

//X轴变化
void JoystickThumbPad::calculateXAxis()
{
    if (!isVisible()) {
        return;
    }

    // 直接使用 m_stickPosition
    qreal newXAxis = (m_stickPosition.x() / width()) * 2.0 - 1.0;
    newXAxis = qBound(-1.0, newXAxis, 1.0);
    //浮点数值对比
    if (!qFuzzyCompare(m_xAxis, newXAxis)) {
        m_xAxis = newXAxis;
        // DEBUG<<m_xAxis;
        emit xAxisChanged(m_xAxis);
        emit stickMoved(m_xAxis, m_yAxis);
    }
}
//Y轴变化
void JoystickThumbPad::calculateYAxis()
{
    if (!isVisible() || !m_calculateYAxisMutex) {
            return;
        }
      //  DEBUG<<m_yAxisPositiveRangeOnly;
        qreal fullRange = m_yAxisPositiveRangeOnly ? 1.0 : 2.0;
        qreal pctUp = 1.0 - (m_stickPosition.y() / height());
        qreal rangeUp = pctUp * fullRange;

        if (!m_yAxisPositiveRangeOnly) {
            rangeUp -= 1.0;
        }

        qreal newYAxis = qBound(-1.0, rangeUp, 1.0);
        if (m_yAxisPositiveRangeOnly) {
            newYAxis = qBound(0.0, rangeUp, 1.0);
        }

        if (!qFuzzyCompare(m_yAxis, newYAxis)) {
            m_yAxis = newYAxis;
            //DEBUG<<m_yAxis;
            emit yAxisChanged(m_yAxis);
            emit stickMoved(m_xAxis, m_yAxis);
        }
}

void JoystickThumbPad::updateStickPosition(const QPointF &pos, bool forceUpdate)
{
    QPointF clampedPos = clampStickPosition(pos);

    if (forceUpdate || m_processTouchPoints) {
        m_stickPosition = clampedPos;
        calculateXAxis();
        calculateYAxis();
        update();
    }
}

QPointF JoystickThumbPad::clampStickPosition(const QPointF &pos)
{
    return QPointF(
                qBound(0.0, pos.x(), static_cast<qreal>(width())),
                qBound(0.0, pos.y(), static_cast<qreal>(height()))
                );
}

void JoystickThumbPad::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBezel(painter);
    drawBackground(painter);
    drawArrows(painter);
    drawHat(painter);
}

void JoystickThumbPad::drawBezel(QPainter &painter)
{
    if (!m_bezelImage.isNull()) {
        // 使用加载的图片
        QRectF targetRect(0, 0, width(), height());
        painter.drawPixmap(targetRect.toRect(), m_bezelImage);
    } else {
        // 回退到手动绘制
        QRadialGradient gradient(width() / 2, height() / 2, width() / 2);
        gradient.setColorAt(0, QColor(60, 60, 60, 100));
        gradient.setColorAt(1, QColor(20, 20, 20, 150));

        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(rect());
    }
}

void JoystickThumbPad::drawBackground(QPainter &painter)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(m_bgColor.red(), m_bgColor.green(), m_bgColor.blue(), 128));
    painter.drawEllipse(rect());

    // 绘制内圈
    painter.setPen(QPen(m_fgColor, 2));
    painter.setBrush(Qt::NoBrush);

    qreal innerMargin = width() / 4.0;
    QRectF innerRect = rect().adjusted(innerMargin, innerMargin, -innerMargin, -innerMargin);
    painter.drawEllipse(innerRect);

    // 绘制外圈边框
    painter.drawEllipse(rect().adjusted(1, 1, -1, -1));
}

void JoystickThumbPad::drawArrows(QPainter &painter)
{
    // 箭头显示在油门摇杆上（非左手模式=左杆，左手模式=右杆）
    bool isThrottleStick = (m_leftHandedMode && !m_isLeftStick) || (!m_leftHandedMode && m_isLeftStick);

    if (!isThrottleStick) {
        return;  // 不显示箭头
    }

    qreal arrowSize = m_hatWidth * 0.6;
    qreal margin = m_hatWidth * 0.8;

    bool hasImages = !m_arrowUpImage.isNull() && !m_arrowDownImage.isNull() &&
            !m_arrowLeftImage.isNull() && !m_arrowRightImage.isNull();

    if (hasImages) {
        // 显示所有四个方向的箭头
        // 右侧箭头
        QPointF rightCenter(width() - margin, height() / 2);
        drawColoredImage(painter, m_arrowRightImage, rightCenter, arrowSize);

        // 左侧箭头
        QPointF leftCenter(margin, height() / 2);
        drawColoredImage(painter, m_arrowLeftImage, leftCenter, arrowSize);

        // 上箭头
        QPointF topCenter(width() / 2, margin);
        drawColoredImage(painter, m_arrowUpImage, topCenter, arrowSize);

        // 下箭头
        QPointF bottomCenter(width() / 2, height() - margin);
        drawColoredImage(painter, m_arrowDownImage, bottomCenter, arrowSize);

    } else {
        // 回退到手动绘制箭头
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_fgColor);

        // 右侧箭头
        QPointF rightCenter(width() - margin, height() / 2);
        drawArrow(painter, rightCenter, arrowSize, 0);

        // 左侧箭头
        QPointF leftCenter(margin, height() / 2);
        drawArrow(painter, leftCenter, arrowSize, 180);

        // 上箭头
        QPointF topCenter(width() / 2, margin);
        drawArrow(painter, topCenter, arrowSize, 90);

        // 下箭头
        QPointF bottomCenter(width() / 2, height() - margin);
        drawArrow(painter, bottomCenter, arrowSize, 270);
    }
}

void JoystickThumbPad::drawArrow(QPainter &painter, const QPointF &center, qreal size, qreal rotation)
{
    painter.save();
    painter.translate(center);
    painter.rotate(rotation);

    QPolygonF arrow;
    arrow << QPointF(0, -size/2) << QPointF(size/2, size/4) << QPointF(-size/2, size/4);

    painter.drawPolygon(arrow);
    painter.restore();
}

void JoystickThumbPad::drawColoredImage(QPainter &painter, const QPixmap &pixmap, const QPointF &center, qreal size)
{
    if (pixmap.isNull()) {
        return;
    }

    QRectF targetRect(center.x() - size/2, center.y() - size/2, size, size);

    // 如果需要着色，可以使用 QPainter::CompositionMode
    // 这里简化处理，直接绘制
    painter.drawPixmap(targetRect.toRect(), pixmap);
}

void JoystickThumbPad::drawHat(QPainter &painter)
{
    QPointF pos = m_animationGroup->state() == QAbstractAnimation::Running ? m_animStickPosition : m_stickPosition;

    QRectF hatRect(pos.x() - m_hatWidthHalf, pos.y() - m_hatWidthHalf, m_hatWidth, m_hatWidth);

    // 绘制阴影
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 80));
    painter.drawEllipse(hatRect.adjusted(2, 2, 2, 2));

    // 绘制摇杆帽
    QRadialGradient gradient(hatRect.center(), m_hatWidthHalf);
    gradient.setColorAt(0, QColor(m_fgColor.lighter(150).red(), m_fgColor.lighter(150).green(), m_fgColor.lighter(150).blue(), 128));
    gradient.setColorAt(0.7, QColor(m_fgColor.red(), m_fgColor.green(), m_fgColor.blue(), 128));
    gradient.setColorAt(1, QColor(m_fgColor.darker(150).red(), m_fgColor.darker(150).green(), m_fgColor.darker(150).blue(), 128));

    painter.setBrush(gradient);
    painter.setPen(QPen(QColor(m_fgColor.darker(200).red(), m_fgColor.darker(200).green(), m_fgColor.darker(200).blue(), 128), 1));
    painter.drawEllipse(hatRect);
}

void JoystickThumbPad::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    m_centerPosition = QPointF(width() / 2.0, height() / 2.0);
    m_hatWidth = qMin(width(), height()) / 5.0;
    m_hatWidthHalf = m_hatWidth / 2.0;

    if (!m_alreadyCreated) {
        // 初始位置设置在中心
        m_stickPosition = m_centerPosition;
        yAxisReCentered();
    }
}

void JoystickThumbPad::mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton) {
        m_processTouchPoints = true;
        m_touchActive = true;
        m_touchPoint = event->localPos();  // Qt 5 使用 localPos()

        m_centerPosition = QPointF(width() / 2.0, height() / 2.0);
         DEBUG<<"第er个";
        updateStickPosition(m_touchPoint, true);
        DEBUG << "[JoystickThumbPad] ========== MOUSE PRESS ==========";
        DEBUG << "  Position:" << event->localPos();
        DEBUG << "  Widget size:" << width() << "x" << height();
        emit stickPressed();
        event->accept();
    }
}

void JoystickThumbPad::mouseMoveEvent(QMouseEvent *event)
{

    if (m_processTouchPoints && (event->buttons() & Qt::LeftButton))
    {

        m_touchPoint = event->localPos();  // Qt 5 使用 localPos()
         //DEBUG<<"第san个";
        updateStickPosition(m_touchPoint, false);
        event->accept();
    }
}

void JoystickThumbPad::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_processTouchPoints) {
        m_processTouchPoints = false;
        m_touchActive = false;

        QPointF startPos = m_stickPosition;
        QPointF endPos = QPointF(width() / 2.0, height() / 2.0);

        QPropertyAnimation *xAnim = new QPropertyAnimation(this, "xPos");
        xAnim->setDuration(150);
        xAnim->setStartValue(startPos.x());
        xAnim->setEndValue(endPos.x());
        xAnim->setEasingCurve(QEasingCurve::OutCubic);

        QPropertyAnimation *yAnim = new QPropertyAnimation(this, "yPos");
        yAnim->setDuration(150);
        yAnim->setStartValue(startPos.y());
        yAnim->setEndValue(endPos.y());
        yAnim->setEasingCurve(QEasingCurve::OutCubic);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(xAnim);
        group->addAnimation(yAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);

        connect(group, &QParallelAnimationGroup::finished, this, [this]() {
            m_stickPosition = QPointF(width() / 2.0, height() / 2.0);
            calculateXAxis();
            calculateYAxis();
            update();
        });

        emit stickReleased();
        event->accept();
    }
}

bool JoystickThumbPad::event(QEvent *event)
{
    if (event->type() == QEvent::TouchBegin ||
            event->type() == QEvent::TouchUpdate ||
            event->type() == QEvent::TouchEnd) {
        touchEvent(static_cast<QTouchEvent*>(event));
        return true;
    }
    return QWidget::event(event);
}

void JoystickThumbPad::touchEvent(QTouchEvent *event)
{
    const QList<QTouchEvent::TouchPoint> &touchPoints = event->touchPoints();

    if (touchPoints.isEmpty()) {
        return;
    }

    switch (event->type()) {
    case QEvent::TouchBegin:
        if (!m_touchActive) {
            m_processTouchPoints = true;
            m_touchActive = true;
            m_touchPoint = touchPoints.first().pos();
            m_centerPosition = QPointF(width() / 2.0, height() / 2.0);
             DEBUG<<"第si个";
            updateStickPosition(m_touchPoint, true);
            emit stickPressed();
        }
        break;

    case QEvent::TouchUpdate:
        if (m_processTouchPoints) {
            m_touchPoint = touchPoints.first().pos();
             DEBUG<<"第wu个";
            updateStickPosition(m_touchPoint, false);
        }
        break;

    case QEvent::TouchEnd:
        if (m_processTouchPoints) {
            m_processTouchPoints = false;
            m_touchActive = false;
            reCenter(true);
            emit stickReleased();
        }
        break;

    default:
        break;
    }

    event->accept();
}
