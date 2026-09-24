#include "rovdashboardwidgets.h"
#include <QPainter>
#include <QtMath>
#include <cmath>

namespace {
QColor cyan() { return QColor(42, 216, 235); }
}

// 根据仪表类型创建姿态或航向显示控件。绘制使用项目内嵌图片，
// 控件尺寸由 Designer 中的占位区域决定，因此这里不固定实际显示宽高。
ROVAttitudeGauge::ROVAttitudeGauge(GaugeType type, QWidget *parent)
    : QWidget(parent), m_type(type)
{
    setMinimumSize(145, 90);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    datainit();
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateValue()));//数据从这边来的
    timer->start();
}
// 从 Qt 资源系统加载仪表素材，避免部署目录不同导致图片丢失。
// 航向角和姿态角的显示范围在这里统一初始化。
void ROVAttitudeGauge::datainit()
{
    minValue = 0;
    maxValue = 360;
    precision = 0;
    startAngle = 0;
    endAngle = 0;
    // The old ais_*.png paths do not exist in this project. The actual
    // horizon image is embedded so any build directory can display it.
    picAis2.load(QStringLiteral(":/qfi/images/rollpitch/FND_ATT.png"));
    picAisroll.load(QStringLiteral(":/qfi/images/rollpitch/FND_Roll.png"));
    picAis3.load(QStringLiteral(":/qfi/images/rollpitch/FND_SAN2.png"));
    picAisbg.load(QStringLiteral(":/qfi/images/rollpitch/FND_NAV_3.png"));
    picHeadingbg.load(QStringLiteral(":/qfi/images/heading/HeadingIndicatorScal.png"));
}

void ROVAttitudeGauge::setValue(float value)
{
    m_value = value;
    update();
}

void ROVAttitudeGauge::setValue_roll(float value)
{
    m_currentRoll=value;

}

void ROVAttitudeGauge::setValue_pitch(float value)
{
    m_currentPitch=value;

}

void ROVAttitudeGauge::setValue_yaw(float value)
{
    m_currentYaw=value;

}

void ROVAttitudeGauge::setValue_Depth(float depthMeters)
{
    m_currentDepth=depthMeters;
}
void ROVAttitudeGauge::setValue_Speed(float speed)
{

    m_currentSpeed=speed;
}


// 先按横滚角旋转地平线，再按俯仰角平移刻度图；外层 paintEvent
// 会限制可见区域，因而图片超出仪表边界的部分不会显示。
void ROVAttitudeGauge::drawPicAis_pitch(QPainter *painter)
{
    painter->save();
    painter->scale(1.5,1.5);
    painter->translate(0,0);
    painter->rotate(0);
    double steps = (maxValue-minValue);
    double angleStep = (360.0 - startAngle - endAngle) / steps;
    painter->rotate(startAngle);
    double degRotate = angleStep * (m_currentRoll - minValue);
    painter->rotate(-degRotate);
    //pitch
    double pitch =m_currentPitch;
    double drawPitchValue=pitch;
    double picHeight=640;//测试出来的
    double degtranslate=countDisceStep(90,-90,picHeight,drawPitchValue);
    painter->translate(0,degtranslate-picHeight/2);
    painter->drawPixmap(-picAis2.width()/(picscal*2),
                        -picAis2.height()/(picscal*2),
                        picAis2.width()/picscal,
                        picAis2.height()/picscal,
                        picAis2);
    painter->restore();
}

// 横滚刻度弧固定在仪表顶部，不随地平线图片一起旋转。
void ROVAttitudeGauge::drawPicAis_roll(QPainter *painter)
{
    painter->save();
    painter->translate(0, 18);
    painter->scale(1.05,1.05);
    painter->drawPixmap(-picAisroll.width()/(picscal*2),
                        -picAisroll.height()/(picscal*2),
                        picAisroll.width()/picscal,
                        picAisroll.height()/picscal,
                        picAisroll);
    painter->restore();
}

// 中央参考标记保持静止，为操作者提供姿态比较基准。
void ROVAttitudeGauge::drawPicAis_rollScale(QPainter *painter)
{
    painter->save();
    painter->translate(0,2);
    painter->scale(1.8,1.8);
    painter->drawPixmap(-picAisbg.width()/(picscal*2),
                        -picAisbg.height()/(picscal*2),
                        picAisbg.width()/picscal,
                        picAisbg.height()/picscal,
                        picAisbg);
    painter->restore();
}

