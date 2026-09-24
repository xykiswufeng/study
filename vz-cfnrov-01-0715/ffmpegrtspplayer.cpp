#include "ffmpegrtspplayer.h"
#include <QPainter>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <cstdio>
#define DEBUG qDebug() << __FILE__ << __LINE__
FFmpegRtspPlayer::FFmpegRtspPlayer(QObject *parent)
    : QObject(parent)
    , m_decodeThread(nullptr)
    , m_formatCtx(nullptr)//管理RTSP输入，网络读取和流信息
    , m_codecCtx(nullptr)//管理视频解码器
    , m_videoStreamIndex(-1)//记录视频流在rtsp中的编号
    , m_isPlaying(false)//当前是否在播放
    , m_frameCount(0)
    , m_lastFpsTime(0)
    , m_currentFps(0.0)
    , m_showTimestamp(true)
    , m_timestampFormat("yyyy-MM-dd hh:mm:ss")
    , m_timestampColor(Qt::yellow)
    , m_timestampCorner(Qt::BottomRightCorner)
    , m_timestampFontSize(16)
    , m_timestampMargin(10)
    , m_recording(false)
    , m_recordFormatCtx(nullptr)
    , m_recordCodecCtx(nullptr)
    , m_recordStream(nullptr)
    , m_recordSwsCtx(nullptr)
    , m_recordFrame(nullptr)
    , m_recordPts(0)
    , m_recordFps(25)
    , m_recordHeaderWritten(false)
{
    // 连接信号（用于缓存当前帧）
    connect(this, &FFmpegRtspPlayer::newFrame,
            this, &FFmpegRtspPlayer::onNewFrame,
            Qt::DirectConnection);  // 直接连接，减少延迟
}

FFmpegRtspPlayer::~FFmpegRtspPlayer()
{
    shutdown();
}

// FFmpeg 在阻塞网络读取期间调用此回调；关闭或切流时返回 1，
// 使读取尽快退出，避免界面等待网络超时。
int FFmpegRtspPlayer::interruptCallback(void *opaque)
{
    auto *player = static_cast<FFmpegRtspPlayer *>(opaque);
    return player && player->shouldAbort() ? 1 : 0;
}

bool FFmpegRtspPlayer::shouldAbort() const
{
    return m_abortRequested.load() || m_shuttingDown.load();
}

// 等待异步启动任务结束，但最多等待指定毫秒数；
// 调用方据此决定是否继续清理播放器资源。
bool FFmpegRtspPlayer::waitForStartTask(int timeoutMs)
{
    QFuture<void> future;
    {
        QMutexLocker locker(&m_lifecycleMutex);
        future = m_startFuture;
    }
    if (!future.isRunning())
        return true;

    QElapsedTimer elapsed;
    elapsed.start();
    while (future.isRunning() && elapsed.elapsed() < timeoutMs)
        QThread::msleep(10);
    return !future.isRunning();
}

// 缓存解码后的最近一帧，读取方通过同一把互斥锁获取画面。
void FFmpegRtspPlayer::onNewFrame(const QImage &image)
{
    if (!image.isNull()) {
        QMutexLocker locker(&m_imageMutex);
        m_currentImage = image;
    }
}

