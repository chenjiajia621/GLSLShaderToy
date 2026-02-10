#include "MyRhiItem.h"
#include <QFile>
#include "StructModel.h"
#include <QDirIterator>
#include <QDebug>

void SquircleRenderer::init(QRhi* rhi, QSize size) {
    // 1. 检查重建逻辑
    bool needRebuild = isReset || renderPass.empty();

    if (!renderPass.empty() && renderPass[0]->texture) {
        if (renderPass[0]->texture->pixelSize() != size) {
            qDebug() << "[Init] Size changed from" << renderPass[0]->texture->pixelSize() << "to" << size << ", rebuilding...";
            needRebuild = true;
        }
    }

    // 基础检查
    if (!needRebuild) return;

    // 【调试】输出关键状态
    qDebug() << "[Init] Triggered. LoopNum:" << loopNum
             << "BindOrder Size:" << inputBindOrder.size()
             << "IsReset:" << isReset;

    if (loopNum == 0) {
        qDebug() << "[Init] LoopNum is 0, aborting.";
        return;
    }
    // 【关键】防止空绑定导致后续逻辑崩溃
    if (inputBindOrder.empty()) {
        qDebug() << "[Init] inputBindOrder is empty! Waiting for getArr(). Aborting.";
        return;
    }

    std::lock_guard<std::mutex> lock(mux);

    qDebug() << "[Init] Clearing old passes...";
    renderPass.clear();
    rpDesc.reset();
    isReset = false;

    // loopNum 是总数，取最小值更安全
    int safeLoopNum = std::min((int)MyShader.size(), loopNum);
    qDebug() << "[Init] SafeLoopNum:" << safeLoopNum << "(Shaders:" << MyShader.size() << ")";

    for(int i = 0; i < safeLoopNum; i++)
    {
        auto initRendPass = std::make_unique<RenderPass>();
        initRendPass->shaderPath = MyShader[i];

        bool isScreen = (i == safeLoopNum - 1);
        qDebug() << "  [Init] Building Pass" << i << "IsScreen:" << isScreen << "Path:" << MyShader[i];

        // A. 创建资源 (仅限离屏 Pass)
        if (!isScreen) {
            initRendPass->texture.reset(rhi->newTexture(QRhiTexture::RGBA16F, size, 1, QRhiTexture::RenderTarget));
            initRendPass->texture->create();

            QRhiTextureRenderTargetDescription desc;
            desc.setColorAttachments({ QRhiColorAttachment(initRendPass->texture.get()) });

            initRendPass->renderTarget.reset(rhi->newTextureRenderTarget(desc));

            if (!rpDesc) {
                rpDesc.reset(initRendPass->renderTarget->newCompatibleRenderPassDescriptor());
            }
            initRendPass->renderTarget->setRenderPassDescriptor(rpDesc.get());
            initRendPass->renderTarget->create();
            qDebug() << "    -> Offscreen resources created.";
        }
        else {
            initRendPass->texture = nullptr;
            initRendPass->renderTarget = nullptr;
            qDebug() << "    -> Screen pass (no texture created).";
        }

        // B. 配置输入绑定
        if (i < inputBindOrder.size()) {
            initRendPass->inputSlot = static_cast<BufferSlot>(inputBindOrder[i]);
            qDebug() << "    -> InputSlot set to:" << inputBindOrder[i];
        } else {
            initRendPass->inputSlot = BufferSlot::None;
            qDebug() << "    -> [WARNING] No bind order for this pass! Set to None.";
        }

        renderPass.push_back(std::move(initRendPass));
    }
    qDebug() << "[Init] Finished. RenderPass count:" << renderPass.size();
}

// ========================================================================
// Helpers
// ========================================================================
QShader SquircleRenderer::getShader(const QString &name) {
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[Shader] Failed to open:" << name;
        return QShader();
    }
    return QShader::fromSerialized(f.readAll());
}

void SquircleRenderer::initGeometryData() {
    m_vertexData = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f
    };
    m_isVertexUploaded = false;
}

void SquircleRenderer::releasePipelines() {
    // 可以在这里重置所有管线
    std::lock_guard<std::mutex> lock(mux);
    qDebug() << "[Release] Releasing pipelines...";
    for(auto& pass : renderPass) {
        pass->pipeline.reset();
    }
}

