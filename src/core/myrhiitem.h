#ifndef MYRHIITEM_H
#define MYRHIITEM_H

// --- Qt Includes ---
#include <QQuickItem>
#include <QQuickWindow>
#include <QRunnable>
#include <QColor>
#include <QPointF>
#include <QImage>
#include <QStandardPaths>
#include <QDir>
#include <QObject>

//RHI Includes
#include <rhi/qrhi.h>
#include <rhi/qshaderbaker.h>

//Std Includes
#include <memory>
#include <mutex>
#include <vector>

//Local Includes
#include "StructModel.h"

// SquircleRenderer
class SquircleRenderer : public QObject {
    Q_OBJECT
public:
    //初始化与生命周期
    void init(QRhi* rhi, QSize size);
    void initGeometryData();
    void releasePipelines();

    // 2. 参数与逻辑更新
    void setParams(const RenderParams& params) { m_params = params; }
    void updateUniformLogic();

public slots:
    // 渲染槽函数
    void simulate(); // 离屏渲染
    void render();   // 上屏渲染

public:
    //公有成员变量
    float m_viewportX = 0.0f;
    float m_viewportY = 0.0f;
    float m_viewportW = 100.0f; // 给个默认值防止刚启动时黑屏
    float m_viewportH = 100.0f;

    QQuickWindow *m_window = nullptr;
    RenderParams m_params;
    std::mutex mux;

    //Shader
    QStringList MyShader;

    //纹理路径
    QStringList texUrl = {
        ":qt/qml/MyRhi/assets/others/noiseInit.png",
        ":qt/qml/MyRhi/assets/others/picInit.jpg",
        ":qt/qml/MyRhi/assets/others/other.png"
    };

    //核心控制变量
    int loopNum = 0;              // 总Pass数量
    bool isReset = false;         // 重置标志
    bool picIsReset = false;      // 图片重置标注
    std::vector<int> inputBindOrder; //绑定关系数组

    //核心管线容器
    std::vector<std::unique_ptr<RenderPass>> renderPass;

    // RHI: 通用资源
    std::unique_ptr<QRhiBuffer> m_vBuf;
    std::unique_ptr<QRhiBuffer> m_uBuf;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiTexture> m_bgTex[3];
    std::unique_ptr<QRhiRenderPassDescriptor> rpDesc;

private:

    void createPipelines(QRhi *rhi);
    QShader getShader(const QString &name);
    std::vector<float> m_vertexData;
    ShaderToyUniforms m_currentUniforms;
    bool m_isVertexUploaded = false;
};

#endif // MYRHIITEM_H