// 按当前视频帧的格式建立 FFmpeg 滤镜链。
// 修改亮度、对比度或镜头畸变参数时，应同时检查输出像素格式。
bool FFmpegRtspPlayer::initFilterGraph(AVFrame *srcFrame)
{
    freeFilterGraph();

    m_filterGraph = avfilter_graph_alloc();
    if (!m_filterGraph)
    {
        qWarning() << "avfilter_graph_alloc failed";
        return false;
    }
    // ==========【这里改你的滤镜参数！】==========

    // eq=brightness亮度(-2~2),contrast对比度(0~10)

    const char* filterDesc =
        "format=pix_fmts=yuv420p,"
        "lenscorrection=k1=-0.25:k2=0.05,"
        "eq=brightness=0.12:contrast=1.15"
       ;

    // ============================================
    int ret = 0;
    const AVFilter* srcFilter = avfilter_get_by_name("buffer");
    const AVFilter* sinkFilter = avfilter_get_by_name("buffersink");

    if(!srcFilter || !sinkFilter)
    {
        qWarning() << "filter buffer/buffersink not found";
        freeFilterGraph();
        return false;
    }

    const AVPixelFormat sourceFormat = static_cast<AVPixelFormat>(srcFrame->format);
    const char *sourceFormatName = av_get_pix_fmt_name(sourceFormat);
    if (!sourceFormatName || !m_formatCtx || m_videoStreamIndex < 0) {
        qWarning() << "Invalid filter input frame or stream";
        freeFilterGraph();
        return false;
    }
    AVRational timeBase = m_formatCtx->streams[m_videoStreamIndex]->time_base;
    if (timeBase.num <= 0 || timeBase.den <= 0)
        timeBase = AVRational{1, 25};
    const AVRational sar = srcFrame->sample_aspect_ratio.num > 0 && srcFrame->sample_aspect_ratio.den > 0
        ? srcFrame->sample_aspect_ratio : AVRational{1, 1};

    char args[512];
    snprintf(args,sizeof(args),
    "video_size=%dx%d:"
    "pix_fmt=%s:"
    "time_base=%d/%d:"
    "pixel_aspect=%d/%d",
    srcFrame->width,
    srcFrame->height,
    sourceFormatName,
    timeBase.num,
    timeBase.den,
    sar.num,
    sar.den);

    DEBUG
        << "width:" << srcFrame->width
        << "height:" << srcFrame->height
        << "format:" << srcFrame->format
        << "pixfmt:"
        << av_get_pix_fmt_name((AVPixelFormat)srcFrame->format)
        << "timebase:"
        << m_codecCtx->time_base.num
        << "/"
        << m_codecCtx->time_base.den
        << "args:"
        << args;
    ret = avfilter_graph_create_filter(&m_filterSrcCtx, srcFilter, "in", args, nullptr, m_filterGraph);
    if(ret <0)
    {
        qWarning() << "create buffer src filter fail";

            char errbuf[256];
            av_strerror(ret, errbuf, sizeof(errbuf));

            qWarning()
                << "parse filter graph failed:"
                << errbuf;


        freeFilterGraph();
        return false;
    }

    // buffer 参数字符串不能可靠地覆盖所有颜色元数据；显式同步当前解码帧，
    // 否则全范围(yuvj420p/pc)帧会被当作 unknown range，触发属性变化错误。
    AVBufferSrcParameters *sourceParams = av_buffersrc_parameters_alloc();
    if (!sourceParams) {
        qWarning() << "Failed to allocate buffer source parameters";
        freeFilterGraph();
        return false;
    }
    sourceParams->format = sourceFormat;
    sourceParams->time_base = timeBase;
    sourceParams->width = srcFrame->width;
    sourceParams->height = srcFrame->height;
    sourceParams->sample_aspect_ratio = sar;
    sourceParams->color_space = srcFrame->colorspace;
    sourceParams->color_range = srcFrame->color_range;
    ret = av_buffersrc_parameters_set(m_filterSrcCtx, sourceParams);
    av_free(sourceParams);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qWarning() << "Failed to configure buffer source parameters:" << errbuf;
        freeFilterGraph();
        return false;
    }

    ret = avfilter_graph_create_filter(&m_filterSinkCtx, sinkFilter, "out", nullptr, nullptr, m_filterGraph);
    if(ret <0)
    {
        qWarning() << "create buffer sink filter fail";
        freeFilterGraph();
        return false;
    }

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    if (!outputs || !inputs) {
        qWarning() << "Failed to allocate filter graph endpoints";
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        freeFilterGraph();
        return false;
    }
    outputs->name = av_strdup("in");
    outputs->filter_ctx = m_filterSrcCtx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = m_filterSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;
    if (!outputs->name || !inputs->name) {
        qWarning() << "Failed to allocate filter graph endpoint names";
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        freeFilterGraph();
        return false;
    }

    ret = avfilter_graph_parse_ptr(m_filterGraph, filterDesc, &inputs, &outputs, nullptr);
    if(ret <0)
    {
        qWarning() << "parse filter graph fail";
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        freeFilterGraph();
        return false;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    ret = avfilter_graph_config(m_filterGraph, nullptr);
    if(ret <0)
    {
        qWarning() << "config filter graph fail";
        freeFilterGraph();
        return false;
    }
    m_filterInputWidth = srcFrame->width;
    m_filterInputHeight = srcFrame->height;
    m_filterInputFormat = sourceFormat;
    m_filterInputTimeBase = timeBase;
    m_filterInputSar = sar;
    m_filterInputColorSpace = srcFrame->colorspace;
    m_filterInputColorRange = srcFrame->color_range;
    qDebug() << "Filter graph initialized OK";
    return true;
}