// ========================================================================
// Uniform Logic
// ========================================================================
void SquircleRenderer::updateUniformLogic() {
    if (renderPass.empty()) return;

    // 【修改】直接使用 Viewport 尺寸，或者纹理尺寸
    QSize texSize;
    if (renderPass[0]->texture) {
        texSize = renderPass[0]->texture->pixelSize();
    } else {
        // 如果是直接上屏的 Pass，使用我们的 m_viewportW/H
        texSize = QSize((int)m_viewportW, (int)m_viewportH);
    }

    // 【修改】鼠标坐标换算逻辑简化
    // 之前是用 texSize/winSize 算比例，现在直接乘 DPR 即可
    // 因为 MouseArea 的坐标是逻辑坐标，Shader 需要物理像素坐标
    float dpr = m_window->effectiveDevicePixelRatio();

    m_currentUniforms.iResolution[0] = (float)texSize.width();
    m_currentUniforms.iResolution[1] = (float)texSize.height();
    m_currentUniforms.iTime = m_params.time;
    m_currentUniforms.iTimeDelta = 0.016f;

    // 鼠标坐标直接映射
    float mx = m_params.mousePos.x() * dpr;
    float my = m_params.mousePos.y() * dpr;
    // 如果需要翻转Y轴 (取决于Shader逻辑，ShaderToy通常原点在左下角)
    // my = (float)texSize.height() - my;

    m_currentUniforms.iMouse[0] = mx;
    m_currentUniforms.iMouse[1] = my;
    m_currentUniforms.iMouse[2] = m_params.isPressed ? 1.0f : -1.0f;
    m_currentUniforms.iMouse[3] = 0.0f;
    m_currentUniforms.iFrame = m_params.frame;

    for (int i = 0; i < 4; ++i) {
        m_currentUniforms.iChannelResolution[4 * i + 0] = (float)texSize.width();
        m_currentUniforms.iChannelResolution[4 * i + 1] = (float)texSize.height();
        m_currentUniforms.iChannelResolution[4 * i + 2] = 1.0f;
        m_currentUniforms.iChannelResolution[4 * i + 3] = 0.0f;
    }
}

// ========================================================================
// Create Pipelines (【重写】修正了你代码中的旧逻辑)
// ========================================================================
void SquircleRenderer::createPipelines(QRhi *rhi) {
    QRhiVertexInputLayout inputLayout;

    // 【核心修复】这里是步长(Stride)，不是总大小！
    // 一个顶点只有 x,y 两个 float，所以是 2 * sizeof(float)
    inputLayout.setBindings({{ 2 * sizeof(float) }});

    inputLayout.setAttributes({{ 0, 0, QRhiVertexInputAttribute::Float2, 0 }});

    // 1. 通用资源
    if (!m_vBuf) {
        m_vBuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 8 * sizeof(float)));
        m_vBuf->create();
        qDebug() << "[Resource] Vertex Buffer created.";
    }
    if (!m_uBuf) {
        m_uBuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(ShaderToyUniforms)));
        m_uBuf->create();
        qDebug() << "[Resource] Uniform Buffer created.";
    }
    if (!m_sampler) {
        m_sampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        m_sampler->create();
    }

    // 2. 调用 Init
    init(rhi, QSize((int)m_viewportW, (int)m_viewportH));

    // 3. Reset 检查
    {
        std::lock_guard<std::mutex> lock(mux);
        if (this->isReset) {
            qDebug() << "[Pipeline] isReset is true, clearing pipelines...";
            for (auto& pass : renderPass) {
                pass->pipeline.reset();
                pass->srb.reset();
            }
            this->isReset = false;
        }
    }

    mux.lock();
    if(this->picIsReset)
    {
        for(int i=0;i<std::size(m_bgTex);i++)
        {
            m_bgTex[i].reset();
        }
        picIsReset=false;
    }
    mux.unlock();

    // 4. 加载底图
    if (!m_bgTex[0]) {
        qDebug() << "[Resource] Loading background textures...";
        auto *rub = rhi->nextResourceUpdateBatch();

        // 【新增 1】获取目标渲染框尺寸
        int targetW = (int)m_viewportW;
        int targetH = (int)m_viewportH;

        for(int i=0; i<3; i++) {
            QString path = texUrl[i];
            QImage image(path);
            if (image.isNull()) {
                qWarning() << "  [Warn] Failed to load image:" << path << ". Using red placeholder.";
                image = QImage(64, 64, QImage::Format_RGBA8888);
                image.fill(Qt::red);
            }

            // Aspect Fill)
            if (targetW > 0 && targetH > 0) {
                // A. 缩放：Qt::KeepAspectRatioByExpanding 会保证图片塞满框，短边对齐，长边溢出
                image = image.scaled(targetW, targetH,
                                     Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);

                // B. 裁剪：计算居中偏移量，切掉溢出的部分
                int x = (image.width() - targetW) / 2;
                int y = (image.height() - targetH) / 2;
                image = image.copy(x, y, targetW, targetH);
            }
            // =========================================================

            image = image.convertToFormat(QImage::Format_RGBA8888);

            m_bgTex[i].reset(rhi->newTexture(QRhiTexture::RGBA8, image.size(), 1));
            m_bgTex[i]->create();
            rub->uploadTexture(m_bgTex[i].get(), image);
        }

        initGeometryData();
        if (!m_vertexData.empty()) {
            rub->uploadStaticBuffer(m_vBuf.get(), m_vertexData.data());
        }
        m_window->swapChain()->currentFrameCommandBuffer()->resourceUpdate(rub);
    }

    // 5. 【核心】遍历 renderPass 创建管线
    for (size_t i = 0; i < renderPass.size(); ++i) {
        auto& pass = renderPass[i];
        if (pass->pipeline) continue;

        qDebug() << "[Pipeline] Creating pipeline for Pass" << i;

        bool isScreenPass = (pass->texture == nullptr);

        // A. 动态输入纹理
        QRhiTexture* inputTexture = m_bgTex[0].get();
        int inputIdx = static_cast<int>(pass->inputSlot);

        // 根据 InputSlot 找到对应的 Texture
        if (inputIdx >= 0 && inputIdx < renderPass.size()) {
            if (renderPass[inputIdx]->texture) {
                inputTexture = renderPass[inputIdx]->texture.get();
                qDebug() << "  -> Linked Input: Pass" << i << "reads from Pass" << inputIdx;
            } else {
                qDebug() << "  -> [Warn] Pass" << inputIdx << "has no texture (maybe Screen?). Using fallback.";
            }
        } else {
            qDebug() << "  -> Input: Default/None (Idx:" << inputIdx << ")";
        }

        // B. SRB
        pass->srb.reset(rhi->newShaderResourceBindings());
        pass->srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uBuf.get()),
            // Binding 1 是动态输入 (PingPong 结果)
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture, m_sampler.get()),
            // Binding 2,3,4 是固定底图
            QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_bgTex[0].get(), m_sampler.get()),
            QRhiShaderResourceBinding::sampledTexture(3, QRhiShaderResourceBinding::FragmentStage, m_bgTex[1].get(), m_sampler.get()),
            QRhiShaderResourceBinding::sampledTexture(4, QRhiShaderResourceBinding::FragmentStage, m_bgTex[2].get(), m_sampler.get()),
        });
        pass->srb->create();

        // C. Pipeline
        pass->pipeline.reset(rhi->newGraphicsPipeline());
        pass->pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        pass->pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, getShader(":/myfile/common.vert.qsb") },
            { QRhiShaderStage::Fragment, getShader(pass->shaderPath) }
        });
        pass->pipeline->setVertexInputLayout(inputLayout);
        pass->pipeline->setShaderResourceBindings(pass->srb.get());

        if (isScreenPass) {
            pass->pipeline->setRenderPassDescriptor(m_window->swapChain()->currentFrameRenderTarget()->renderPassDescriptor());
            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::One;
            pass->pipeline->setTargetBlends({ blend });
        } else {
            pass->pipeline->setRenderPassDescriptor(rpDesc.get());
        }

        if (!pass->pipeline->create()) {
            qCritical() << "  -> [Error] Failed to create pipeline for Pass" << i;
        } else {
            qDebug() << "  -> Pipeline created successfully.";
        }
    }
}

