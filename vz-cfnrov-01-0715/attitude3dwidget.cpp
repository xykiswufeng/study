#include "attitude3dwidget.h"

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QLinearGradient>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QVector3D>
#include <QVector4D>
#include <QWheelEvent>

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct DrawableFace
{
    QPolygonF polygon;
    QColor color;
    float depth = 0.0f;
};

float degreesToRadians(float degrees)
{
    return degrees * kPi / 180.0f;
}

void addBox(QVector<QVector3D> &vertices, QVector<Attitude3DFace> &faces,
            const QVector3D &center, const QVector3D &size, const QColor &color)
{
    const QVector3D h = size * 0.5f;
    const int first = vertices.size();
    vertices << center + QVector3D(-h.x(), -h.y(), -h.z())
             << center + QVector3D( h.x(), -h.y(), -h.z())
             << center + QVector3D( h.x(),  h.y(), -h.z())
             << center + QVector3D(-h.x(),  h.y(), -h.z())
             << center + QVector3D(-h.x(), -h.y(),  h.z())
             << center + QVector3D( h.x(), -h.y(),  h.z())
             << center + QVector3D( h.x(),  h.y(),  h.z())
             << center + QVector3D(-h.x(),  h.y(),  h.z());

    const auto addFace = [&](std::initializer_list<int> localIndices, const QColor &faceColor) {
        Attitude3DFace face;
        for (int index : localIndices)
            face.indices.append(first + index);
        face.color = faceColor;
        faces.append(face);
    };

    addFace({0, 3, 2, 1}, color.darker(145));
    addFace({4, 5, 6, 7}, color.lighter(125));
    addFace({0, 1, 5, 4}, color.darker(115));
    addFace({3, 7, 6, 2}, color);
    addFace({0, 4, 7, 3}, color.darker(135));
    addFace({1, 2, 6, 5}, color.lighter(110));
}

QPointF projectPoint(const QVector3D &worldPoint, const QMatrix4x4 &view,
                     const QRectF &viewport, float cameraDistance,
                     float *depthOut = nullptr)
{
    Q_UNUSED(cameraDistance);
    const QVector4D cameraPoint = view * QVector4D(worldPoint, 1.0f);
    const float depth = -cameraPoint.z();
    if (depthOut)
        *depthOut = depth;

    // 使用固定焦距，模型在大画布中更醒目，改变相机距离时也能产生真实缩放效果。
    const float focal = qMin(viewport.width(), viewport.height()) * 1.28f;
    const float safeDepth = qMax(depth, 0.05f);
    return QPointF(viewport.center().x() + cameraPoint.x() * focal / safeDepth,
                   viewport.center().y() - cameraPoint.y() * focal / safeDepth);
}

QColor shadedColor(const QColor &base, const QVector3D &normal)
{
    const QVector3D light = QVector3D(-0.35f, -0.45f, 1.0f).normalized();
    const float illumination = 0.52f + 0.48f * qAbs(QVector3D::dotProduct(normal, light));
    QColor result = base;
    result.setRedF(qBound(0.0, base.redF() * illumination, 1.0));
    result.setGreenF(qBound(0.0, base.greenF() * illumination, 1.0));
    result.setBlueF(qBound(0.0, base.blueF() * illumination, 1.0));
    return result;
}

} // namespace

Attitude3DWidget::Attitude3DWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("attitude3DWidget"));
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(QString::fromUtf8("拖动鼠标旋转观察视角，滚轮缩放，双击恢复默认视角"));

    m_importModelButton = new QPushButton(QString::fromUtf8("＋ 添加模型"), this);
    m_importModelButton->setFixedSize(104, 25);
    m_importModelButton->setCursor(Qt::PointingHandCursor);
    m_importModelButton->setToolTip(QString::fromUtf8("导入 STEP/STP 模型"));
    m_importModelButton->setStyleSheet(QStringLiteral(
        "QPushButton { color: #d9edf7; background: rgba(26, 65, 86, 220);"
        " border: 1px solid #4c829d; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #276f91; border-color: #71bddc; }"
        "QPushButton:pressed { background: #17465e; }"));
    connect(m_importModelButton, &QPushButton::clicked,
            this, [this]() { chooseModelFile(); });

    loadInitialModel();
}