bool FFmpegRtspPlayer::filterInputChanged(const AVFrame *srcFrame) const
{
    if (!srcFrame || !m_formatCtx || m_videoStreamIndex < 0)
        return true;

    AVRational timeBase = m_formatCtx->streams[m_videoStreamIndex]->time_base;
    if (timeBase.num <= 0 || timeBase.den <= 0)
        timeBase = AVRational{1, 25};
    const AVRational sar = srcFrame->sample_aspect_ratio.num > 0 && srcFrame->sample_aspect_ratio.den > 0
        ? srcFrame->sample_aspect_ratio : AVRational{1, 1};
    return m_filterInputWidth != srcFrame->width ||
           m_filterInputHeight != srcFrame->height ||
           m_filterInputFormat != static_cast<AVPixelFormat>(srcFrame->format) ||
           av_cmp_q(m_filterInputTimeBase, timeBase) != 0 ||
           av_cmp_q(m_filterInputSar, sar) != 0 ||
           m_filterInputColorSpace != srcFrame->colorspace ||
           m_filterInputColorRange != srcFrame->color_range;
}

void FFmpegRtspPlayer::freeFilterGraph()
{
    if(m_filterGraph)
    {
        avfilter_graph_free(&m_filterGraph);
        m_filterSrcCtx = nullptr;
        m_filterSinkCtx = nullptr;
        m_filterGraph = nullptr;
    }
    m_filterInputWidth = 0;
    m_filterInputHeight = 0;
    m_filterInputFormat = AV_PIX_FMT_NONE;
    m_filterInputTimeBase = AVRational{0, 1};
    m_filterInputSar = AVRational{0, 1};
    m_filterInputColorSpace = AVCOL_SPC_UNSPECIFIED;
    m_filterInputColorRange = AVCOL_RANGE_UNSPECIFIED;
}

// 返回最近一帧的独立副本，避免调用方与解码线程共享可变图像。
QImage FFmpegRtspPlayer::getCurrentFrame() const
{
    QMutexLocker locker(&m_imageMutex);
    return m_currentImage.copy();//为了调用者得到独立图像，避免解码线程更新画面产生并发问题

}

// 建立 RTSP 输入、查找视频流并启动专用解码线程。
// 任一步失败都会清理已创建的 FFmpeg 对象，并通过错误信号报告原因。
bool FFmpegRtspPlayer::startPlay(const QString &url)
{
    if (shouldAbort())
        return false;

    m_currentUrl = url;
    // qDebug() << "FFmpegPlayer: Starting RTSP stream:" << url;

    // 1. 初始化FFmpeg网络（初始化RTSP,UDP,TCP等网络协议）
    avformat_network_init();

    // 2. 分配上下文（为整个RTSP输入连接的总管理器）
    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx)
        return false;
    //安装中断回调，
    m_formatCtx->interrupt_callback.callback = &FFmpegRtspPlayer::interruptCallback;
    m_formatCtx->interrupt_callback.opaque = this;

    // 3. 设置选项
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "udp", 0);      // udp传输
    av_dict_set(&opts, "fflags", "nobuffer", 0);         // 无缓冲
    av_dict_set(&opts, "flags", "low_delay", 0);         // 低延迟
    av_dict_set(&opts, "probesize", "10240", 0);
    av_dict_set(&opts, "analyzeduration", "1000000", 0);
    av_dict_set(&opts, "stimeout", "5000000", 0);        // 5秒超时
    av_dict_set(&opts, "max_delay", "500000", 0);        // 最大延迟0.5秒

    // 4. 打开输入
    DEBUG<<"00000000000000000000";
    //打开RTSP
    int ret = avformat_open_input(&m_formatCtx, url.toUtf8().constData(), nullptr, &opts);
    DEBUG<<"1111111111111111111111111111111111"<<ret;
    av_dict_free(&opts);

    if (ret != 0)
    {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        if (!shouldAbort())
        {
             emit error(QString("无法打开 RTSP 流: %1").arg(errbuf));
        }
        cleanup();
        return false;
    }

    if (shouldAbort()) {
        cleanup();
        return false;
    }

    // 5. 查找流信息 (流，编码格式，分辨率等信息)
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        if (!shouldAbort())
            emit error("无法找到流信息");
        cleanup();
        return false;
    }

    if (shouldAbort()) {
        cleanup();
        return false;
    }

    // 6. 查找视频流
    m_videoStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        emit error("未找到视频流");
        cleanup();
        return false;
    }

    // 7. 获取视频参数
    AVCodecParameters *codecPar = m_formatCtx->streams[m_videoStreamIndex]->codecpar;
    m_videoSize = QSize(codecPar->width, codecPar->height);
    emit videoSizeChanged(m_videoSize);

    // 8. 查找解码器
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit error("不支持的编码格式");
        cleanup();
        return false;
    }

    // 9. 创建解码器上下文
    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, codecPar);

    // 设置解码选项
    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->thread_count = 2;  // 使用2个线程解码

    // 10. 打开解码器  完成了连接和解码器初始化
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        emit error("无法打开解码器");
        cleanup();
        return false;
    }

    // 11. 启动解码线程
    if (shouldAbort()) {
        cleanup();
        return false;
    }
    m_isPlaying.store(true);
    m_frameCount = 0;
    m_lastFpsTime = av_gettime();

    //启用专用解码器线程
    m_decodeThread = new DecodeThread(this);
    m_decodeThread->start();

    emit started();
    return true;
}

