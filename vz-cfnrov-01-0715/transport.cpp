// transport.cpp
#include "transport.h"
#include "udphandler.h"
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QByteArray>
#include <QQueue>
#include <QString>
#include <cstring>

// 全局 UDP 处理器
static UdpHandler* g_udpHandler = nullptr;
static QThread* g_udpThread = nullptr;
static QString g_lastError;

// 接收缓冲区
static QMutex g_recvMutex;
static QWaitCondition g_recvCondition;
static QQueue<QByteArray> g_recvQueue;

// 初始化标志
static bool g_initialized = false;

// UDP 工作线程类
// 在独立 Qt 线程内创建 UDP 套接字，并把收到的数据放入共享队列。
class UdpWorker : public QObject {
    Q_OBJECT
public:
    UdpWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void init() {
        g_udpHandler = new UdpHandler(this);
        g_udpHandler->setRemoteAddress("127.0.0.1", 14550);

        // 连接信号 - MinGW 下使用 Qt 5 的 connect 语法
        QObject::connect(g_udpHandler, &UdpHandler::dataReceived,
                        [](const QByteArray& data, const QHostAddress& /*addr*/, quint16 /*port*/) {
            QMutexLocker locker(&g_recvMutex);
            g_recvQueue.enqueue(data);
            g_recvCondition.wakeOne();
        });

        QObject::connect(g_udpHandler, &UdpHandler::errorOccurred,
                        [](const QString& error) {
            g_lastError = error;
        });

        g_udpHandler->bind(14551);
        g_initialized = true;
    }
};

// 必须包含 moc 生成的文件
#include "transport.moc"

// 全局 QCoreApplication 指针
static QCoreApplication* g_app = nullptr;
static bool g_appOwned = false;

// 确保 Qt 应用程序存在
static void ensure_qt_app(int* argc = nullptr, char** argv = nullptr) {
    if (!g_app) {
        static int s_argc = 0;
        static char s_argv0[] = "transport";
        static char* s_argv[] = { s_argv0, nullptr };

        if (argc && argv) {
            g_app = new QCoreApplication(*argc, argv);
        } else {
            g_app = new QCoreApplication(s_argc, s_argv);
        }
        g_appOwned = true;
    }
}

extern "C" {

// 初始化 C 接口的 UDP 传输层；收到的数据由 transport_receive 从队列取出。
// 启动等待最多约 500 毫秒，避免调用方无限等待工作线程。
bool transport_init(const char* remote_ip, uint16_t remote_port, uint16_t local_port) {
    ensure_qt_app();

    if (g_initialized) {
        return true;
    }

    g_udpThread = new QThread();
    UdpWorker* worker = new UdpWorker();
    worker->moveToThread(g_udpThread);

    QObject::connect(g_udpThread, &QThread::started, worker, &UdpWorker::init);
    QObject::connect(g_udpThread, &QThread::finished, worker, &QObject::deleteLater);

    g_udpThread->start();

    // 等待初始化完成
    for (int i = 0; i < 50 && !g_initialized; i++) {
        QThread::msleep(10);
        if (g_app) {
            g_app->processEvents();
        }
    }

    if (g_initialized && g_udpHandler) {
        g_udpHandler->setRemoteAddress(QString::fromUtf8(remote_ip), remote_port);
    }

    return g_initialized;
}

void transport_close(void) {
    if (g_udpHandler) {
        g_udpHandler->disconnect();
    }

    if (g_udpThread) {
        g_udpThread->quit();
        g_udpThread->wait(3000);  // 等待最多 3 秒
        delete g_udpThread;
        g_udpThread = nullptr;
    }

    g_udpHandler = nullptr;
    g_initialized = false;

    // 注意：不要删除 g_app，因为可能还有其他 Qt 对象在使用
}

int transport_send(const uint8_t* data, uint16_t len) {
    if (!g_initialized || !g_udpHandler) {
        g_lastError = "Transport not initialized";
        return -1;
    }

    if (!g_udpHandler->isConnected()) {
        g_lastError = "UDP not connected";
        return -1;
    }

    qint64 sent = g_udpHandler->sendData(reinterpret_cast<const char*>(data), len);

    // 处理 Qt 事件
    if (g_app) {
        g_app->processEvents();
    }

    return (int)sent;
}

// 读取一包已排队的 UDP 数据，复制长度受调用方缓冲区大小限制。
// 队列为空返回 0，传输层未初始化返回 -1。
int transport_receive(uint8_t* buffer, uint16_t max_len) {
    if (!g_initialized) {
        return -1;
    }

    // 处理 Qt 事件
    if (g_app) {
        g_app->processEvents();
    }

    QMutexLocker locker(&g_recvMutex);
    if (g_recvQueue.isEmpty()) {
        return 0;
    }

    QByteArray data = g_recvQueue.dequeue();
    uint16_t copy_len = (uint16_t)qMin((int)max_len, data.size());
    memcpy(buffer, data.constData(), copy_len);

    return copy_len;
}

bool transport_is_connected(void) {
    return g_initialized && g_udpHandler && g_udpHandler->isConnected();
}

const char* transport_get_last_error(void) {
    static QByteArray lastErrorBytes;
    lastErrorBytes = g_lastError.toUtf8();
    return lastErrorBytes.constData();
}

} // extern "C"