// 航向表底环和中心参考线保持固定，仅旋转刻度图片。
// 刻度图中的方位字母随航向角移动，顶部数字显示当前绝对航向。
void ROVAttitudeGauge::drawPiHeadingbg(QPainter *painter)
{
    painter->save();
    QFont titleFont(QStringLiteral("Microsoft YaHei UI"));
    titleFont.setPixelSize(28);
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(QColor(118, 120, 133));
    painter->drawText(QRectF(-105, -120, 100, 100), Qt::AlignCenter,
                      QString::fromUtf8("航向"));
    titleFont.setPixelSize(34);
    painter->setFont(titleFont);
    painter->setPen(Qt::white);
    painter->drawText(QRectF(15, -120, 100, 100), Qt::AlignCenter,
                      QString::number(m_currentYaw, 'f', 0));

    // Keep the original semicircle proportions and text positions.
    painter->scale(1.5, 1.5);
    painter->translate(0, 70);
    painter->setPen(QPen(QColor(225, 236, 242), 0.9));
    painter->drawEllipse(QPointF(0, 0), 100, 100);
    painter->setPen(QPen(QColor(118, 120, 133), 0.9));
    painter->drawEllipse(QPointF(0, 0), 60, 60);
    painter->drawEllipse(QPointF(0, 0), 25, 25);
    painter->setPen(QPen(QColor(86, 118, 193), 0.9));
    painter->drawEllipse(QPointF(0, -7), 5, 5);
    painter->drawLine(QPointF(0, -12), QPointF(0, -95));

    // Rotate the original compass-scale artwork with the MAVLink heading.
    painter->save();
    painter->rotate(-m_currentYaw);
    const int scaleWidth = picHeadingbg.width() / picscal;
    const int scaleHeight = picHeadingbg.height() / picscal;
    painter->drawPixmap(-scaleWidth / 2, -scaleHeight / 2,
                        scaleWidth, scaleHeight, picHeadingbg);
    painter->restore();
    painter->restore();
}

double ROVAttitudeGauge::countDisceStep(double maxValue,  double minValue,double scalLength,double drawValue)
{
    double steps =0;//对应刻度值
    double  lengthStep=0;//对应刻度值
    double  lenthTranslate =0;//对应刻度值

    steps=maxValue - minValue;
    lengthStep=scalLength/ steps;
    if (drawValue <= minValue )
    {
        drawValue = minValue ;
    }
    else if (drawValue>=maxValue)
    {
        drawValue = maxValue;
    }
    lenthTranslate = lengthStep * (drawValue - minValue);
    return lenthTranslate;
}

void ROVAttitudeGauge::drawAltSpeedContent(QPainter *painter)
{
    painter->save();
    //文字
    QFont titleFont(QStringLiteral("Microsoft YaHei UI"));
    titleFont.setPixelSize(qBound(10, height() / 6, 30));
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(QColor(118, 120, 133));
   // painter->drawText(QRectF(-125, -50, 100, 100), Qt::AlignCenter, "深度");
    //painter->drawText(QRectF(-125, 0, 100, 100), Qt::AlignCenter, "速度");
    painter->setPen(QColor(79, 112, 245));
    QString strDepthStr=QString::number(m_currentDepth,'f',0)+"米";
    painter->drawText(QRectF(0, -50, 100, 100), Qt::AlignCenter, strDepthStr);
//    painter->setPen(QColor(253, 192, 65));
//    QString strSpeedStr=QString::number(m_currentSpeed,'f',0)+"km/h";
//    painter->drawText(QRectF(0, 0, 100, 100), Qt::AlignCenter, strSpeedStr);
    painter->restore();
}
// 使用 250×200 的设计坐标绘制，再等比缩放到实际控件大小；
// 这样在主界面改变分栏宽度时，两种仪表都不会被拉伸变形。
void ROVAttitudeGauge::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), QColor(9, 29, 39));
    if (width() <= 0 || height() <= 0)
        return;

    // Match the original 250 x 200 instrument drawing while fitting the
    // QWidget host in the Designer layout.
    const qreal fit = qMin(width() / 250.0, height() / 200.0);
    painter.translate(width() / 2.0, height() / 2.0);
    painter.scale(fit, fit);
    if (m_type == Pitch) {
        drawPicAis_pitch(&painter);
        drawPicAis_roll(&painter);
        drawPicAis_rollScale(&painter);
    } else if (m_type == Roll) {
        drawPiHeadingbg(&painter);
    }
}

void ROVAttitudeGauge::updateValue()
{
//    if(m_currentRoll>=360)
//    {
//        m_currentRoll=0;
//    }

//    m_currentRoll++;
//     update();

}

