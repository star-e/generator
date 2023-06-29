#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

// clang-format off

namespace Cocos::Meta {
    
void buildRenderInterfaceDocument(ModuleBuilder& builder) {
    NAMESPACE_BEG(cc);
    NAMESPACE_BEG(render);

    COMMENT(PipelineRuntime, R"(@engineInternal
@en PipelineRuntime is the runtime of both classical and custom pipelines.
It is used internally and should not be called directly.
@zh PipelineRuntime是经典管线以及自定义管线的运行时。
属于内部实现，用户不应直接调用。
)") {
        METHOD_COMMENT(activate, R"(@en Activate PipelineRuntime with default swapchain
@zh 用默认交换链初始化PipelineRuntime
@param swapchain @en Default swapchain @zh 默认的交换链
@returns Success or not
)");
        METHOD_COMMENT(destroy, R"(@en Destroy resources of PipelineRuntime
@zh 销毁PipelineRuntime所持资源
@returns Success or not
)");
        METHOD_COMMENT(render, R"(@en Render contents of cameras
@zh 根据相机进行绘制
@param cameras @en Camera list @zh 相机列表
)");
        METHOD_COMMENT(getDevice, R"(@en Get graphics device
@zh 获得图形设备
)");
        METHOD_COMMENT(getMacros, R"(@en Get user macros
@zh 获得用户宏列表
)");
        METHOD_COMMENT(getGlobalDSManager, R"(@en Get global descriptor set manager
@zh 获得全局(Global)级别描述符集(DescriptorSet)管理器
)");
        METHOD_COMMENT(getDescriptorSetLayout, R"(@en Get global descriptor set layout
@zh 获得全局(Global)级别描述符集的布局(DescriptorSet Layout)
)");
        METHOD_COMMENT(getDescriptorSet, R"(@en Get global descriptor set
@zh 获得全局(Global)级别描述符集(DescriptorSet)
)");
        METHOD_COMMENT(getCommandBuffers, R"(@en Get command buffers of render pipeline
@zh 获得渲染管线的命令缓冲(CommandBuffer)列表
)");
        METHOD_COMMENT(getPipelineSceneData, R"(@en Get scene data of render pipeline.
Scene data contains render configurations of the current scene.
@zh 获得渲染管线相关的场景数据，此场景数据一般包含渲染所需配置信息
)");
        METHOD_COMMENT(getConstantMacros, R"(@en Get constant macros.
Constant macro is platform-dependent and immutable.
@zh 获得常量宏列表，常量宏平台相关且无法修改
)");
        METHOD_COMMENT(getProfiler, R"(@en Get profiler model.
This model is used to render profile information in Debug mode.
@zh 获得分析工具(Profiler)的渲染实例，用于Debug模式下显示调试与性能检测信息
)");
        METHOD_COMMENT(getGeometryRenderer, R"(@en Get geometry renderer.
Geometry renderer is used to render procedural geometries.
@zh 获得几何渲染器(GeometryRenderer)，几何渲染器用于程序化渲染基础几何图形
)");
        METHOD_COMMENT(getShadingScale, R"(@en Get shading scale.
Shading scale affects shading texels per pixel.
@zh 获得渲染倍率(ShadingScale)，每像素(pixel)绘制的纹素(texel)会根据渲染倍率进行调整。
)");
        METHOD_COMMENT(getMacroString, R"(@en Get macro as string.
@zh 根据宏名获得字符串
@param name @en Name of macro @zh 宏的名字
@returns String value
)");
        METHOD_COMMENT(getMacroInt, R"(@en Get macro as integer.
@zh 根据宏名获得整型
@param name @en Name of macro @zh 宏的名字
@returns Integer value
)");
        METHOD_COMMENT(getMacroBool, R"(@en Get macro as boolean.
@zh 根据宏名获得布尔值
@param name @en Name of macro @zh 宏的名字
@returns Boolean value
)");
        METHOD_COMMENT(setMacroString, R"(@en Assign string value to macro.
@zh 给宏赋值字符串
@param name @en Name of macro @zh 宏的名字
@param value @en String value @zh 字符串
)");
        METHOD_COMMENT(setMacroInt, R"(@en Assign integer value to macro.
@zh 给宏赋值整型
@param name @en Name of macro @zh 宏的名字
@param value @en Integer value @zh 整型值
)");
        METHOD_COMMENT(setMacroBool, R"(@en Assign boolean value to macro.
@zh 给宏赋值布尔值
@param name @en Name of macro @zh 宏的名字
@param value @en Boolean value @zh 布尔值
)");
        METHOD_COMMENT(onGlobalPipelineStateChanged, R"(@en Trigger pipeline state change event
@zh 触发管线状态更新事件
)");
    } // PipelineRuntime