// 中断连接任务并等待解码线程退出；确认线程不再访问 FFmpeg 对象后清理。
void FFmpegRtspPlayer::stopPlay()
{
    m_abortRequested.store(true);
    m_isPlaying.store(false);

    // 正在异步连接 RTSP 时，FFmpeg interrupt_callback 会立即打断连接。
    const bool startTaskStopped = waitForStartTask(1800);
    if (!startTaskStopped)
        qWarning() << "RTSP start task did not stop within timeout";

    // 停止解码线程
    if (m_decodeThread) {
        m_decodeThread->requestStop();
        m_decodeThread->wait(1200);

        if (m_decodeThread->isRunning()) {
            qWarning() << "Decode thread timeout, terminating...";
            m_decodeThread->terminate();
            m_decodeThread->wait(500);
        }

        if (!m_decodeThread->isRunning()) {
            delete m_decodeThread;
            m_decodeThread = nullptr;
        } else {
            qCritical() << "Decode thread could not be stopped safely";
        }
    }

    stopRecording();

    // 只有工作线程全部停止后才释放 FFmpeg 指针，避免并发访问。
    if (startTaskStopped && (!m_decodeThread || !m_decodeThread->isRunning()))
        cleanup();

    avformat_network_deinit();
    emit stopped();
}

// 把可能阻塞的 RTSP 建连放入后台任务；切流前先终止上一条连接。
void FFmpegRtspPlayer::startPlayAsync(const QString &url)
{
    if (m_shuttingDown.load())
        return;

    // 若上一次连接或播放尚未结束，先通过中断回调快速结束。
    if (m_isPlaying.load() || m_startFuture.isRunning() || m_decodeThread)
        stopPlay();

    QMutexLocker locker(&m_lifecycleMutex);
    if (m_shuttingDown.load())
        return;
    m_abortRequested.store(false);
    m_startFuture = QtConcurrent::run([this, url]() { startPlay(url); });
}

// 析构路径：通知全部工作任务退出，并等待异步建连停止后释放资源。
void FFmpegRtspPlayer::shutdown()
{
    if (m_shuttingDown.exchange(true) && !m_startFuture.isRunning() &&
        (!m_decodeThread || !m_decodeThread->isRunning()))
        return;
    m_abortRequested.store(true);
    stopPlay();

    // 析构前必须保证异步连接任务不再使用 this。interrupt_callback 通常会
    // 立即结束；这里的上限覆盖 FFmpeg 的 5 秒网络超时，杜绝无限等待。
    if (!waitForStartTask(5500))
        qCritical() << "RTSP start task exceeded shutdown deadline";
    if (!m_decodeThread || !m_decodeThread->isRunning())
        cleanup();
}

void FFmpegRtspPlayer::requestShutdown()
{
    m_shuttingDown.store(true);
    m_abortRequested.store(true);
    m_isPlaying.store(false);
    if (m_decodeThread)
        m_decodeThread->requestStop();
}