void Attitude3DWidget::setAttitude(float rollDeg, float pitchDeg, float yawDeg)
{
    m_rollDeg = rollDeg;
    m_pitchDeg = pitchDeg;
    m_yawDeg = yawDeg;
    m_hasAttitude = true;
    update();
}

QSize Attitude3DWidget::minimumSizeHint() const
{
    return QSize(260, 190);
}

void Attitude3DWidget::loadInitialModel()
{
    QSettings settings(QStringLiteral("wz-724"), QStringLiteral("wz-724"));
    const QString savedMesh = settings.value(QStringLiteral("attitude3d/meshPath")).toString();
    QString error;
    if (!savedMesh.isEmpty() && QFileInfo::exists(savedMesh) &&
        loadMeshFile(savedMesh, &error)) {
        m_modelName = settings.value(QStringLiteral("attitude3d/modelName"),
                                     QFileInfo(savedMesh).completeBaseName()).toString();
        return;
    }

    if (loadMeshFile(QStringLiteral(":/models/ROV_03_realtime.wzmesh"), &error))
        m_modelName = QString::fromUtf8("装配体ROV_03");
}

QString Attitude3DWidget::findStepConverter() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/wz-step-converter.exe"),
        QDir(appDir).absoluteFilePath(QStringLiteral("../tools/step_converter_dist/wz-step-converter.exe")),
        QDir::current().absoluteFilePath(QStringLiteral("tools/step_converter_dist/wz-step-converter.exe"))
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
    }
    return QString();
}

bool Attitude3DWidget::convertStepFile(const QString &stepPath, QString *meshPath,
                                       QString *errorMessage)
{
    const QString converter = findStepConverter();
    if (converter.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(
                "缺少 wz-step-converter.exe，无法将 STEP 曲面转换为显示网格。");
        }
        return false;
    }

    const QFileInfo sourceInfo(stepPath);
    const QByteArray cacheSource =
        (sourceInfo.canonicalFilePath() + QString::number(sourceInfo.size()) +
         sourceInfo.lastModified().toString(Qt::ISODateWithMs)).toUtf8();
    const QString cacheKey =
        QString::fromLatin1(QCryptographicHash::hash(cacheSource, QCryptographicHash::Sha1).toHex());
    const QString modelDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        QStringLiteral("/wz-724/models");
    if (!QDir().mkpath(modelDir)) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("无法创建模型缓存目录：%1").arg(modelDir);
        return false;
    }

    const QString outputPath =
        QDir(modelDir).filePath(sourceInfo.completeBaseName() +
                                QStringLiteral("-") + cacheKey.left(12) +
                                QStringLiteral(".wzmesh"));
    if (!QFileInfo::exists(outputPath)) {
        QProcess converterProcess;
        converterProcess.setProgram(converter);
        converterProcess.setArguments({
            stepPath, outputPath,
            QStringLiteral("--deflection"), QStringLiteral("10.0"),
            QStringLiteral("--angle"), QStringLiteral("0.9")
        });

        QProgressDialog progress(QString::fromUtf8("正在读取并三角化 STEP 模型…"),
                                 QString::fromUtf8("取消"), 0, 0, this);
        progress.setWindowTitle(QString::fromUtf8("添加三维模型"));
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        converterProcess.start();
        if (!converterProcess.waitForStarted(5000)) {
            if (errorMessage)
                *errorMessage = QString::fromUtf8("无法启动 STEP 转换器：%1")
                                    .arg(converterProcess.errorString());
            return false;
        }

        while (!converterProcess.waitForFinished(80)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            if (progress.wasCanceled()) {
                converterProcess.kill();
                converterProcess.waitForFinished();
                if (errorMessage)
                    *errorMessage = QString::fromUtf8("已取消模型导入。");
                return false;
            }
        }
        progress.close();

        if (converterProcess.exitStatus() != QProcess::NormalExit ||
            converterProcess.exitCode() != 0) {
            QString detail = QString::fromUtf8(converterProcess.readAllStandardError()).trimmed();
            if (detail.isEmpty())
                detail = QString::fromUtf8(converterProcess.readAllStandardOutput()).trimmed();
            if (errorMessage) {
                *errorMessage = QString::fromUtf8("STEP 模型转换失败。%1")
                                    .arg(detail.isEmpty() ? QString() :
                                         QStringLiteral("\n") + detail);
            }
            return false;
        }
    }

    *meshPath = outputPath;
    return true;
}

