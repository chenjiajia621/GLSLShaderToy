#ifndef RHIPINGPONGITEM_H
#define RHIPINGPONGITEM_H
#include <QObject>
#include <QQuickItem>

class SquircleRenderer;
class QProcess;

class RhiPingPongItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(float t READ t WRITE setT NOTIFY tChanged)
    Q_PROPERTY(QPointF mousePos READ mousePos WRITE setMousePos NOTIFY mousePosChanged)
    Q_PROPERTY(bool isPressed READ isPressed WRITE setIsPressed NOTIFY isPressedChanged)
    // 运行控制属性
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

public:
    RhiPingPongItem();
    ~RhiPingPongItem();

    float t() const { return m_t; }
    void setT(float t);

    QPointF mousePos() const { return m_mousePos; }
    void setMousePos(QPointF p);

    bool isPressed() const { return m_isPressed; }
    void setIsPressed(bool p);

    bool running() const { return m_running; }
    void setRunning(bool r);

    QString convertToQsb(const QString &fileUrl);

    Q_INVOKABLE void getFile(const QStringList &fileList);
    Q_INVOKABLE void getTexUrl(const QStringList &texUrl);
    Q_INVOKABLE void getArr(const QList<int> &arr);

    QProcess *process = nullptr;

signals:
    void tChanged();
    void mousePosChanged();
    void isPressedChanged();
    void runningChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    void releaseResources();
    SquircleRenderer *m_renderer = nullptr;

    float m_t = 0.0f;
    QPointF m_mousePos;
    bool m_isPressed = false;
    bool m_running = true;

    // ==========================================
    // 【新增】数据缓存 (Cache)
    // 即使 m_renderer 被销毁，这些数据也会保留
    // ==========================================
    int m_cacheLoopNum = 0;
    QStringList m_cacheShaders;     // 存储编译后的 .qsb 路径
    QStringList m_cacheTexUrls;     // 存储纹理路径
    std::vector<int> m_cacheBindOrder; // 存储绑定数组
};

#endif // RHIPINGPONGITEM_H