    COMMENT(PipelineType, R"(@en Type of render pipeline.
Different types of pipeline have different hardward capabilities and interfaces.
@zh 管线类型，不同类型的管线具有不同的硬件能力与接口
)") {
        ENUM_COMMENT(BASIC, R"(@en Basic render pipeline.
Basic render pipeline is available on all platforms.
The corresponding interface is {@link BasicPipeline}
@zh 基础渲染管线，全平台支持。对应接口为 {@link BasicPipeline}
)");
        ENUM_COMMENT(STANDARD, R"(@en Standard render pipeline.
Standard render pipeline supports compute shader and subpass rendering.
It works well on Tile-based GPUs and is available on all native platforms.
Vulkan, GLES3 and Metal backends are supported.
The corresponding interface is {@link Pipeline}
@zh 标准渲染管线.
标准渲染管线支持计算着色器(Compute Shader)与次通道渲染(Subpass rendering)。
能充分利用Tile-based GPU，支持所有原生平台并对移动平台特别优化。
支持Vulkan、GLES3、Metal图形后端。
对应接口为{@link Pipeline}
)");
    } // PipelineType

    COMMENT(SubpassCapabilities, R"(@en Render subpass capabilities.
Tile-based GPUs support reading color or depth_stencil attachment in pixel shader.
Our implementation is based-on Vulkan abstraction (aka input attachment),
and it is emulated on other graphics backends.
For example, in GLES3 we have used various framebuffer fetch (FBF) extensions.
As a result, different backends and hardwares support different input attachment features.
User should inspect pipeline capabilities when implementing tile-based rendering algorithms.
Using unsupported feature is undefined behaviour.
@zh 次通道渲染能力
Tile-based GPU可以在像素着色器读取当前像素的值。
我们的抽象方式基于Vulkan的input attachment，并在其他图形后端模拟了这个功能。
比如在GLES3上，我们使用了多种framebuffer fetch (FBF) 扩展来实现这个功能。
所以对于不同的硬件以及图形API，支持的能力是略有不同的。
在编写渲染算法时，应该查询当前设备的能力，来选择合适的tile-based算法。
使用硬件不支持的特性，会导致未定义行为。
)") {
        ENUM_COMMENT(INPUT_DEPTH_STENCIL, R"(@en Supports read depth/stencil value at current pixel.
@zh 支持读取当前像素的depth/stencil值
)");
        ENUM_COMMENT(INPUT_COLOR, R"(@en Supports read color value 0 at current pixel.
@zh 支持读取当前像素第0个颜色值
)");
        ENUM_COMMENT(INPUT_COLOR_MRT, R"(@en Supports read color values at current pixel.
@zh 支持读取当前像素任意颜色值
)");
    }
    
    COMMENT(PipelineCapabilities, R"(@en Pipeline capabilities.
The following capabilities are partially supported on different hardware and graphics backends.
@zh 管线能力。根据硬件与后端，支持的特性会有所不同
)") {
    }

    COMMENT(RenderNode, R"(@en Base class of render graph node.
A node of render graph represents a specific type of rendering operation.
A render graph consists of these nodes and form a forest(which is a set of trees).
@zh RenderGraph中节点的基类，每个RenderGraph节点代表一种渲染操作，并构成一个森林(一组树)
)") {
        METHOD_COMMENT(getName, R"(@en Get debug name of current node.
@zh 获得当前节点调试用的名字
)");
    }

    COMMENT(Setter, R"(@en Render node which supports setting uniforms and descriptors.
@zh 节点支持设置常量值(uniform/constant)与描述符
)") {
        METHOD_COMMENT(setMat4, R"(@en Set matrix4x4 常量(uniform) which consists of 16 floats (64 bytes).
@zh 设置4x4矩阵，常量(uniform)有16个float (64 bytes)
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
)");
        METHOD_COMMENT(setQuaternion, R"(@en Set quaternion uniform which consists of 4 floats (16 bytes).
@zh 设置四元数向量，常量(uniform)有4个float (16 bytes)
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
)");
        METHOD_COMMENT(setColor, R"(@en Set color uniform which consists of 4 floats (16 bytes).
@zh 设置颜色值，常量(uniform)有4个float (16 bytes)
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
)");
        METHOD_COMMENT(setVec4, R"(@en Set vector4 uniform which consists of 4 floats (16 bytes).
@zh 设置vector4向量，常量(uniform)有4个float (16 bytes)
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
)");
        METHOD_COMMENT(setVec2, R"(@en Set vector2 uniform which consists of 2 floats (8 bytes).
@zh 设置vector2向量，常量(uniform)有2个float (8 bytes)
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
)");
        METHOD_COMMENT(setFloat, R"(@en Set float uniform (4 bytes).
@zh 设置浮点值 (4 bytes)
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
)");
        METHOD_COMMENT(setArrayBuffer, R"(@en Set uniform array.
Size and type of the data should match the corresponding uniforms in the shader.
Mismatches will cause undefined behaviour.
Memory alignment is not required.
@zh 设置数组。类型与大小需要与着色器中的常量(uniform)相匹配，不匹配会引起未定义行为。
内存地址不需要对齐。
@param name @en uniform name in shader. @zh 填写着色器中的常量(uniform)名字
@param arrayBuffer @en array of bytes @zh byte数组
)");
        METHOD_COMMENT(setBuffer, R"(@en Set buffer descriptor.
Size and type of the buffer should match the one in shader.
Buffer should be in read states and satisfy shader stage visibilities.
Mismatches will cause undefined behaviour.
@zh 设置缓冲(buffer)描述符。大小与类型需要与着色器中的一致，处于只读状态且着色阶段可见。
不匹配会引起未定义行为。
@param name @en descriptor name in shader. @zh 填写着色器中的描述符(descriptor)名字
@param buffer @en readonly buffer @zh 只读的缓冲
)");
        METHOD_COMMENT(setTexture, R"(@en Set texture descriptor.
Type of the texture should match the one in shader.
Texture should be in read states and satisfy shader stage visibilities.
Mismatches will cause undefined behaviour.
@zh 设置贴图描述符。类型需要与着色器中的一致，处于只读状态且着色阶段可见。
不匹配会引起未定义行为。
@param name @en descriptor name in shader. @zh 填写着色器中的描述符(descriptor)名字
@param texture @en readonly texture @zh 只读的贴图
)");
        METHOD_COMMENT(setSampler, R"(@en Set sampler descriptor.
Type of the sampler should match the one in shader.
@zh 设置采样器描述符。类型需要与着色器中的一致。
不匹配会引起未定义行为。
@param name @en descriptor name in shader. @zh 填写着色器中的描述符(descriptor)名字
)");
    }
    
    auto setViewport = [&](uint32_t vertID){
        builder.addMethodComment(vertID, "setViewport", R"(@en Set rendering viewport.
@zh 设置渲染视口
@param viewport @en The required viewport @zh 所需视口
)");
    };

    COMMENT(RenderQueueBuilder, R"(@en Render queue
A render queue is an abstraction of graphics commands submission.
Only when the graphics commands in a render queue are all submitted,
the next render queue will start submitting.
@zh 渲染队列。渲染队列是图形命令提交的抽象。
只有一个渲染队列中的渲染命令全部提交完，才会开始提交下一个渲染队列中的命令。
)") {
        METHOD_COMMENT(addSceneOfCamera, R"(@en Render the scene the camera is looking at.
@zh 渲染当前相机指向的场景。
@param camera @en Required camera @zh 所需相机
@param light @en Lighting information of the scene @zh 场景光照信息
@param sceneFlags @en Rendering flags of the scene @zh 场景渲染标志位
)");
        METHOD_COMMENT(addFullscreenQuad, R"(@en Render a full-screen quad.
@zh 渲染全屏四边形
@param material @en The material used for shading @zh 着色所需材质
@param passID @en Material pass ID @zh 材质通道ID
@param sceneFlags @en Rendering flags of the quad @zh Quad所需场景渲染标志位
)");
        METHOD_COMMENT(addCameraQuad, R"(@en Render a full-screen quad from the camera view.
@zh 从相机视角渲染全屏四边形
@param camera @en The required camera @zh 所需相机
@param material @en The material used for shading @zh 着色所需材质
@param passID @en Material pass ID @zh 材质通道ID
@param sceneFlags @en Rendering flags of the quad @zh Quad所需场景渲染标志位
)");
        METHOD_COMMENT(clearRenderTarget, R"(@en Clear current render target.
@zh 清除当前渲染目标
@param name @en The name of the render target @zh 渲染目标的名字
@param color @en The clearing color @zh 用来清除与填充的颜色
)");
        setViewport(vertID);
    }

    auto addRenderTarget = [&](uint32_t vertID, bool subpass = false) {
        std::ostringstream oss;
        oss << R"(@en Add render target for rasterization
The render target must have registered in pipeline.
@zh 添加光栅化渲染目标，渲染目标必须已注册。
@param name @en name of the render target @zh 渲染目标的名字
)";
        if (subpass) {
            oss << "@param accessType @en Access type @zh 读写状态\n";
            oss << "@param slotName @en name of the descriptor in shader @zh 着色器中描述符的名字\n";
        }
        oss << R"(@param loadOp @en Type of load operation @zh 读取操作的类型
