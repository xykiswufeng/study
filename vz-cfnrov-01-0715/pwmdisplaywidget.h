#ifndef PWM_DISPLAY_WIDGET_H
#define PWM_DISPLAY_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QPushButton>
#include <QMap>

class PwmDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    /// @brief 创建并初始化 PwmDisplayWidget 对象。
    explicit PwmDisplayWidget(QWidget *parent = nullptr);
    /// @brief 设置“Armed”并同步相关状态。
    void setArmed(bool armed);

signals:
    /// @brief 通知订阅者“reverse”已经变化。
    void reverseChanged(int channel, bool reversed);
    /// @brief 通知订阅者“enabled”已经变化。
    void enabledChanged(int channel, bool enabled);
    /// @brief 执行参数对应的业务操作。
    void pidParamSetRequested(const QString &paramId, float value);
    /// @brief 执行“pidSaveToEepromRequested”对应的业务操作。
    void pidSaveToEepromRequested();

public slots:
    /// @brief 根据最新数据更新PWM。
    void updatePwmValues(const uint16_t values[16], uint8_t count);
    /// @brief 通知订阅者已收到参数、数值。
    void onParamValueReceived(const QString &paramId, float value);

private slots:
    /// @brief 处理“ReverseCheckboxToggled”对应的事件或信号回调。
    void onReverseCheckboxToggled(int channel, bool checked);
    /// @brief 处理“EnabledCheckboxToggled”对应的事件或信号回调。
    void onEnabledCheckboxToggled(int channel, bool checked);
    /// @brief 处理点击对应的事件或信号回调。
    void onPidApplyClicked();
    /// @brief 处理点击对应的事件或信号回调。
    void onPidExportClicked();
    /// @brief 处理点击对应的事件或信号回调。
    void onPidImportClicked();

private:
    QLabel *m_valueLabels[6];
    QCheckBox *m_reverseCheckBoxes[6];
    QCheckBox *m_enabledCheckBoxes[6];
    bool m_motorReversed[6] = {false};
    int m_servoFunction[6] = {33, 34, 35, 36, 37, 38};
    bool m_armed = false;

    struct PidParam {
        QString paramId;
        QDoubleSpinBox *spinBox;
    };
    PidParam m_pidParams[10];
    QPushButton *m_pidApplyBtn = nullptr;
    QPushButton *m_pidExportBtn = nullptr;
    QPushButton *m_pidImportBtn = nullptr;
};

#endif
