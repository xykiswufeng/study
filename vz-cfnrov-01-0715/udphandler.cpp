#include "udphandler.h"
#include <QDebug>
#include <QNetworkInterface>

UdpHandler::UdpHandler(QObject *parent)
    : QObject(parent)
    , m_udpSocket(new QUdpSocket(this))
    , m_remoteAddr(QHostAddress::LocalHost)
    , m_remotePort(0)
    , m_localPort(0)
    , m_broadcastEnabled(false)
{
    // MinGW 下使用传统 SIGNAL/SLOT 语法确保兼容性
    connect(m_udpSocket, SIGNAL(readyRead()),
            this, SLOT(onReadyRead()));
    connect(m_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(onStateChanged(QAbstractSocket::SocketState)));
    connect(m_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));

    qDebug() << "UdpHandler created";
}

UdpHandler::~UdpHandler()
{
    disconnect();
    qDebug() << "UdpHandler destroyed";
}

// 字符串地址先由 Qt 校验，解析失败时保留原有目标并报告错误。
void UdpHandler::setRemoteAddress(const QString &ip, quint16 port)
{
    QHostAddress addr(ip);
    if (addr.isNull()) {
        m_lastError = QString("Invalid IP address: %1").arg(ip);
        emit errorOccurred(m_lastError);
        qDebug() << m_lastError;
        return;
    }
    setRemoteAddress(addr, port);
}

void UdpHandler::setRemoteAddress(const QHostAddress &addr, quint16 port)
{
    m_remoteAddr = addr;
    m_remotePort = port;
    qDebug() << "Remote address set to:" << addr.toString() << ":" << port;
}

bool UdpHandler::bind(quint16 port)
{
    return bind(QHostAddress::AnyIPv4, port);
}

// 重新绑定前关闭旧套接字；绑定结果通过连接或错误信号通知上层。
bool UdpHandler::bind(const QHostAddress &localAddr, quint16 port)
{
    if (m_udpSocket->state() == QAbstractSocket::BoundState) {
        disconnect();
    }

    // QUdpSocket::ShareAddress 允许多个程序绑定同一端口
    QAbstractSocket::BindMode bindMode = QUdpSocket::ShareAddress;

    if (m_udpSocket->bind(localAddr, port, bindMode)) {
        m_localPort = port;
        qDebug() << "UDP Socket bound to" << localAddr.toString() << ":" << port;
        emit connected();
        return true;
    } else {
        m_lastError = QString("Bind failed: %1").arg(m_udpSocket->errorString());
        qDebug() << m_lastError;
        emit errorOccurred(m_lastError);
        return false;
    }
}

void UdpHandler::disconnect()
{
    if (m_udpSocket && m_udpSocket->state() != QAbstractSocket::UnconnectedState) {
        m_udpSocket->close();
        m_localPort = 0;
        qDebug() << "UDP disconnected";
        emit disconnected();
    }
}

