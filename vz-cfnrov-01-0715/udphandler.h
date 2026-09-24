#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QString>

class UdpHandler : public QObject
{
    Q_OBJECT
public:
    /// @brief 创建并初始化 UdpHandler 对象。
    explicit UdpHandler(QObject *parent = nullptr);
    /// @brief 停止后台任务并释放 UdpHandler 占用的资源。
    ~UdpHandler();

    // 设置远程地址
    /// @brief 设置“RemoteAddress”并同步相关状态。
    void setRemoteAddress(const QString &ip, quint16 port);
    /// @brief 设置“RemoteAddress”并同步相关状态。
    void setRemoteAddress(const QHostAddress &addr, quint16 port);

    // 获取远程地址
    /// @brief 执行“remoteAddress”对应的业务操作。
    QHostAddress remoteAddress() const { return m_remoteAddr; }
    /// @brief 执行“remotePort”对应的业务操作。
    quint16 remotePort() const { return m_remotePort; }

    // 绑定本地端口
    /// @brief 执行“bind”对应的业务操作。
    bool bind(quint16 port);
    /// @brief 执行“bind”对应的业务操作。
    bool bind(const QHostAddress &localAddr, quint16 port);

    // 断开连接
    /// @brief 执行“disconnect”对应的业务操作。
    void disconnect();

    // 发送数据
    /// @brief 封装并发送数据。
    qint64 sendData(const QByteArray &data);
    /// @brief 封装并发送数据。
    qint64 sendData(const char *data, qint64 len);

    // 发送数据到指定地址（不改变已设置的远程地址）
    /// @brief 封装并发送数据。
    qint64 sendDataTo(const QByteArray &data, const QHostAddress &addr, quint16 port);
    /// @brief 封装并发送数据。
    qint64 sendDataTo(const char *data, qint64 len, const QHostAddress &addr, quint16 port);

    // 状态查询
    /// @brief 判断“Connected”是否处于有效状态。
    bool isConnected() const;
    /// @brief 判断“Bound”是否处于有效状态。
    bool isBound() const;
    /// @brief 执行“localPort”对应的业务操作。
    quint16 localPort() const;
    /// @brief 执行错误对应的业务操作。
    QString errorString() const;

    // 设置选项
    /// @brief 设置“BroadcastEnabled”并同步相关状态。
    void setBroadcastEnabled(bool enabled);
    /// @brief 判断“BroadcastEnabled”是否处于有效状态。
    bool isBroadcastEnabled() const;

signals:
    /// @brief 通知订阅者已收到数据。
    void dataReceived(const QByteArray &data, const QHostAddress &senderAddr, quint16 senderPort);
    /// @brief 执行错误对应的业务操作。
    void errorOccurred(const QString &error);
    /// @brief 建立“ed”所需的信号连接。
    void connected();
    /// @brief 执行“disconnected”对应的业务操作。
    void disconnected();
    /// @brief 执行“bytesWritten”对应的业务操作。
    void bytesWritten(qint64 bytes);

private slots:
    /// @brief 处理“ReadyRead”对应的事件或信号回调。
    void onReadyRead();
    /// @brief 通知订阅者状态已经变化。
    void onStateChanged(QAbstractSocket::SocketState state);
    /// @brief 通知订阅者发生“on”错误。
    void onError(QAbstractSocket::SocketError socketError);

private:
    QUdpSocket *m_udpSocket;
    QHostAddress m_remoteAddr;
    quint16 m_remotePort;
    quint16 m_localPort;
    QString m_lastError;
    bool m_broadcastEnabled;
};

#endif // UDPHANDLER_H