bool Attitude3DWidget::loadMeshFile(const QString &meshPath, QString *errorMessage)
{
    QFile file(meshPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("无法打开模型网格：%1").arg(file.errorString());
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    char magic[8] = {};
    if (stream.readRawData(magic, 8) != 8 ||
        QByteArray(magic, 8) != QByteArrayLiteral("WZ3DMESH")) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("模型网格格式无效。");
        return false;
    }

    quint32 version = 0;
    quint32 vertexCount = 0;
    quint32 faceCount = 0;
    stream >> version >> vertexCount >> faceCount;
    if (version != 1 || vertexCount == 0 || faceCount == 0 ||
        vertexCount > 2000000 || faceCount > 5000000) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("模型网格版本或数据规模无效。");
        return false;
    }

    QVector<QVector3D> vertices;
    vertices.reserve(static_cast<int>(vertexCount));
    QVector3D minimum;
    QVector3D maximum;
    for (quint32 i = 0; i < vertexCount; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        stream >> x >> y >> z;
        const QVector3D vertex(x, y, z);
        vertices.append(vertex);
        if (i == 0) {
            minimum = maximum = vertex;
        } else {
            minimum.setX(qMin(minimum.x(), x));
            minimum.setY(qMin(minimum.y(), y));
            minimum.setZ(qMin(minimum.z(), z));
            maximum.setX(qMax(maximum.x(), x));
            maximum.setY(qMax(maximum.y(), y));
            maximum.setZ(qMax(maximum.z(), z));
        }
    }

    QVector<Attitude3DFace> faces;
    faces.reserve(static_cast<int>(faceCount));
    for (quint32 i = 0; i < faceCount; ++i) {
        quint32 a = 0;
        quint32 b = 0;
        quint32 c = 0;
        quint8 red = 0;
        quint8 green = 0;
        quint8 blue = 0;
        quint8 alpha = 0;
        stream >> a >> b >> c >> red >> green >> blue >> alpha;
        if (a >= vertexCount || b >= vertexCount || c >= vertexCount) {
            if (errorMessage)
                *errorMessage = QString::fromUtf8("模型网格包含越界索引。");
            return false;
        }
        Attitude3DFace face;
        face.indices << static_cast<int>(a) << static_cast<int>(b) << static_cast<int>(c);
        face.color = QColor(red, green, blue, alpha);
        faces.append(face);
    }

    if (stream.status() != QDataStream::Ok) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("模型网格数据不完整。");
        return false;
    }

    const QVector3D center = (minimum + maximum) * 0.5f;
    const QVector3D extent = maximum - minimum;
    const float largestExtent = qMax(extent.x(), qMax(extent.y(), extent.z()));
    if (largestExtent <= 0.00001f) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("模型尺寸无效。");
        return false;
    }
    const float normalizationScale = 3.25f / largestExtent;
    for (QVector3D &vertex : vertices) {
        const QVector3D normalized = (vertex - center) * normalizationScale;
        // 根据实物方向校准：此前看到的是底面，因此原模型 -Z 才是上。
        // 原模型 -Y 为前、-X 为左、-Z 为上。
        // 转成姿态控件的显示坐标：+X 向前、+Y 向左、+Z 向上。
        vertex = QVector3D(-normalized.y(), -normalized.x(), -normalized.z());
    }

    m_modelVertices.swap(vertices);
    m_modelFaces.swap(faces);
    m_modelName = QFileInfo(meshPath).completeBaseName();
    update();
    return true;
}