// 向已配置的目标发送一包数据；未绑定或未配置目标时返回 -1。
qint64 UdpHandler::sendData(const QByteArray &data)
{
    if (!isBound()) {
        m_lastError = "UDP socket not bound";
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (m_remotePort == 0 || m_remoteAddr.isNull()) {
        m_lastError = "Remote address not set";
        emit errorOccurred(m_lastError);
        return -1;
    }

    qint64 len = m_udpSocket->writeDatagram(data, m_remoteAddr, m_remotePort);
    if (len == -1) {
        m_lastError = QString("Send failed: %1").arg(m_udpSocket->errorString());
        qDebug() << m_lastError;
        emit errorOccurred(m_lastError);
    } else {
        // qDebug() << "Sent" << len << "bytes to" << m_remoteAddr.toString() << ":" << m_remotePort;
        emit bytesWritten(len);
    }
    return len;
}

qint64 UdpHandler::sendData(const char *data, qint64 len)
{
    if (!isBound()) {
        m_lastError = "UDP socket not bound";
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (m_remotePort == 0 || m_remoteAddr.isNull()) {
        m_lastError = "Remote address not set";
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (data == nullptr || len <= 0) {
        m_lastError = "Invalid data pointer or length";
        emit errorOccurred(m_lastError);
        return -1;
    }

    qint64 sentLen = m_udpSocket->writeDatagram(data, len, m_remoteAddr, m_remotePort);
    if (sentLen == -1) {
        m_lastError = QString("Send failed: %1").arg(m_udpSocket->errorString());
        qDebug() << m_lastError;
        emit errorOccurred(m_lastError);
    } else {
        // qDebug() << "Sent" << sentLen << "bytes to" << m_remoteAddr.toString() << ":" << m_remotePort;
        emit bytesWritten(sentLen);
    }
    return sentLen;
}

qint64 UdpHandler::sendDataTo(const QByteArray &data, const QHostAddress &addr, quint16 port)
{
    if (!isBound()) {
        m_lastError = "UDP socket not bound";
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (addr.isNull() || port == 0) {
        m_lastError = "Invalid destination address or port";
        emit errorOccurred(m_lastError);
        return -1;
    }

    qint64 len = m_udpSocket->writeDatagram(data, addr, port);
    if (len == -1) {
        m_lastError = QString("Send failed: %1").arg(m_udpSocket->errorString());
        qDebug() << m_lastError;
        emit errorOccurred(m_lastError);
    } else {
        // qDebug() << "Sent" << len << "bytes to" << addr.toString() << ":" << port;
        emit bytesWritten(len);
    }
    return len;
}

qint64 UdpHandler::sendDataTo(const char *data, qint64 len, const QHostAddress &addr, quint16 port)
{
    if (!isBound()) {
        m_lastError = "UDP socket not bound";
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (addr.isNull() || port == 0) {
        m_lastError = "Invalid destination address or port";
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (data == nullptr || len <= 0) {
        m_lastError = "Invalid data pointer or length";
        emit errorOccurred(m_lastError);
        return -1;
    }

    qint64 sentLen = m_udpSocket->writeDatagram(data, len, addr, port);
    if (sentLen == -1) {
        m_lastError = QString("Send failed: %1").arg(m_udpSocket->errorString());
        qDebug() << m_lastError;
        emit errorOccurred(m_lastError);
    } else {
        // qDebug() << "Sent" << sentLen << "bytes to" << addr.toString() << ":" << port;
        emit bytesWritten(sentLen);
    }
    return sentLen;
}

bool UdpHandler::isConnected() const
{
    return isBound() && m_remotePort != 0 && !m_remoteAddr.isNull();
}

bool UdpHandler::isBound() const
{
    return m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState;
}

quint16 UdpHandler::localPort() const
{
    return m_udpSocket ? m_udpSocket->localPort() : 0;
}

QString UdpHandler::errorString() const
{
    return m_lastError.isEmpty() ? m_udpSocket->errorString() : m_lastError;
}

void UdpHandler::setBroadcastEnabled(bool enabled)
{
    m_broadcastEnabled = enabled;
    if (m_udpSocket) {
        // 设置套接字选项允许广播
        int optval = enabled ? 1 : 0;
        m_udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, optval);
    }
}

bool UdpHandler::isBroadcastEnabled() const
{
    return m_broadcastEnabled;
}

void UdpHandler::onReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        if (datagram.isValid()) {
            QByteArray data = datagram.data();
            QHostAddress senderAddr = datagram.senderAddress();
            quint16 senderPort = datagram.senderPort();

            emit dataReceived(data, senderAddr, senderPort);
        } else {
            qDebug() << "Received invalid datagram";
        }
    }
}

void UdpHandler::onStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "Socket state changed to:" << state;

    switch (state) {
    case QAbstractSocket::UnconnectedState:
        qDebug() << "  -> Unconnected";
        break;
    case QAbstractSocket::HostLookupState:
        qDebug() << "  -> Host lookup";
        break;
    case QAbstractSocket::ConnectingState:
        qDebug() << "  -> Connecting";
        break;
    case QAbstractSocket::ConnectedState:
        qDebug() << "  -> Connected";
        break;
    case QAbstractSocket::BoundState:
        qDebug() << "  -> Bound";
        break;
    case QAbstractSocket::ClosingState:
        qDebug() << "  -> Closing";
        break;
    case QAbstractSocket::ListeningState:
        qDebug() << "  -> Listening";
        break;
    default:
        qDebug() << "  -> Unknown state";
        break;
    }
}

void UdpHandler::onError(QAbstractSocket::SocketError socketError)
{
    m_lastError = m_udpSocket->errorString();
    QString errorMsg = QString("Socket error %1: %2")
                      .arg(socketError)
                      .arg(m_lastError);

    qDebug() << errorMsg;
    emit errorOccurred(errorMsg);

    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() << "  -> Connection refused";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        qDebug() << "  -> Remote host closed";
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() << "  -> Host not found";
        break;
    case QAbstractSocket::SocketAccessError:
        qDebug() << "  -> Socket access error";
        break;
    case QAbstractSocket::SocketResourceError:
        qDebug() << "  -> Socket resource error";
        break;
    case QAbstractSocket::SocketTimeoutError:
        qDebug() << "  -> Socket timeout";
        break;
    case QAbstractSocket::DatagramTooLargeError:
        qDebug() << "  -> Datagram too large";
        break;
    case QAbstractSocket::NetworkError:
        qDebug() << "  -> Network error";
        break;
    case QAbstractSocket::AddressInUseError:
        qDebug() << "  -> Address in use";
        break;
    case QAbstractSocket::SocketAddressNotAvailableError:
        qDebug() << "  -> Socket address not available";
        break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        qDebug() << "  -> Unsupported socket operation";
        break;
    default:
        qDebug() << "  -> Unknown error";
        break;
    }
}

// 静态辅助函数：获取本机所有 IP 地址
QList<QHostAddress> UdpHandler_getLocalAddresses()
{
    return QNetworkInterface::allAddresses();
}

// 静态辅助函数：检查地址是否为本机地址
bool UdpHandler_isLocalAddress(const QHostAddress &addr)
{
    if (addr == QHostAddress::LocalHost || addr == QHostAddress::LocalHostIPv6) {
        return true;
    }

    QList<QHostAddress> localAddrs = QNetworkInterface::allAddresses();
    foreach (const QHostAddress &localAddr, localAddrs) {
        if (localAddr == addr) {
            return true;
        }
    }
    return false;
}