void FFmpegRtspPlayer::cleanup()
{
    //XYK
    freeFilterGraph(); // 新增释放滤镜图
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }

    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }

    m_videoSize = QSize();
    m_videoStreamIndex = -1;
}

void FFmpegRtspPlayer::decodeLoop()
{
    AVFrame *filterFrame = av_frame_alloc();
    //保存的是压缩数据，例如一段H.264/H.265数据，传来的压缩视频包
    AVPacket *packet = av_packet_alloc();
    //保存解码后的原始像素数据
    AVFrame *frame = av_frame_alloc();
    SwsContext *swsCtx = nullptr;

    int lastWidth = 0, lastHeight = 0;
    AVPixelFormat lastFormat = AV_PIX_FMT_NONE;

   // qDebug() << "FFmpegPlayer: Decode thread started, thread ID:" << QThread::currentThreadId();

    while (m_isPlaying.load() && m_decodeThread && !m_decodeThread->isStopRequested()) {
        // 读取网络数据包
        int ret = av_read_frame(m_formatCtx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                qDebug() << "FFmpegPlayer: End of stream";
            } else if (m_isPlaying.load() && !shouldAbort()) {
                char errbuf[256];
                av_strerror(ret, errbuf, sizeof(errbuf));
                emit error(QString("读取帧错误: %1").arg(errbuf));
            }
            break;
        }

        // 只处理视频流
        if (packet->stream_index == m_videoStreamIndex) {
            // 发送压缩数据包到解码器
            ret = avcodec_send_packet(m_codecCtx, packet);
            if (ret == 0) {
                // 接收解码后的帧
                while (avcodec_receive_frame(m_codecCtx, frame) == 0)
                {
                    av_frame_unref(filterFrame);
//                    DEBUG<<frame->width;
//                    DEBUG<<frame->height;
//                    DEBUG<<frame->format;
                    //xyk增加滤镜
                    if (!m_filterGraph || filterInputChanged(frame))
                       {
                           if(!initFilterGraph(frame))
                           {
                               qWarning()<<"滤镜初始化失败";
                               continue;
                           }
                       }
                        //将画面送入滤镜
                       int ret = av_buffersrc_add_frame_flags(
                                   m_filterSrcCtx,
                                   frame,
                                   AV_BUFFERSRC_FLAG_KEEP_REF);
                        if (ret < 0) {
                            char errbuf[AV_ERROR_MAX_STRING_SIZE];
                            av_strerror(ret, errbuf, sizeof(errbuf));
                            qWarning() << "Failed to feed filter graph:" << errbuf;
                            continue;
                        }
                        //  从滤镜出口取出处理后的画面
                        ret = av_buffersink_get_frame(m_filterSinkCtx, filterFrame);
                        if (ret < 0) {
                            // 当前链路一进一出；没有输出时不能再使用已送入滤镜的原始帧。
                            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                                av_strerror(ret, errbuf, sizeof(errbuf));
                                qWarning() << "Failed to read filtered frame:" << errbuf;
                            }
                            continue;
                        }
                        AVFrame *processFrame = filterFrame;

                    //结束滤镜




                    // 检查是否需要重新创建SWS上下文
                    bool sizeChanged = (lastWidth != processFrame->width) ||
                                     (lastHeight != processFrame->height) ||
                                     (lastFormat != (AVPixelFormat)processFrame->format);

                    if (sizeChanged || !swsCtx) {
                        if (swsCtx) {
                            sws_freeContext(swsCtx);
                        }

                        swsCtx = sws_getContext(
                            processFrame->width, processFrame->height, (AVPixelFormat)processFrame->format,
                            processFrame->width, processFrame->height, AV_PIX_FMT_BGRA,
                            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
                        );

                        if (!swsCtx) {
                            qWarning() << "Failed to create SwsContext";
                            continue;
                        }

                        lastWidth = processFrame->width;
                        lastHeight = processFrame->height;
                        lastFormat = (AVPixelFormat)processFrame->format;

                      //  qDebug() << "FFmpegPlayer: Video size changed to"
//                                 << lastWidth << "x" << lastHeight;
                    }

                    // 转换为QImage
                    QImage image(processFrame->width, processFrame->height, QImage::Format_ARGB32);

                    uint8_t *dstData[1] = { image.bits() };
                    int dstLinesize[1] = { static_cast<int>(image.bytesPerLine()) };

                    sws_scale(swsCtx,
                             processFrame->data, processFrame->linesize, 0, processFrame->height,
                             dstData, dstLinesize);

                    // 显示用帧保持原始方向；MainWindow 会在旋转视频后绘制界面时间。
                    // 录像仍保留日期时间水印，避免改变既有录像内容。
                    QImage recordingImage = image;
                    if (m_showTimestamp) {
                        recordingImage = overlayTimestamp(recordingImage);
                    }
                    writeRecordedFrame(recordingImage);

                    // 发送图像（没有时间水印的原始QImage）
                    emit newFrame(image);

                    // 更新帧率
                    m_frameCount++;
                    calculateFps();
                }
            } else if (ret != AVERROR(EAGAIN)) {
                qWarning() << "Error sending packet to decoder:" << ret;
            }
        }

        av_packet_unref(packet);
    }

    // 清理
    if (swsCtx) {
        sws_freeContext(swsCtx);
    }
    av_frame_free(&filterFrame);
    av_frame_free(&frame);
    av_packet_free(&packet);

    // 流异常结束时同步播放状态，否则上层重连定时器会因 isPlaying() 仍为
    // true 而永远不再发起连接。主动 stop/shutdown 已经会自行发送 stopped。
    const bool endedUnexpectedly = m_isPlaying.exchange(false) && !shouldAbort();
    qDebug() << "FFmpegPlayer: Decode thread finished";
    if (endedUnexpectedly)
        emit stopped();
}

