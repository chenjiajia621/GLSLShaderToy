#ifndef STRUCTMODEL_H
#define STRUCTMODEL_H
#include <QSize>
#include <QPointF>
#include <QVector4D>
class QRhiGraphicsPipeline;
class QRhiShaderResourceBindings;
class QRhiTexture;
class QRhiTextureRenderTarget;

// ----------------------------------------------------------------
 // tex tyoe
// ----------------------------------------------------------------
enum class BufferSlot {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    None = -1 // 用于某些不需要输入的特殊情况
};

// ----------------------------------------------------------------
// data from qml
// ----------------------------------------------------------------
struct RenderParams {
    // --- 基础信息 ---
    float time = 0.0f;          // 对应 iTime (运行时间)
    float timeDelta = 0.0f;     // 对应 iTimeDelta (帧间隔)
    int frame = 0;              // 对应 iFrame (帧数)
    QSize screenSize;           // 对应 iResolution (屏幕/纹理大小)

    // --- 鼠标交互 ---
    QPointF mousePos;           // 实时鼠标位置 (xy)
    QPointF mousePressOrigin;   // 点击时的起始位置 (用于计算 zw)
    bool isPressed = false;     // 鼠标是否按下

    // --- 日期信息 ---
    QVector4D date;
};

// ========================================================================
// 2. ShaderToyUniforms: GPU
// ========================================================================
struct ShaderToyUniforms {
    // --- 基础组 (Offset 0) ---
    float iResolution[2];
    float iTime;
    float iTimeDelta;

    // --- 鼠标组 (Offset 16) ---
    float iMouse[4];

    // --- 日期组 (Offset 32) ---
    float iDate[4];

    // --- 杂项组 (Offset 48) ---
    float iSampleRate;
    int iFrame;
    float padding0[2];

    // --- 通道分辨率组 (Offset 64) ---
    float iChannelResolution[16];
};

// ----------------------------------------------------------------
// shader struct
// ----------------------------------------------------------------
struct RenderPass {
    // --- Shader ---
    QString shaderPath;

    // --- 拓扑连接 ---
    BufferSlot outputSlot = BufferSlot::None;
    BufferSlot inputSlot  = BufferSlot::None;

    // --- 资源 (Resources) ---
    // 【新增】每个 Pass 独占的纹理和渲染目标
    // 对于上屏 Pass (Screen)，这两个指针为 nullptr，因为它使用 SwapChain
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;

    // --- 管线状态 (Pipeline State) ---
    std::unique_ptr<QRhiGraphicsPipeline> pipeline;
    std::unique_ptr<QRhiShaderResourceBindings> srb;
};


#endif // STRUCTMODEL_H