// ========================================================================
// Simulate (【重写】修正了旧变量引用)
// ========================================================================
void SquircleRenderer::simulate() {
    QRhi *rhi = m_window->rhi();
    if (!rhi) return;

    createPipelines(rhi);

    auto* rub = rhi->nextResourceUpdateBatch();
    updateUniformLogic();
    rub->updateDynamicBuffer(m_uBuf.get(), 0, sizeof(ShaderToyUniforms), &m_currentUniforms);

    auto* cb = m_window->swapChain()->currentFrameCommandBuffer();
    cb->resourceUpdate(rub);

    // 遍历执行所有离屏 Pass
    int execCount = 0;
    for(size_t i = 0; i < renderPass.size(); i++)
    {
        auto& pass = renderPass[i];

        if (pass->renderTarget && pass->pipeline) {
            cb->beginPass(pass->renderTarget.get(), Qt::transparent, {1.0f, 0});
            cb->setGraphicsPipeline(pass->pipeline.get());
            QSize size = pass->texture->pixelSize();
            cb->setViewport({0, 0, (float)size.width(), (float)size.height()});
            cb->setShaderResources(pass->srb.get());

            const QRhiCommandBuffer::VertexInput vbuf(m_vBuf.get(), 0);
            cb->setVertexInput(0, 1, &vbuf);
            cb->draw(4);
            cb->endPass();
            execCount++;
        }
    }
}

void SquircleRenderer::render() {
    if (renderPass.empty()) return;

    // 上屏 Pass 是最后一个
    auto& screenPass = renderPass.back();
    // 检查管线是否存在
    if (!screenPass->pipeline) return;

    auto* cb = m_window->swapChain()->currentFrameCommandBuffer();

    // 1. 绑定管线
    cb->setGraphicsPipeline(screenPass->pipeline.get());

    // 2. 设置视口
    QSize s = m_window->swapChain()->currentFrameRenderTarget()->pixelSize();
    cb->setViewport({ m_viewportX, m_viewportY, m_viewportW, m_viewportH });

    // 3. 绑定资源
    cb->setShaderResources(screenPass->srb.get());

    // 4. 绑定顶点并绘制
    const QRhiCommandBuffer::VertexInput vbuf(m_vBuf.get(), 0);
    cb->setVertexInput(0, 1, &vbuf);
    cb->draw(4);

    // 【注意】不要调用 cb->endPass()，Qt 会自己处理

    // 保持刷新
    m_window->update();
}