ROVDepthGauge::ROVDepthGauge(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(70);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void ROVDepthGauge::setDepth(float depthMeters)
{
    m_depth = qMax(0.0f, depthMeters);
    update();
}

// 深度刻度的零点位于表盘底部，深度增加时液柱向上延伸。
// 绘制结果只反映最近一次 setDepth 收到的有效深度值。
void ROVDepthGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    p.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::DemiBold));
    p.setPen(QColor(172, 211, 222));
    p.drawText(QRect(2, 5, width() - 4, 22), Qt::AlignCenter, QString::fromUtf8("深度"));
    const QRectF track(width() * 0.30, 39, width() * 0.23, height() - 85);
    p.setPen(QPen(QColor(46, 99, 115), 2));
    p.setBrush(QColor(5, 22, 30));
    p.drawRoundedRect(track, track.width() / 2, track.width() / 2);
    //xyk这里深度值显示没有问题，查看上面下来的数据有没有问题
    //m_depth=80;
    const qreal n = qBound(0.0, static_cast<double>(m_depth / 100.0f), 1.0);
    const qreal h = qMax(3.0, (track.height() - 6) * n);
   // qDebug()<<m_depth<<n<<h;
    QRectF fill(track.left() + 3, track.bottom() - h - 3, track.width() - 6, h);
    QLinearGradient gradient(fill.topLeft(), fill.bottomLeft());
    gradient.setColorAt(0, QColor(245, 185, 47));
    gradient.setColorAt(0.5, QColor(32, 211, 226));
    gradient.setColorAt(1, QColor(26, 117, 176));
    p.setPen(Qt::NoPen);
    p.setBrush(gradient);
    p.drawRoundedRect(fill, fill.width() / 2, fill.width() / 2);
    p.setFont(QFont(QStringLiteral("Consolas"), 7));
    p.setPen(QColor(119, 168, 181));
    for (int i = 0; i <= 5; ++i) {
        const qreal y = track.bottom() - track.height() * i / 5.0;
        p.drawLine(QPointF(track.right() + 2, y), QPointF(track.right() + 6, y));
        p.drawText(QRectF(track.right() + 7, y - 7, width() - track.right() - 8, 14),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(i * 20));
    }
    p.setFont(QFont(QStringLiteral("Consolas"), 10, QFont::Bold));
    p.setPen(QColor(223, 246, 249));
    p.drawText(QRect(1, height() - 38, width() - 2, 26), Qt::AlignCenter,
               QStringLiteral("%1m").arg(m_depth, 0, 'f', 1));
}

ROVThrusterGauge::ROVThrusterGauge(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(205, 90);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

// 将 PWM 相对中位值 1500 的偏差映射为 0～100% 的显示高度。
// 对缺失的通道按零处理，避免复用上一帧的推进器状态。
void ROVThrusterGauge::setPwmValues(const uint16_t values[16], uint8_t count)
{
    for (int i = 0; i < 6; ++i) {
        const int delta = i < count ? qAbs(static_cast<int>(values[i]) - 1500) : 0;
        m_percent[static_cast<size_t>(i)] = qBound(0, delta / 5, 100);
    }
    update();
}

// 每个推进器占据等宽槽位，柱条在槽位内居中；布局随面板宽度变化，
// 标题、柱条与通道编号分别保留独立的垂直空间。
void ROVThrusterGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    p.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::DemiBold));
    p.setPen(QColor(190, 222, 232));
    p.drawText(QRect(2, 7, width() - 4, 22), Qt::AlignCenter,
               QString::fromUtf8("推进器功率"));

    // 六路推进器各占一个等宽区域；柱条在各自区域居中，面板变宽时自动拉开间距。
    const qreal sideMargin = qMax(12.0, width() * 0.08);
    const qreal contentWidth = qMax(0.0, width() - sideMargin * 2.0);
    const qreal slotWidth = contentWidth / 6.0;
    const qreal barWidth = qBound(10.0, slotWidth * 0.34, 30.0);
    const qreal top = 39.0; // 给标题留出间隔，避免压在柱条上。
    const qreal barHeight = qMax(20.0, height() - top - 23.0);
    p.setFont(QFont(QStringLiteral("Consolas"), 8, QFont::Bold));
    for (int i = 0; i < 6; ++i) {
        const qreal x = sideMargin + (i + 0.5) * slotWidth - barWidth / 2.0;
        QRectF track(x, top, barWidth, barHeight);
        p.setPen(QPen(QColor(43, 93, 108), 1));
        p.setBrush(QColor(5, 22, 30));
        p.drawRoundedRect(track, 4, 4);
        const qreal filled = (barHeight - 4) * m_percent[static_cast<size_t>(i)] / 100.0;
        p.setPen(Qt::NoPen);
        p.setBrush(cyan());
        p.drawRoundedRect(QRectF(x + 2, top + barHeight - filled - 2,
                                 barWidth - 4, filled), 3, 3);
        p.setPen(QColor(145, 194, 205));
        p.drawText(QRectF(x - 5, top + barHeight + 3, barWidth + 10, 16), Qt::AlignCenter,
                   QStringLiteral("T%1").arg(i + 1));
    }
}
