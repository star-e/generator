#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

// clang-format off

namespace Cocos::Meta {
    
void buildRenderInterfaceDocument(ModuleBuilder& builder) {
    NAMESPACE_BEG(cc);
    NAMESPACE_BEG(render);

    COMMENT(PipelineRuntime, R"(@internal
@en PipelineRuntime is the runtime of both classical and custom pipelines.
It is used internally and should not be called directly.
@zh PipelineRuntime是经典管线以及自定义管线的运行时。
属于内部实现，用户不应直接调用。
)") {
        METHOD_COMMENT(activate, R"(@en Activate PipelineRuntime with default swapchain
@zh 用默认交换链初始化PipelineRuntime
)");
        METHOD_COMMENT(destroy, R"(@en Destroy resources of PipelineRuntime
@zh 销毁PipelineRuntime所持资源
)");
        METHOD_COMMENT(render, R"(@en Render contents of cameras
@zh 根据相机进行绘制
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
)");
        METHOD_COMMENT(getMacroInt, R"(@en Get macro as integer.
@zh 根据宏名获得整型
@param name @en Name of macro @zh 宏的名字
)");
        METHOD_COMMENT(getMacroBool, R"(@en Get macro as boolean.
@zh 根据宏名获得布尔值
@param name @en Name of macro @zh 宏的名字
)");
        METHOD_COMMENT(setMacroString, R"(@en Assign string value to macro.
@zh 给宏赋值字符串
@param name @en Name of macro @zh 宏的名字
)");
        METHOD_COMMENT(setMacroInt, R"(@en Assign integer value to macro.
@zh 给宏赋值整型
@param name @en Name of macro @zh 宏的名字
)");
        METHOD_COMMENT(setMacroBool, R"(@en Assign boolean value to macro.
@zh 给宏赋值布尔值
@param name @en Name of macro @zh 宏的名字
)");
        METHOD_COMMENT(onGlobalPipelineStateChanged, R"(@en trigger pipeline state change event
@zh 触发管线状态更新事件
)");
    } // PipelineRuntime
    
    NAMESPACE_END(render);
    NAMESPACE_END(cc);
}

}