@param storeOp @en Type of store operation @zh 写入操作的类型
@param color @en The clear color to use when loadOp is Clear @zh 读取操作为清除时，所用颜色
)";
        builder.addMethodComment(vertID, "addRenderTarget", oss.str());
    };

    auto addDepthStencil = [&](uint32_t vertID, bool subpass = false) {
        std::ostringstream oss;
        oss << R"(@en Add depth stencil for rasterization
The depth stencil must have registered in pipeline.
@zh 添加光栅化深度模板缓冲，深度模板缓冲必须已注册。
@param name @en name of the depth stencil @zh 渲染目标的名字
)";
        if (subpass) {
            oss << "@param accessType @en Access type @zh 读写状态\n";
            oss << "@param depthSlotName @en name of the depth descriptor in shader @zh 着色器中深度描述符的名字\n";
            oss << "@param stencilSlotName @en name of the stencil descriptor in shader @zh 着色器中模板描述符的名字\n";
        }
        oss << R"(@param loadOp @en Type of load operation @zh 读取操作的类型
@param storeOp @en Type of store operation @zh 写入操作的类型
@param depth @en Depth value used to clear @zh 用于清除的深度值
@param stencil @en Stencil value used to clear @zh 用于清除的模板值
@param clearFlags @en To clear depth, stencil or both @zh 清除分量：深度、模板、两者。
)";
        builder.addMethodComment(vertID, "addDepthStencil", oss.str());
    };

    auto addTexture = [&](uint32_t vertID) {
        builder.addMethodComment(vertID, "addTexture", R"(@en Add texture for sampling
The texture must have registered in pipeline.
@zh 添加采样用的贴图，贴图必须已注册。
@param name @en name of the texture @zh 贴图的注册名
@param slotName @en name of descriptor in the shader @zh 着色器中描述符的名字
@param sampler @en the sampler to use @zh 采样器名字
@param plane @en the image plane ID to sample (color|depth|stencil|video) @zh 需要采样的贴图平面(颜色|深度|模板|视频)
)");
    };

    auto addQueue = [&](uint32_t vertID, bool bCompute) {
        std::ostringstream oss;
        oss << R"(@en Add render queue.
Every render queue has a hint type, such as NONE, OPAQUE, MASK or BLEND.
User should only add objects of this hint type to the render queue.
Objects of mixed types might cause downgrading of performance.
The order of render queues should be adjusted according to the hardward and algorithms,
in order to reach peak performance.
For example, [1.opaque, 2.mask, 3.blend] might result in best performance on mobile platforms.
This hint is for validation only and has no effect on rendering.

Every render queue has a phase name. Only objects of the same phase name will be rendered.

@zh 添加渲染队列
每个渲染队列有一个用途提示，例如无提示(NONE)、不透明(OPAQUE)、遮罩(MASK)和混合(BLEND)。
每个队列最好只渲染相匹配的对象，混合不同类型的对象，会造成性能下降。
不同类型队列的渲染顺序，需要根据硬件类型与渲染算法进行调整，以到达最高性能。
比如在移动平台上，先渲染OPAQUE，再渲染MASK、最后渲染BLEND可能会有最好的性能。
用途提示只用于问题检测，对渲染流程没有任何影响。

每个队列有一个相位(phase)名字，具有相同相位名字的物件才会被渲染。

@param hint @en Usage hint of the queue @zh 用途的提示
@param phaseName @en The name of the phase declared in the effect. Default value is 'default' @zh effect中相位(phase)的名字，缺省为'default'。
)";
        if (bCompute) {
            oss << "@returns @en compute queue builder @zh 计算队列\n";
        } else {
            oss << "@returns @en render queue builder @zh 渲染队列\n";
        }
        builder.addMethodComment(vertID, "addQueue", oss.str());
    };

    auto showStatistics = [&](uint32_t vertID) {
        builder.addMethodComment(vertID, "getShowStatistics", R"(@en Show statistics on screen
@zh 在屏幕上渲染统计数据
)");
    };

    COMMENT(BasicRenderPassBuilder, R"(@en Basic render pass.
@zh 基础光栅通道
)") {
        addRenderTarget(vertID);
        addDepthStencil(vertID);
        addTexture(vertID);
        addQueue(vertID, false);
        setViewport(vertID);
        showStatistics(vertID);
    }

    COMMENT(BasicPipeline, R"(@en BasicPipeline
Basic pipeline provides basic rendering features which are supported on all platforms.
User can register resources which will be used in the render graph.
Theses resources are generally read and write, and will be managed by the pipeline.
In each frame, user can create a render graph to be executed by the pipeline.
@zh 基础渲染管线。
基础渲染管线提供基础的渲染能力，能在全平台使用。
用户可以在渲染管线中注册资源，这些资源将由管线托管，用于render graph。
这些资源一般是可读写的资源。
用户可以每帧构建一个render graph，然后交由管线执行。
)") {
        METHOD_COMMENT(beginSetup, R"(@engineInternal
@en Begin render pipeline setup
@zh 开始管线构建
)");
        METHOD_COMMENT(endSetup, R"(@engineInternal
@en End render pipeline setup
@zh 结束管线构建
)");
        METHOD_COMMENT(containsResource, R"(@en Check whether the resource has been registered in the pipeline.
@zh 检查资源是否在管线中已注册
@param name @en Resource name @zh 资源名字
@returns Exist or not
)");
        METHOD_COMMENT(addRenderWindow, R"(@en Add render window to the pipeline.
@zh 注册渲染窗口(RenderWindow)
@param name @en Resource name @zh 资源名字
@param format @en Expected format of the render window @zh 期望的渲染窗口格式
@param width @en Expected width of the render window @zh 期望的渲染窗口宽度
@param height @en Expected height of the render window @zh 期望的渲染窗口高度
@param renderWindow @en The render window to add. @zh 需要注册的渲染窗口
@returns Resource ID
)");
        METHOD_COMMENT(updateRenderWindow, R"(@en Update render window information.
When render window information is updated, such as resized, user should notify the pipeline.
@zh 更新渲染窗口信息。当渲染窗口发生更新时，用户应通知管线。
@param renderWindow @en The render window to update. @zh 渲染窗口
)");
        METHOD_COMMENT(addRenderTarget, R"(@en Add 2D render target.
@zh 添加2D渲染目标
@param name @en Resource name @zh 资源名字
@param format @en Format of the resource @zh 资源的格式
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param residency @en Residency of the resource. @zh 资源的驻留性
@returns Resource ID
)");
        METHOD_COMMENT(addDepthStencil, R"(@en Add 2D depth stencil.
@zh 添加2D深度模板缓冲
@param name @en Resource name @zh 资源名字
@param format @en Format of the resource @zh 资源的格式
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param residency @en Residency of the resource. @zh 资源的驻留性
@returns Resource ID
)");
        METHOD_COMMENT(updateRenderTarget, R"(@en Update render target information.
@zh 更新渲染目标的信息
@param name @en Resource name @zh 资源名字
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param format @en Format of the resource @zh 资源的格式
)");
        METHOD_COMMENT(updateDepthStencil, R"(@en Update depth stencil information.
@zh 更新深度模板缓冲的信息
@param name @en Resource name @zh 资源名字
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param format @en Format of the resource @zh 资源的格式
)");
        METHOD_COMMENT(beginFrame, R"(@engineInternal
@en Begin rendering one frame
@zh 开始一帧的渲染
)");
        METHOD_COMMENT(update, R"(@engineInternal
@en Update camera
@zh 更新相机
@param camera @en Camera @zh 相机
)");
        METHOD_COMMENT(endFrame, R"(@engineInternal
@en End rendering one frame
@zh 结束一帧的渲染
)");
        METHOD_COMMENT(addRenderPass, R"(@en Add render pass
@zh 添加渲染通道
@param width @en Width of the render pass @zh 渲染通道的宽度
@param height @en Height of the render pass @zh 渲染通道的高度
@param passName @en Pass name declared in the effect. Default value is 'default' @zh effect中的pass name，缺省为'default'
@returns Basic render pass builder
)");
        METHOD_COMMENT(addMultisampleRenderPass, R"(@en Add multisample render pass
@zh 添加多重采样渲染通道
@param width @en Width of the render pass @zh 渲染通道的宽度
@param height @en Height of the render pass @zh 渲染通道的高度
@param count @en Sample count @zh 采样数
@param quality @en Sample quality. Default value is 0 @zh 采样质量，默认值是0
@param passName @en Pass name declared in the effect. Default value is 'default' @zh effect中的pass name，缺省为'default'
@returns Multisample basic render pass builder
)");
        METHOD_COMMENT(addCopyPass, R"(@en Add copy pass.
The source and target resources:
Must be different resources(have different resource names).
Must have compatible formats.
Must have identical dimensions(width, height, depth), sample count and sample quality.
Can't be currently mapped.

Reinterpret copy is not supported.

@zh 添加拷贝通道，来源与目标必须满足：
是不同的注册资源。
资源格式兼容。
具有相同的尺寸、采样数、采样质量。
不能被Map。

暂不支持转义拷贝。

@param copyPairs @en Array of copy source and target @zh 拷贝来源与目标的数组
)");
        METHOD_COMMENT(getDescriptorSetLayout, R"(@engineInternal
)");
    } // BasicPipeline

    auto addStorageBuffer = [&](uint32_t vertID) {
        builder.addMethodComment(vertID, "addStorageBuffer", R"(@en Add storage buffer.
The buffer must have registered in pipeline.
@zh 添加存储缓冲，缓冲必须已注册。
@param name @en Name of the buffer @zh 缓冲的注册名
@param accessType @en Access type @zh 读写状态
@param slotName @en name of descriptor in the shader @zh 着色器中描述符的名字
)");
    };
    
    auto addStorageImage = [&](uint32_t vertID) {
        builder.addMethodComment(vertID, "addStorageImage", R"(@en Add storage texture.
The texture must have registered in pipeline.
@zh 添加存储贴图，贴图必须已注册。
@param name @en Name of the buffer @zh 贴图的注册名
@param accessType @en Access type @zh 读写状态
@param slotName @en name of descriptor in the shader @zh 着色器中描述符的名字
)");
    };

    COMMENT(RenderSubpassBuilder, R"(@beta Feature is under development
@en Render subpass
@zh 渲染次通道
)") {
        addRenderTarget(vertID, true);
        addDepthStencil(vertID, true);
        addTexture(vertID);
        addStorageBuffer(vertID);
        addStorageImage(vertID);
        setViewport(vertID);
        addQueue(vertID, false);
        showStatistics(vertID);
    }

    COMMENT(MultisampleRenderSubpassBuilder, R"(@beta Feature is under development
@en Multisample render subpass
@zh 多重采样渲染次通道
)") {
        METHOD_COMMENT(resolveRenderTarget, R"(@en Resolve render target
@zh 汇总渲染目标
@param source @en Multisample source @zh 多重采样来源
@param target @en Resolve target @zh 汇总目标
)");
        METHOD_COMMENT(resolveDepthStencil, R"(@en Resolve depth stencil
@zh 汇总深度模板缓冲
@param source @en Multisample source @zh 多重采样来源
@param target @en Resolve target @zh 汇总目标
@param depthMode @en Resolve mode of depth component @zh 深度分量汇总模式
@param stencilMode @en Resolve mode of stencil component @zh 模板分量汇总模式
)");
    }

    COMMENT(ComputeQueueBuilder, R"(@en Compute queue
@zh 计算队列
)") {
        METHOD_COMMENT(addDispatch, R"(@en Dispatch compute task
@zh 发送计算任务
@param threadGroupCountX @en Thread group count X  @zh 线程组的X分量的数目
@param threadGroupCountY @en Thread group count Y  @zh 线程组的Y分量的数目
@param threadGroupCountZ @en Thread group count Z  @zh 线程组的Z分量的数目
@param material @en The material to use @zh 计算任务用的材质
@param passID @en The name of the pass declared in the effect. @zh effect中的通道名字
)");
    }
    
    COMMENT(ComputeSubpassBuilder, R"(@beta Feature is under development
@en Compute subpass
@zh 计算次通道
)") {
        METHOD_COMMENT(addRenderTarget, R"(@en Add input render target.
@zh 添加输入渲染目标
@param name @en name of the render target @zh 渲染目标的名字
@param slotName @en name of the descriptor in shader @zh 着色器中描述符的名字
)");
        addTexture(vertID);
        addStorageBuffer(vertID);
        addStorageImage(vertID);
        addQueue(vertID, true);
    }

        
    COMMENT(RenderPassBuilder, R"(@beta Feature is under development
@en Render pass
@zh 渲染通道
)") {
        addStorageBuffer(vertID);
        addStorageImage(vertID);
        METHOD_COMMENT(addRenderSubpass, R"(@en Add render subpass.
@zh 添加渲染次通道
@param subpassName @en Subpass name declared in the effect @zh effect中的subpass name
@returns Render subpass builder
)");
        METHOD_COMMENT(addMultisampleRenderSubpass, R"(@en Add multisample render subpass.
Sample count and quality should match those of the resources.
@zh 添加多重采样渲染次通道，采样数与质量需要与资源一致。
@param count @en Sample count @zh 采样数
@param quality @en Sample quality @zh 采样质量
@param subpassName @en Subpass name declared in the effect @zh effect中的subpass name
@returns Multisample render subpass builder
)");
        METHOD_COMMENT(addComputeSubpass, R"(@en Add compute subpass.
@zh 添加计算次通道
@param subpassName @en Subpass name declared in the effect @zh effect中的subpass name
@returns Compute subpass builder
)");
    }

    COMMENT(ComputePassBuilder, R"(@en Compute pass
@zh 计算通道
)") {
        addTexture(vertID);
        addStorageBuffer(vertID);
        addStorageImage(vertID);
        addQueue(vertID, true);
    }
    
    COMMENT(SceneVisitor, R"(@deprecated @en Not used @zh 未使用
)");
    COMMENT(SceneTask, R"(@deprecated @en Not used @zh 未使用
)");
    COMMENT(SceneTransversal, R"(@deprecated @en Not used @zh 未使用
)");

    COMMENT(Pipeline, R"(@en Render pipeline.
@zh 渲染管线
)") {
         METHOD_COMMENT(addStorageBuffer, R"(@en Add storage buffer.
@zh 添加存储缓冲
@param name @en Resource name @zh 资源名字
@param format @en Format of the resource @zh 资源的格式
@param size @en Size of the resource @zh 资源的大小
@param residency @en Residency of the resource. @zh 资源的驻留性
)");
         METHOD_COMMENT(addStorageTexture, R"(@en Add 2D storage texture
@zh 添加2D存储贴图
@param name @en Resource name @zh 资源名字
@param format @en Format of the resource @zh 资源的格式
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param residency @en Residency of the resource. @zh 资源的驻留性
)");
         METHOD_COMMENT(addShadingRateTexture, R"(@en Add 2D shading rate texture
@zh 添加2D着色率贴图
@param name @en Resource name @zh 资源名字
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param residency @en Residency of the resource. @zh 资源的驻留性
)");
         METHOD_COMMENT(updateStorageBuffer, R"(@en Update storage buffer information.
@zh 更新存储缓冲的信息
@param name @en Resource name @zh 资源名字
@param size @en Size of the resource @zh 资源的大小
@param format @en Format of the resource @zh 资源的格式
)");
         METHOD_COMMENT(updateStorageTexture, R"(@en Update storage texture information.
@zh 更新2D存储贴图的信息
@param name @en Resource name @zh 资源名字
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
@param format @en Format of the resource @zh 资源的格式
)");
         METHOD_COMMENT(updateShadingRateTexture, R"(@en Update shading rate texture information.
@zh 更新2D着色率贴图的信息
@param name @en Resource name @zh 资源名字
@param width @en Width of the resource @zh 资源的宽度
@param height @en Height of the resource @zh 资源的高度
)");
        METHOD_COMMENT(addRenderPass, R"(@en Add render pass
@zh 添加渲染通道
@param width @en Width of the render pass @zh 渲染通道的宽度
@param height @en Height of the render pass @zh 渲染通道的高度
@param passName @en Pass name declared in the effect. Default value is 'default' @zh effect中的pass name，缺省为'default'
@returns Render pass builder
)");
        METHOD_COMMENT(addComputePass, R"(@en Add compute pass
@zh 添加计算通道
@param passName @en Pass name declared in the effect. Default value is 'default' @zh effect中的pass name，缺省为'default'
@returns Compute pass builder
)");
        METHOD_COMMENT(addUploadPass, R"(@en Add upload pass.
The source and target resources:
Must be different resources(have different resource names).
Must have compatible formats.
Must have identical dimensions(width, height, depth), sample count and sample quality.
Can't be currently mapped.

@zh 添加上传通道，来源与目标必须满足：
是不同的注册资源。
资源格式兼容。
具有相同的尺寸、采样数、采样质量。
不能被Map。

@param uploadPairs @en Array of upload source and target @zh 上传来源与目标的数组
)");
        METHOD_COMMENT(addMovePass, R"(@en Add move pass.
Move-construct target resource, by moving source resources into subresources of target.
After the move, the target resource must be completely initialized.
Target write conflicts will result in undefined behaviour.
The source and target resources:
Must be different resources(have different resource names).
Must have compatible formats.
Must have identical dimensions(width, height, depth), sample count and sample quality.
Can't be currently mapped.

@zh 添加移动通道。
移动构造目标资源，将来源移入目标的次级资源。
移动后，目标资源必须完全初始化。
目标写入冲突是未定义行为。
来源与目标必须满足：
是不同的注册资源。
资源格式兼容。
具有相同的尺寸、采样数、采样质量。
不能被Map。

@param movePairs @en Array of move source and target @zh 移动来源与目标的数组
)");
    }

    COMMENT(PipelineBuilder, R"(@en Pipeline builder.
User can implement this interface and setup render graph.
Call setCustomPipeline to register the pipeline builder
@zh 管线构造器
用户可以实现这个接口，来构建自己想要的render graph。
调用setCustomPipeline注册管线
)") {
        METHOD_COMMENT(setup, R"(@en Setup render graph
@zh 构建渲染管线
@param cameras @en Camera list to render @zh 需要渲染的相机列表
@param pipeline @en Current render pipeline @zh 当前管线
)");
    }

    COMMENT(RenderingModule, R"(@engineInternal
)") {
    }

    NAMESPACE_END(render);
    NAMESPACE_END(cc);
}

}