void FFmpegRtspPlayer::calculateFps()
{
    qint64 now = av_gettime();
    qint64 elapsed = now - m_lastFpsTime;

    if (elapsed >= 1000000) {  // 每秒更新一次
        m_currentFps = (double)m_frameCount * 1000000.0 / elapsed;
        emit fpsUpdated(m_currentFps);

        m_frameCount = 0;
        m_lastFpsTime = now;
    }
}

QImage FFmpegRtspPlayer::overlayTimestamp(const QImage &src)
{
    if (!m_showTimestamp || src.isNull()) {
        return src;
    }

    QImage image = src.copy();
    QPainter painter(&image);

    // 设置字体
    QFont font = painter.font();
#ifdef Q_OS_WIN
    font.setFamily("Consolas");
#elif defined(Q_OS_MAC)
    font.setFamily("Monaco");
#else
    font.setFamily("Monospace");
#endif
    font.setPointSize(m_timestampFontSize);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(m_timestampColor);

    QString timestamp = QDateTime::currentDateTime().toString(m_timestampFormat);
    if (!m_timestampPrefix.isEmpty()) {
        timestamp = m_timestampPrefix + QStringLiteral("  ") + timestamp;
    }

    // 计算文本大小
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(timestamp);
    int textHeight = fm.height();

    // 背景框大小
    int bgWidth = textWidth + 20;
    int bgHeight = textHeight + 8;

    // 计算位置
    int x, y;
    int margin = m_timestampMargin;

    switch (m_timestampCorner) {
    case Qt::TopLeftCorner:
        x = margin;
        y = margin + textHeight;
        break;
    case Qt::TopRightCorner:
        x = image.width() - bgWidth - margin;
        y = margin + textHeight;
        break;
    case Qt::BottomLeftCorner:
        x = margin;
        y = image.height() - margin;
        break;
    case Qt::BottomRightCorner:
    default:
        x = image.width() - bgWidth - margin;
        y = image.height() - margin;
        break;
    }

    // 绘制背景
    QRect bgRect(x - 10, y - textHeight - 4, bgWidth, bgHeight);
    painter.fillRect(bgRect, QColor(0, 0, 0, 160));

    // 绘制文字
    painter.drawText(bgRect, Qt::AlignCenter, timestamp);

    return image;
}

bool FFmpegRtspPlayer::startRecording(const QString &filePath)
{
    QMutexLocker locker(&m_recordMutex);
    if (m_recording) {
        return true;
    }
    if (!m_isPlaying.load() || m_videoSize.isEmpty()) {
        emit recordingError(QString::fromUtf8("视频流尚未就绪，无法开始录像"));
        return false;
    }
    if (!initializeRecorder(filePath, m_videoSize.width(), m_videoSize.height())) {
        return false;
    }

    m_recording = true;
    m_recordFilePath = filePath;
    emit recordingStarted(filePath);
    return true;
}

