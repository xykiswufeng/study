#ifndef DIALOGSETUP_H
#define DIALOGSETUP_H

#include <QDialog>
#include <QKeyEvent>
#include <QTimer>

#include "activemap.h"

class MavlinkManager;

namespace Ui {
class DialogSetup;
}

class DialogSetup : public QDialog
{
    Q_OBJECT

public:
    /// @brief 创建并初始化 DialogSetup 对象。
    explicit DialogSetup(QWidget *parent = nullptr, ActiveMap *map = nullptr,
                         MavlinkManager *mavlinkManager = nullptr);
    /// @brief 停止后台任务并释放 DialogSetup 占用的资源。
    ~DialogSetup();

    ActiveMap *map;
    /// @brief 设置数值并同步相关状态。
    void setMapValue();

protected:

private slots:

    /// @brief 处理按钮对应的事件或信号回调。
    void on_buttonBox_accepted();
    /// @brief 处理目标对应的事件或信号回调。
    void onAttTargetTimer();

    /// @brief 处理按钮、点击对应的事件或信号回调。
    void on_pushButton_attStart_clicked();
    /// @brief 处理按钮、点击对应的事件或信号回调。
    void on_pushButton_attStop_clicked();

private:
    Ui::DialogSetup *ui;
    MavlinkManager *m_mavlinkManager = nullptr;
    QTimer *m_attTargetTimer = nullptr;
    bool m_attTargetSending = false;
};

#endif // DIALOGSETUP_H
