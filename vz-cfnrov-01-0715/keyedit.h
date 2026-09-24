#ifndef KEYEDIT_H
#define KEYEDIT_H

#include <QLineEdit>
#include <QKeyEvent>

class KeyEdit : public QLineEdit{
    Q_OBJECT

public:
    /// @brief 创建并初始化 KeyEdit 对象。
    explicit KeyEdit(QWidget *parent = nullptr);

    QMap<int, QString> mapKey;
    int code = 0;

protected:
    /// @brief 处理键盘按下事件并更新控制指令。
    void keyPressEvent(QKeyEvent *event) override;

};

#endif // KEYEDIT_H
