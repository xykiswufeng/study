#ifndef FFMPEGRTSPPLAYER_H
#define FFMPEGRTSPPLAYER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <QElapsedTimer>
#include <QFuture>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/time.h>
//xyk
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}


/**
 * @brief RTSP视频播放器（纯解码器，不继承QWidget）
 *
 * 职责：
 * - 打开RTSP流
 * - 解码视频帧
 * - 通过信号发送QImage给外部显示
 */
class FFmpegRtspPlayer : public QObject
{
    Q_OBJECT

public:
    /// @brief 创建并初始化 FFmpegRtspPlayer 对象。
    explicit FFmpegRtspPlayer(QObject *parent = nullptr);
    /// @brief 停止后台任务并释放 FFmpegRtspPlayer 占用的资源。
    ~FFmpegRtspPlayer();

    // 播放控制
    /// @brief 打开 RTSP 地址并启动视频解码。
    bool startPlay(const QString &url);
    /// @brief 在后台线程中打开 RTSP 地址，避免阻塞界面。
    void startPlayAsync(const QString &url);
    /// @brief 停止播放并收尾相关状态。
    void stopPlay();
    /// @brief 发出退出请求，使网络读取和解码线程尽快停止。
    void requestShutdown();
    /// @brief 等待播放任务结束并完整释放播放器资源。
    void shutdown();
    /// @brief 判断播放器当前是否正在输出视频帧。
    bool isPlaying() const { return m_isPlaying.load(); }

    // 获取状态
    /// @brief 获取当前、帧率。
    double getCurrentFps() const { return m_currentFps; }
    /// @brief 获取当前、地址。
    QString getCurrentUrl() const { return m_currentUrl; }
    /// @brief 获取视频、尺寸。
    QSize getVideoSize() const { return m_videoSize; }

    // 获取当前帧（用于外部主动获取）
    /// @brief 获取最近一帧视频画面的线程安全副本。
    QImage getCurrentFrame() const;

    // 时间戳设置
    /// @brief 设置显示、时间戳并同步相关状态。
    void setShowTimestamp(bool show) { m_showTimestamp = show; }
    /// @brief 设置时间戳、格式并同步相关状态。
    void setTimestampFormat(const QString &format) { m_timestampFormat = format; }
    /// @brief 设置时间戳、颜色并同步相关状态。
    void setTimestampColor(const QColor &color) { m_timestampColor = color; }
    /// @brief 设置时间戳、位置并同步相关状态。
    void setTimestampPosition(Qt::Corner corner) { m_timestampCorner = corner; }
    /// @brief 设置时间戳、字体、尺寸并同步相关状态。
    void setTimestampFontSize(int size) { m_timestampFontSize = size; }
    /// @brief 设置时间戳、边距并同步相关状态。
    void setTimestampMargin(int margin) { m_timestampMargin = margin; }
    /// @brief 设置时间戳、前缀并同步相关状态。
    void setTimestampPrefix(const QString &prefix) { m_timestampPrefix = prefix; }

    // 将带时间水印的解码画面编码为本地 Motion-JPEG AVI。
    /// @brief 启动录像。
    bool startRecording(const QString &filePath);
    /// @brief 停止录像并收尾相关状态。
    void stopRecording();
    /// @brief 判断录像是否处于有效状态。
    bool isRecording() const;
    /// @brief 获取当前录像文件的保存路径。
    QString recordingFilePath() const;

signals:
    // 核心信号：发送解码后的视频帧
    /// @brief 通知订阅者已经解码出一帧新画面。
    void newFrame(const QImage &image);

    // 状态信号
    /// @brief 通知订阅者视频播放已经启动。
    void started();
    /// @brief 通知订阅者视频播放已经停止。
    void stopped();
    /// @brief 通知订阅者播放器发生错误，并携带错误说明。
    void error(const QString &msg);
    /// @brief 通知订阅者帧率已经更新。
    void fpsUpdated(double fps);
    /// @brief 通知订阅者视频、尺寸已经变化。
    void videoSizeChanged(const QSize &size);
    /// @brief 通知订阅者录像已经开始，并提供保存路径。
    void recordingStarted(const QString &filePath);
    /// @brief 通知订阅者录像已经停止，并提供保存路径。
    void recordingStopped(const QString &filePath);
    /// @brief 通知订阅者发生录像错误。
    void recordingError(const QString &message);

private slots:
    /// @brief 处理画面帧对应的事件或信号回调。
    void onNewFrame(const QImage &image);
//xyk
private:

