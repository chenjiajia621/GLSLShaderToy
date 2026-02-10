#include "rhipingpongitem.h"
#include "myrhiitem.h"
#include <QProcess>

RhiPingPongItem::RhiPingPongItem() {
    connect(this, &QQuickItem::windowChanged, this, &RhiPingPongItem::handleWindowChanged);
    process = new QProcess(this);

}

RhiPingPongItem::~RhiPingPongItem() { releaseResources(); }

// ... (属性 Setters 保持不变) ...
void RhiPingPongItem::setT(float t) {
    if (m_t == t) return;
    m_t = t;
    emit tChanged();
}
void RhiPingPongItem::setMousePos(QPointF p) {
    if (m_mousePos == p) return;
    m_mousePos = p;
    emit mousePosChanged();
}
void RhiPingPongItem::setIsPressed(bool p) {
    if (m_isPressed == p) return;
    m_isPressed = p;
    emit isPressedChanged();
}

// =================================================================
// Sync (核心修改：注入缓存数据)
// =================================================================
void RhiPingPongItem::sync() {
    if (!m_running) return;

    if (!m_renderer) {
        m_renderer = new SquircleRenderer();
        m_renderer->m_window = window();
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &SquircleRenderer::simulate, Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer, &SquircleRenderer::render, Qt::DirectConnection);

        // 【核心修复】: 刚复活时，注入之前的记忆 (缓存数据)
        // 这样 init() 就不会因为数据为空而报错或崩溃
        if (m_cacheLoopNum > 0 && !m_cacheShaders.isEmpty()) {
            m_renderer->mux.lock();
            m_renderer->loopNum = m_cacheLoopNum;
            m_renderer->MyShader = m_cacheShaders;

            // 只有当纹理缓存不为空时才覆盖默认值
            if (!m_cacheTexUrls.isEmpty()) {
                m_renderer->texUrl = m_cacheTexUrls;
            }

            m_renderer->inputBindOrder = m_cacheBindOrder;

            // 只有当绑定数据齐全时，才标记 Reset 触发初始化
            if (!m_cacheBindOrder.empty()) {
                m_renderer->isReset = true;
            }
            m_renderer->mux.unlock();
        }
    }

    const qreal dpr = window()->effectiveDevicePixelRatio();

    // 计算 Item 在窗口中的绝对位置
    QPointF itemPos = this->mapToScene(QPointF(0, 0));

    // 赋值给 renderer 的成员变量
    m_renderer->m_viewportX = (float)(itemPos.x() * dpr);
    m_renderer->m_viewportY = (float)(itemPos.y() * dpr);
    m_renderer->m_viewportW = (float)(width() * dpr);
    m_renderer->m_viewportH = (float)(height() * dpr);


    RenderParams params;
    params.time = m_t;
    params.frame = (int)(m_t * 60.0f);
    params.screenSize = window()->size();
    params.mousePos = m_mousePos;
    params.isPressed = m_isPressed;

    m_renderer->setParams(params);
}

// ... (cleanup, handleWindowChanged, releaseResources 保持不变) ...
void RhiPingPongItem::cleanup() {
    delete m_renderer;
    m_renderer = nullptr;
}
void RhiPingPongItem::handleWindowChanged(QQuickWindow *win) {
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &RhiPingPongItem::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &RhiPingPongItem::cleanup, Qt::DirectConnection);
        win->setColor(Qt::black);
    }
}
void RhiPingPongItem::releaseResources() {
    if (window() && m_renderer) {
        class CleanupJob : public QRunnable {
            SquircleRenderer *r;
        public:
            CleanupJob(SquircleRenderer *renderer) : r(renderer) {}
            void run() override { delete r; }
        };
        window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
        m_renderer = nullptr;
    }
}

// ... (convertToQsb 保持不变) ...
QString RhiPingPongItem::convertToQsb(const QString &fileUrl) {
    QString appDir = QCoreApplication::applicationDirPath();
    QString qsbPath = appDir + "/compiler/qsb.exe";
    if (!QFile::exists(qsbPath)) return QString();

    QString localInputPath;
    if (fileUrl.startsWith("file:")) localInputPath = QUrl(fileUrl).toLocalFile();
    else localInputPath = fileUrl;

    if (!QFile::exists(localInputPath)) return QString();

    QString outPutDir = appDir + "/qsbfile/";
    QDir outputDir(outPutDir);
    if (!outputDir.exists()) outputDir.mkpath(".");

    QString filename = QFileInfo(localInputPath).fileName();
    QString finalOutputPath = outPutDir + filename + ".qsb";

    QStringList args;
    args << "--glsl" << "310 es,440" << "--hlsl" << "50" << "--msl" << "12";
    args << "-o" << finalOutputPath << localInputPath;

    process->setWorkingDirectory(QFileInfo(qsbPath).absolutePath());
    process->setProgram(qsbPath);
    process->setArguments(args);
    process->start();

    if (!process->waitForFinished(5000)) return QString();
    if (process->exitCode() == 0) return finalOutputPath;
    else return QString();
}

// =================================================================
// 交互函数 (修改：总是更新缓存)
// =================================================================

void RhiPingPongItem::getFile(const QStringList &fileList)
{
    // 不再检查 !m_renderer，允许在关闭状态下编译
    if (fileList.count() < 1) return;

    int newLoopNum = fileList.count();
    QStringList finalPaths;

    for (const QString &path : fileList) {
        QString result = convertToQsb(path);
        if (!result.isEmpty()) finalPaths.append(result);
        else return;
    }

    if (finalPaths.count() != fileList.count()) return;

    // 1. 更新缓存
    m_cacheLoopNum = newLoopNum;
    m_cacheShaders = finalPaths;
    m_cacheBindOrder.clear(); // 新的 Shader 来了，旧绑定失效

    // 2. 如果 Renderer 活着，同步更新它
    if (m_renderer) {
        m_renderer->mux.lock();
        m_renderer->loopNum = newLoopNum;
        m_renderer->MyShader = finalPaths;
        m_renderer->inputBindOrder.clear();
        m_renderer->isReset = true;
        m_renderer->mux.unlock();
        window()->update();
    }
}

void RhiPingPongItem::getArr(const QList<int> &arr)
{
    // 1. 更新缓存
    m_cacheBindOrder.clear();
    for(int i=0; i<arr.count(); i++) {
        m_cacheBindOrder.push_back(arr[i]);
    }

    // 2. 如果 Renderer 活着，同步更新它
    if (m_renderer) {
        m_renderer->mux.lock();
        m_renderer->inputBindOrder = m_cacheBindOrder;
        m_renderer->isReset = true;
        m_renderer->mux.unlock();
        window()->update();
    }
}

void RhiPingPongItem::getTexUrl(const QStringList &texUrl)
{
    // 1. 更新缓存
    m_cacheTexUrls = texUrl;

    // 2. 如果 Renderer 活着，同步更新它
    if (m_renderer) {
        m_renderer->mux.lock();
        m_renderer->texUrl = texUrl;
        m_renderer->picIsReset = true;
        m_renderer->isReset = true;
        m_renderer->mux.unlock();
    }
}

void RhiPingPongItem::setRunning(bool r) {
    if (m_running == r) return;
    m_running = r;
    if (m_running) {
        update(); // 唤醒，触发 sync，注入缓存
    } else {
        releaseResources(); // 销毁
    }
    emit runningChanged();
}