bool Attitude3DWidget::loadModelFile(const QString &filePath, QString *errorMessage)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    QString meshPath = filePath;
    if (suffix == QStringLiteral("step") || suffix == QStringLiteral("stp")) {
        if (!convertStepFile(filePath, &meshPath, errorMessage))
            return false;
    } else if (suffix != QStringLiteral("wzmesh")) {
        if (errorMessage)
            *errorMessage = QString::fromUtf8("请选择 STEP、STP 或 WZMESH 模型文件。");
        return false;
    }

    if (!loadMeshFile(meshPath, errorMessage))
        return false;

    m_modelName = QFileInfo(filePath).completeBaseName();
    QSettings settings(QStringLiteral("wz-724"), QStringLiteral("wz-724"));
    settings.setValue(QStringLiteral("attitude3d/meshPath"), meshPath);
    settings.setValue(QStringLiteral("attitude3d/modelName"), m_modelName);
    return true;
}

void Attitude3DWidget::chooseModelFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, QString::fromUtf8("添加三维模型"), QString(),
        QString::fromUtf8("STEP 模型 (*.step *.stp);;wz-724 网格 (*.wzmesh)"));
    if (filePath.isEmpty())
        return;

    QString error;
    if (!loadModelFile(filePath, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("模型导入失败"), error);
        return;
    }
    QMessageBox::information(
        this, QString::fromUtf8("模型导入完成"),
        QString::fromUtf8("已加载模型：%1\n模型将继续跟随 Pixhawk 姿态旋转。")
            .arg(m_modelName));
}

void Attitude3DWidget::resetView()
{
    // 从模型 -X 方向看向原点，使 +Z（“上”）朝屏幕上方，
    // +Y（“左”）朝屏幕左侧，+X（“前”）垂直指向屏幕内。
    m_viewAzimuthDeg = 180.0f;
    m_viewElevationDeg = 0.0f;
    m_cameraDistance = 7.2f;
    update();
}

