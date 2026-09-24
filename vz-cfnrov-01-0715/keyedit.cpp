#include "keyedit.h"
#include <QDebug>


KeyEdit::KeyEdit(QWidget *parent) :
    QLineEdit(parent){

    if(1==1){
        mapKey[Qt::Key_Escape] = "Esc";
        mapKey[Qt::Key_Tab] = "Tab";
        mapKey[Qt::Key_Backtab] = "Tab";
        mapKey[Qt::Key_Backspace] = "退格";
        mapKey[Qt::Key_Return] = "enter";
        mapKey[Qt::Key_Enter] = "enter(小键盘)";
        mapKey[Qt::Key_Insert] = "插入";
        mapKey[Qt::Key_Delete] = "删除";
        mapKey[Qt::Key_Pause] = "Pause/Break";
        mapKey[Qt::Key_Print] = "打印键";
        mapKey[Qt::Key_Home] = "Home键";
        mapKey[Qt::Key_End] = "End键";
        mapKey[Qt::Key_Left] = "←";
        mapKey[Qt::Key_Up] = "↑";
        mapKey[Qt::Key_Right] = "→";
        mapKey[Qt::Key_Down] = "↓";
        mapKey[Qt::Key_PageUp] = "Page Up";
        mapKey[Qt::Key_PageDown] = "Page Down";
        mapKey[Qt::Key_Shift] = "Shift";
        mapKey[Qt::Key_Control] = "Ctrl";
        mapKey[Qt::Key_Alt] = "Alt";
        mapKey[Qt::Key_AltGr] = " Alt（右）";
        mapKey[Qt::Key_CapsLock] = "大写锁定";
        mapKey[Qt::Key_NumLock] = "数字锁定";
        mapKey[Qt::Key_ScrollLock] = "卷动锁定";
        mapKey[Qt::Key_Menu] = "菜单";
        //mapKey[Qt::Key_Space] = "空格";

        mapKey[Qt::Key_nobreakspace] = "不换行空格";
        mapKey[Qt::Key_exclamdown] = "! 惊叹号";
        mapKey[Qt::Key_cent] = "美分键";
        mapKey[Qt::Key_sterling] = "英镑键";
        mapKey[Qt::Key_currency] = "货币键";
        mapKey[Qt::Key_yen] = "日元键";
        mapKey[Qt::Key_Back] = "后退键";
        mapKey[Qt::Key_Forward] = "前进键";
        mapKey[Qt::Key_Stop] = "停止键";
        mapKey[Qt::Key_Refresh] = "刷新键";
        mapKey[Qt::Key_VolumeDown] = "音量-";
        mapKey[Qt::Key_VolumeMute] = "静音";
        mapKey[Qt::Key_VolumeUp] = "音量+";
        mapKey[Qt::Key_HomePage] = "首页键";
        mapKey[Qt::Key_Favorites] = "收藏键";
        mapKey[Qt::Key_Search] = "搜索键";
    }

}

void KeyEdit::keyPressEvent(QKeyEvent *event){
    //qDebug() << tr("MyLineEdit类的keypress事件已触发，键盘按下");
    //QLineEdit::keyPressEvent(event);//执行
    //event->ignore();//输出后，才将继续执行主窗口的keypressEvent事件

    //qDebug() << "key press = " << event->key() << "text :" << event->text();

    int _keycode = event->key();
    if(_keycode > 0x20 && _keycode <= 0x7e){
        this->code = _keycode;
        this->setText(event->text());
    }else if(_keycode >= Qt::Key_F1 && _keycode <= Qt::Key_F35){
        this->code = _keycode;
        this->setText("F" + QString::number(_keycode - Qt::Key_F1 + 1));
    }else{
        QString strKey = mapKey.value(_keycode);
        if(strKey != ""){
            this->code = _keycode;
            this->setText(strKey);
        }
    }
}