void FFmpegRtspPlayer::stopRecording()
{
    QString completedPath;
    {
        QMutexLocker locker(&m_recordMutex);
        if (!m_recording && !m_recordFormatCtx) {
            return;
        }
        completedPath = m_recordFilePath;
        m_recording = false;
        closeRecorder();
        m_recordFilePath.clear();
    }
    if (!completedPath.isEmpty()) {
        emit recordingStopped(completedPath);
    }
}

bool FFmpegRtspPlayer::isRecording() const
{
    QMutexLocker locker(&m_recordMutex);
    return m_recording;
}

QString FFmpegRtspPlayer::recordingFilePath() const
{
    QMutexLocker locker(&m_recordMutex);
    return m_recordFilePath;
}

//录像器初始化
bool FFmpegRtspPlayer::initializeRecorder(const QString &filePath,
                                          int sourceWidth, int sourceHeight)
{
    closeRecorder();

    const QByteArray encodedPath = QFile::encodeName(filePath);
    //创建AVI文件容器
    int result = avformat_alloc_output_context2(&m_recordFormatCtx, nullptr, "avi",
                                                 encodedPath.constData());
    if (result < 0 || !m_recordFormatCtx) {
        emit recordingError(QString::fromUtf8("无法创建录像文件"));
        closeRecorder();
        return false;
    }

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!encoder) {
        emit recordingError(QString::fromUtf8("当前 FFmpeg 不支持 MJPEG 编码"));
        closeRecorder();
        return false;
    }

    m_recordStream = avformat_new_stream(m_recordFormatCtx, nullptr);
    m_recordCodecCtx = avcodec_alloc_context3(encoder);
    if (!m_recordStream || !m_recordCodecCtx) {
        emit recordingError(QString::fromUtf8("无法创建录像编码器"));
        closeRecorder();
        return false;
    }

    // YUV420 要求宽高为偶数；通常 RTSP 本身已经满足该条件。
    const int outputWidth = qMax(2, sourceWidth & ~1);
    const int outputHeight = qMax(2, sourceHeight & ~1);
    m_recordFps = 25;
    if (m_formatCtx && m_videoStreamIndex >= 0) {
        const AVRational sourceRate = m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate;
        const double sourceFps = av_q2d(sourceRate);
        if (sourceFps >= 5.0 && sourceFps <= 120.0) {
            m_recordFps = qBound(5, qRound(sourceFps), 120);
        }
    }

    m_recordCodecCtx->codec_id = AV_CODEC_ID_MJPEG;
    m_recordCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    m_recordCodecCtx->width = outputWidth;
    m_recordCodecCtx->height = outputHeight;
    m_recordCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    m_recordCodecCtx->color_range = AVCOL_RANGE_JPEG;
    m_recordCodecCtx->time_base = AVRational{1, m_recordFps};
    m_recordCodecCtx->framerate = AVRational{m_recordFps, 1};
    m_recordCodecCtx->bit_rate = 12000000;
    m_recordCodecCtx->gop_size = 1;

    if (m_recordFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        m_recordCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    result = avcodec_open2(m_recordCodecCtx, encoder, nullptr);
    if (result < 0) {
        emit recordingError(QString::fromUtf8("无法打开录像编码器"));
        closeRecorder();
        return false;
    }

    result = avcodec_parameters_from_context(m_recordStream->codecpar, m_recordCodecCtx);
    if (result < 0) {
        emit recordingError(QString::fromUtf8("无法写入录像编码参数"));
        closeRecorder();
        return false;
    }
    m_recordStream->time_base = m_recordCodecCtx->time_base;

    if (!(m_recordFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        result = avio_open(&m_recordFormatCtx->pb, encodedPath.constData(), AVIO_FLAG_WRITE);
        if (result < 0) {
            emit recordingError(QString::fromUtf8("无法写入所选录像路径"));
            closeRecorder();
            return false;
        }
    }

    result = avformat_write_header(m_recordFormatCtx, nullptr);
    if (result < 0) {
        emit recordingError(QString::fromUtf8("无法写入录像文件头"));
        closeRecorder();
        return false;
    }
    m_recordHeaderWritten = true;

    m_recordFrame = av_frame_alloc();
    if (!m_recordFrame) {
        emit recordingError(QString::fromUtf8("无法分配录像帧"));
        closeRecorder();
        return false;
    }
    m_recordFrame->format = m_recordCodecCtx->pix_fmt;
    m_recordFrame->width = outputWidth;
    m_recordFrame->height = outputHeight;
    result = av_frame_get_buffer(m_recordFrame, 32);
    if (result < 0) {
        emit recordingError(QString::fromUtf8("无法分配录像图像缓冲区"));
        closeRecorder();
        return false;
    }

    m_recordSwsCtx = sws_getContext(
        sourceWidth, sourceHeight, AV_PIX_FMT_BGRA,
        outputWidth, outputHeight, m_recordCodecCtx->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_recordSwsCtx) {
        emit recordingError(QString::fromUtf8("无法创建录像颜色转换器"));
        closeRecorder();
        return false;
    }

    m_recordPts = 0;
    return true;
}

void FFmpegRtspPlayer::writeRecordedFrame(const QImage &sourceImage)
{
    QMutexLocker locker(&m_recordMutex);
    if (!m_recording || !m_recordCodecCtx || !m_recordFrame ||
        !m_recordSwsCtx || sourceImage.isNull()) {
        return;
    }

    QImage image = sourceImage;
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    if (av_frame_make_writable(m_recordFrame) < 0) {
        emit recordingError(QString::fromUtf8("录像缓冲区不可写，录像已停止"));
        m_recording = false;
        closeRecorder();
        return;
    }

    const uint8_t *sourceData[1] = { image.constBits() };
    const int sourceLineSize[1] = { static_cast<int>(image.bytesPerLine()) };
    sws_scale(m_recordSwsCtx, sourceData, sourceLineSize, 0, image.height(),
              m_recordFrame->data, m_recordFrame->linesize);
    m_recordFrame->pts = m_recordPts++;

    int result = avcodec_send_frame(m_recordCodecCtx, m_recordFrame);
    if (result < 0) {
        emit recordingError(QString::fromUtf8("视频帧编码失败，录像已停止"));
        m_recording = false;
        closeRecorder();
        return;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return;
    }
    while (avcodec_receive_packet(m_recordCodecCtx, packet) == 0) {
        av_packet_rescale_ts(packet, m_recordCodecCtx->time_base,
                             m_recordStream->time_base);
        packet->stream_index = m_recordStream->index;
        av_interleaved_write_frame(m_recordFormatCtx, packet);
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
}

void FFmpegRtspPlayer::closeRecorder()
{
    if (m_recordHeaderWritten && m_recordCodecCtx &&
        m_recordFormatCtx && m_recordStream) {
        avcodec_send_frame(m_recordCodecCtx, nullptr);
        AVPacket *packet = av_packet_alloc();
        if (packet) {
            while (avcodec_receive_packet(m_recordCodecCtx, packet) == 0) {
                av_packet_rescale_ts(packet, m_recordCodecCtx->time_base,
                                     m_recordStream->time_base);
                packet->stream_index = m_recordStream->index;
                av_interleaved_write_frame(m_recordFormatCtx, packet);
                av_packet_unref(packet);
            }
            av_packet_free(&packet);
        }
        av_write_trailer(m_recordFormatCtx);
    }

    if (m_recordSwsCtx) {
        sws_freeContext(m_recordSwsCtx);
        m_recordSwsCtx = nullptr;
    }
    if (m_recordFrame) {
        av_frame_free(&m_recordFrame);
    }
    if (m_recordCodecCtx) {
        avcodec_free_context(&m_recordCodecCtx);
    }
    if (m_recordFormatCtx) {
        if (!(m_recordFormatCtx->oformat->flags & AVFMT_NOFILE) &&
            m_recordFormatCtx->pb) {
            avio_closep(&m_recordFormatCtx->pb);
        }
        avformat_free_context(m_recordFormatCtx);
        m_recordFormatCtx = nullptr;
    }
    m_recordStream = nullptr;
    m_recordPts = 0;
    m_recordHeaderWritten = false;
}

// ========== DecodeThread 实现 ==========
FFmpegRtspPlayer::DecodeThread::DecodeThread(FFmpegRtspPlayer *player)
    : m_player(player)
    , m_stopRequested(false)
{
}

void FFmpegRtspPlayer::DecodeThread::run()
{
    //持续读取视频
    if (m_player) {
        m_player->decodeLoop();
    }
}