    AVFilterGraph* m_filterGraph = nullptr;
    AVFilterContext* m_filterSrcCtx = nullptr;
    AVFilterContext* m_filterSinkCtx = nullptr;
    /// @brief 初始化滤镜所需的对象和连接。
    bool initFilterGraph(AVFrame* srcFrame);
    /// @brief 判断输入视频帧的格式或尺寸是否发生变化。
    bool filterInputChanged(const AVFrame* srcFrame) const;
    /// @brief 释放滤镜资源并清空对应指针。
    void freeFilterGraph();
    int m_filterInputWidth = 0;
    int m_filterInputHeight = 0;
    AVPixelFormat m_filterInputFormat = AV_PIX_FMT_NONE;
    AVRational m_filterInputTimeBase{0, 1};
    AVRational m_filterInputSar{0, 1};
    AVColorSpace m_filterInputColorSpace = AVCOL_SPC_UNSPECIFIED;
    AVColorRange m_filterInputColorRange = AVCOL_RANGE_UNSPECIFIED;

private:
    class DecodeThread : public QThread
    {
    public:
        /// @brief 创建并初始化 DecodeThread 对象。
        DecodeThread(FFmpegRtspPlayer *player);
        /// @brief 执行工作线程的主循环。
        void run() override;
        /// @brief 请求工作线程停止解码循环。
        void requestStop() { m_stopRequested.store(true); }
        /// @brief 判断工作线程是否已经收到停止请求。
        bool isStopRequested() const { return m_stopRequested.load(); }

    private:
        FFmpegRtspPlayer *m_player;
        std::atomic_bool m_stopRequested;
    };

    // 解码循环（在线程中运行）
    /// @brief 持续读取并解码视频包，直到收到停止请求。
    void decodeLoop();

    // 计算帧率
    /// @brief 计算帧率。
    void calculateFps();

    // 在图像上叠加时间戳
    /// @brief 在视频帧上叠加摄像头名称和时间水印。
    QImage overlayTimestamp(const QImage &src);

    /// @brief 创建录像编码器、输出流和像素转换资源。
    bool initializeRecorder(const QString &filePath, int sourceWidth, int sourceHeight);
    /// @brief 把一帧画面编码并写入当前录像文件。
    void writeRecordedFrame(const QImage &image);
    /// @brief 写完文件尾并释放录像编码资源。
    void closeRecorder();

    // 清理资源
    /// @brief 清理“cleanup”持有的运行资源。
    void cleanup();
    /// @brief 判断当前播放或建连任务是否需要立即中止。
    bool shouldAbort() const;
    /// @brief 在限定时间内等待异步建连任务结束。
    bool waitForStartTask(int timeoutMs);
    /// @brief 在退出或切流时中断 FFmpeg 的阻塞网络操作。
    static int interruptCallback(void *opaque);

private:
    // 线程
    DecodeThread *m_decodeThread;
    QFuture<void> m_startFuture;
    mutable QMutex m_lifecycleMutex;
    std::atomic_bool m_abortRequested{false};//要求中断当前网络连接或播放
    std::atomic_bool m_shuttingDown{false};//播放器是否正在彻底关闭

    // 当前帧缓存
    QImage m_currentImage;
    mutable QMutex m_imageMutex;

    // 视频信息
    QString m_currentUrl;
    QSize m_videoSize;

    // FFmpeg 相关
    AVFormatContext *m_formatCtx;
    AVCodecContext *m_codecCtx;
    int m_videoStreamIndex;

    // 播放状态
    std::atomic_bool m_isPlaying;

    // 帧率计算
    int m_frameCount;
    qint64 m_lastFpsTime;
    double m_currentFps;

    // 时间戳设置
    bool m_showTimestamp;
    QString m_timestampFormat;
    QColor m_timestampColor;
    Qt::Corner m_timestampCorner;
    int m_timestampFontSize;
    int m_timestampMargin;
    QString m_timestampPrefix;

    // 录像器（由解码线程写帧，UI线程可安全启停）
    mutable QMutex m_recordMutex;
    bool m_recording;
    QString m_recordFilePath;
    AVFormatContext *m_recordFormatCtx;
    AVCodecContext *m_recordCodecCtx;
    AVStream *m_recordStream;
    SwsContext *m_recordSwsCtx;
    AVFrame *m_recordFrame;
    qint64 m_recordPts;
    int m_recordFps;
    bool m_recordHeaderWritten;
};

#endif // FFMPEGRTSPPLAYER_H