void Attitude3DWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, QColor(15, 31, 43));
    background.setColorAt(0.58, QColor(10, 22, 31));
    background.setColorAt(1.0, QColor(5, 12, 18));
    painter.fillRect(rect(), background);
    painter.setPen(QPen(QColor(59, 91, 111), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    QRectF viewport = rect().adjusted(8, 32, -8, -42);
    if (viewport.width() < 20 || viewport.height() < 20)
    {
        return;
    }
    const float azimuth = degreesToRadians(m_viewAzimuthDeg);
    const float elevation = degreesToRadians(m_viewElevationDeg);
    const float horizontalDistance = m_cameraDistance * std::cos(elevation);
    const QVector3D camera(horizontalDistance * std::cos(azimuth),
                           horizontalDistance * std::sin(azimuth),
                           m_cameraDistance * std::sin(elevation));
    // 使用球面轨道的切向量作为相机上方向，越过正上方/正下方时也不会奇异，
    // 因而鼠标可以连续进行完整的 360° 纵向观察。
    const QVector3D cameraUp(-std::sin(elevation) * std::cos(azimuth),
                             -std::sin(elevation) * std::sin(azimuth),
                             std::cos(elevation));
    QMatrix4x4 view;
    view.lookAt(camera, QVector3D(0, 0, 0), cameraUp);

    // Fixed world grid makes attitude changes easy to judge.
    painter.save();
    painter.setClipRect(viewport);
    const float gridZ = -1.05f;
    for (int i = -4; i <= 4; ++i) {
        const bool major = (i == 0);
        painter.setPen(QPen(major ? QColor(70, 112, 137, 170)
                                 : QColor(43, 73, 91, 115),
                            major ? 1.2 : 0.8));
        painter.drawLine(projectPoint(QVector3D(-4, i, gridZ), view, viewport, m_cameraDistance),
                         projectPoint(QVector3D( 4, i, gridZ), view, viewport, m_cameraDistance));
        painter.drawLine(projectPoint(QVector3D(i, -4, gridZ), view, viewport, m_cameraDistance),
                         projectPoint(QVector3D(i,  4, gridZ), view, viewport, m_cameraDistance));
    }

    QMatrix4x4 attitude;
    // Pixhawk uses aerospace/NED signs. Convert them to the z-up model space.
    attitude.rotate(-m_yawDeg, 0, 0, 1);
    attitude.rotate(-m_pitchDeg, 0, 1, 0);

    // Pixhawk 的 roll 正方向与当前模型坐标系相反，只对 3D 模型翻滚取反。
    attitude.rotate(m_rollDeg, 1, 0, 0);
    QVector<QVector3D> fallbackVertices;
    QVector<Attitude3DFace> fallbackFaces;
    if (m_modelVertices.isEmpty() || m_modelFaces.isEmpty()) {
        addBox(fallbackVertices, fallbackFaces, QVector3D(0.0f, 0.0f, 0.0f),
               QVector3D(2.45f, 1.25f, 0.65f), QColor(47, 155, 202));
        addBox(fallbackVertices, fallbackFaces, QVector3D(-0.15f, -0.98f, 0.42f),
               QVector3D(2.25f, 0.35f, 0.36f), QColor(231, 162, 45));
        addBox(fallbackVertices, fallbackFaces, QVector3D(-0.15f, 0.98f, 0.42f),
               QVector3D(2.25f, 0.35f, 0.36f), QColor(231, 162, 45));
        addBox(fallbackVertices, fallbackFaces, QVector3D(-0.35f, 0.0f, 0.57f),
               QVector3D(0.95f, 0.8f, 0.35f), QColor(118, 198, 224));
        addBox(fallbackVertices, fallbackFaces, QVector3D(-0.65f, -0.78f, -0.38f),
               QVector3D(0.48f, 0.48f, 0.48f), QColor(53, 64, 71));
        addBox(fallbackVertices, fallbackFaces, QVector3D(-0.65f, 0.78f, -0.38f),
               QVector3D(0.48f, 0.48f, 0.48f), QColor(53, 64, 71));
        addBox(fallbackVertices, fallbackFaces, QVector3D(0.72f, -0.72f, -0.32f),
               QVector3D(0.42f, 0.42f, 0.52f), QColor(53, 64, 71));
        addBox(fallbackVertices, fallbackFaces, QVector3D(0.72f, 0.72f, -0.32f),
               QVector3D(0.42f, 0.42f, 0.52f), QColor(53, 64, 71));
    }
    const QVector<QVector3D> &modelVertices =
        m_modelVertices.isEmpty() ? fallbackVertices : m_modelVertices;
    const QVector<Attitude3DFace> &modelFaces =
        m_modelFaces.isEmpty() ? fallbackFaces : m_modelFaces;

    QVector<QVector3D> worldVertices;
    worldVertices.reserve(modelVertices.size());
    for (const QVector3D &vertex : modelVertices)
        worldVertices.append((attitude * QVector4D(vertex, 1.0f)).toVector3D());

    QVector<DrawableFace> drawableFaces;
    drawableFaces.reserve(modelFaces.size());
    for (const Attitude3DFace &face : modelFaces) {
        DrawableFace drawable;
        QVector<QVector3D> faceWorld;
        for (int index : face.indices) {
            float depth = 0.0f;
            const QVector3D point = worldVertices.at(index);
            drawable.polygon << projectPoint(point, view, viewport, m_cameraDistance, &depth);
            drawable.depth += depth;
            faceWorld.append(point);
        }
        drawable.depth /= qMax(1, face.indices.size());
        if (faceWorld.size() >= 3) {
            const QVector3D normal = QVector3D::crossProduct(faceWorld[1] - faceWorld[0],
                                                             faceWorld[2] - faceWorld[0]).normalized();
            drawable.color = shadedColor(face.color, normal);
        } else {
            drawable.color = face.color;
        }
        drawableFaces.append(drawable);
    }

    std::sort(drawableFaces.begin(), drawableFaces.end(),
              [](const DrawableFace &a, const DrawableFace &b) {
        return a.depth > b.depth;
    });

    for (const DrawableFace &face : drawableFaces) {
        painter.setBrush(face.color);
        // STEP 网格由大量三角面组成。绘制轮廓会形成密集“铁丝网”，
        // 实体模式不画三角边，只保留光照明暗来表现曲面和体积。
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(face.polygon);
    }

    // 机体轴：红色 = 前，绿色 = 左，青色 = 上。
    const QVector3D origin = (attitude * QVector4D(0, 0, 0, 1)).toVector3D();
    struct Axis { QVector3D end; QColor color; QString label; };
    const Axis axes[] = {
        {QVector3D(2.15f, 0, 0), QColor(255, 75, 60), QString::fromUtf8("前")},
        {QVector3D(0, 1.75f, 0), QColor(92, 221, 123), QString::fromUtf8("左")},
        {QVector3D(0, 0, 1.55f), QColor(76, 205, 255), QString::fromUtf8("上")}
    };
    const QPointF origin2D = projectPoint(origin, view, viewport, m_cameraDistance);
    QFont axisFont = painter.font();
    axisFont.setPixelSize(10);
    axisFont.setBold(true);
    painter.setFont(axisFont);
    for (const Axis &axis : axes) {
        const QVector3D endWorld = (attitude * QVector4D(axis.end, 1)).toVector3D();
        const QPointF end2D = projectPoint(endWorld, view, viewport, m_cameraDistance);
        painter.setPen(QPen(axis.color, 2.2));
        painter.drawLine(origin2D, end2D);
        painter.setPen(axis.color);
        painter.drawText(end2D + QPointF(4, -3), axis.label);
    }
    painter.restore();

    // Header and telemetry overlay.
    QFont titleFont = painter.font();
    titleFont.setPixelSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor(218, 235, 244));
    painter.drawText(QRectF(12, 7, qMax(80, width() - 145), 20),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     m_modelName.isEmpty()
                         ? QString::fromUtf8("PIXHAWK 3D 姿态")
                         : QString::fromUtf8("PIXHAWK 3D 姿态 · %1").arg(m_modelName));

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_hasAttitude ? QColor(40, 205, 112) : QColor(240, 173, 47));
    painter.drawEllipse(QPointF(width() - 118, 16), 4, 4);

    QFont telemetryFont = painter.font();
    telemetryFont.setPixelSize(11);
    telemetryFont.setBold(false);
    painter.setFont(telemetryFont);
    painter.setPen(QColor(194, 216, 228));
    const QString attitudeText = m_hasAttitude
        ? QStringLiteral("ROLL %1°   PITCH %2°   YAW %3°")
              .arg(m_rollDeg, 0, 'f', 1)
              .arg(m_pitchDeg, 0, 'f', 1)
              .arg(m_yawDeg, 0, 'f', 1)
        : QString::fromUtf8("等待 Pixhawk ATTITUDE 数据…");
    painter.drawText(QRectF(8, height() - 34, width() - 16, 18),
                     Qt::AlignHCenter | Qt::AlignVCenter, attitudeText);

    QFont hintFont = painter.font();
    hintFont.setPixelSize(9);
    painter.setFont(hintFont);
    painter.setPen(QColor(117, 149, 166));
    painter.drawText(QRectF(8, height() - 17, width() - 16, 13),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     QString::fromUtf8("拖动可360°旋转视角 · 滚轮缩放 · 双击复位"));
}

void Attitude3DWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Attitude3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        const QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_viewAzimuthDeg = std::fmod(m_viewAzimuthDeg + delta.x() * 0.45f, 360.0f);
        m_viewElevationDeg = std::fmod(m_viewElevationDeg - delta.y() * 0.35f, 360.0f);
        update();
        event->accept();
        return;
    }
    unsetCursor();
    QWidget::mouseMoveEvent(event);
}

void Attitude3DWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resetView();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void Attitude3DWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_importModelButton) {
        m_importModelButton->move(qMax(4, width() - m_importModelButton->width() - 7), 4);
        m_importModelButton->raise();
    }
}

void Attitude3DWidget::wheelEvent(QWheelEvent *event)
{
    m_cameraDistance = qBound(4.5f,
                              m_cameraDistance - event->angleDelta().y() / 480.0f,
                              12.0f);
    update();
    event->accept();
}
