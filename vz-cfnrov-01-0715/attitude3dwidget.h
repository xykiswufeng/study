#ifndef ATTITUDE3DWIDGET_H
#define ATTITUDE3DWIDGET_H

#include <QColor>
#include <QPoint>
#include <QVector>
#include <QVector3D>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QPushButton;
class QResizeEvent;
class QWheelEvent;

struct Attitude3DFace
{
    QVector<int> indices;
    QColor color;
};

// Software-rendered 3D attitude view. It deliberately uses only QtGui/QtWidgets
// so the project does not depend on an additional Qt3D or OpenGL module.
class Attitude3DWidget : public QWidget
{
public:
    /// @brief 创建并初始化 Attitude3DWidget 对象。
    explicit Attitude3DWidget(QWidget *parent = nullptr);

    /// @brief 设置姿态并同步相关状态。
    void setAttitude(float rollDeg, float pitchDeg, float yawDeg);
    /// @brief 执行文件对应的业务操作。
    bool loadModelFile(const QString &filePath, QString *errorMessage = nullptr);
    /// @brief 执行尺寸对应的业务操作。
    QSize minimumSizeHint() const override;

protected:
    /// @brief 处理“paint”事件。
    void paintEvent(QPaintEvent *event) override;
    /// @brief 处理“mousePress”事件。
    void mousePressEvent(QMouseEvent *event) override;
    /// @brief 处理“mouseMove”事件。
    void mouseMoveEvent(QMouseEvent *event) override;
    /// @brief 处理“mouseDoubleClick”事件。
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    /// @brief 处理窗口尺寸变化并重新调整子控件。
    void resizeEvent(QResizeEvent *event) override;
    /// @brief 处理“wheel”事件。
    void wheelEvent(QWheelEvent *event) override;

private:
    /// @brief 执行文件对应的业务操作。
    void chooseModelFile();
    /// @brief 执行文件对应的业务操作。
    bool convertStepFile(const QString &stepPath, QString *meshPath,
                         QString *errorMessage);
    /// @brief 执行文件对应的业务操作。
    bool loadMeshFile(const QString &meshPath, QString *errorMessage);
    /// @brief 执行“findStepConverter”对应的业务操作。
    QString findStepConverter() const;
    /// @brief 执行“loadInitialModel”对应的业务操作。
    void loadInitialModel();
    /// @brief 重置“View”。
    void resetView();

    float m_rollDeg = 0.0f;
    float m_pitchDeg = 0.0f;
    float m_yawDeg = 0.0f;
    // 默认从机体后方正视：上方位于屏幕上侧，左侧位于屏幕左侧，
    // “前”轴垂直于屏幕。这两个角不参与 Pixhawk 姿态或模型轴变换。
    float m_viewAzimuthDeg = 180.0f;
    float m_viewElevationDeg = 0.0f;
    float m_cameraDistance = 7.2f;
    bool m_hasAttitude = false;
    QPoint m_lastMousePos;
    QVector<QVector3D> m_modelVertices;
    QVector<Attitude3DFace> m_modelFaces;
    QString m_modelName;
    QPushButton *m_importModelButton = nullptr;
};

#endif // ATTITUDE3DWIDGET_H
