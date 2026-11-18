# Cocos 渲染管线生成器迁移方案

在 `generator` 仓库实现独立的 C++20 命令行工具 `cocos-codegen`。仓库包含声明解析器、模块图、类型图、输出规划和各语言后端，单独检出即可用 CMake 和声明的第三方依赖构建。第一版以当前 `cocos4` 的行为为兼容目标，输出 C++17、可执行 TypeScript、现有归档格式和原生 JS 绑定。原生图操作迁入图类型的成员函数，相应调用方随模块切换。第一个交付选 `ArchiveInterface`，最后接管 `NativePipeline`。

## 命名约定

| 对象 | 约定与示例 |
| --- | --- |
| 命令行与产物来源 | 可执行文件、帮助文本和清单中的生成器名称统一为 `cocos-codegen`。 |
| 构建 | CMake 可执行目标为 `CocosCodegen`，输出文件名设为 `cocos-codegen`。项目专用构建选项使用 `COCOS_CODEGEN_` 前缀。 |
| C++ 接口 | 生成器代码放在 `cc::codegen`，例如 `cc::codegen::TypeGraph`。引擎类型沿用所属命名空间，例如 `cc::render::OutputArchive`。 |
| 新增 C++ 文件 | 文件名用 `Cocos` 加职责名，省去单字母 `S` 前缀，例如 `CocosTypeGraph.h`、`CocosSpecParser.cpp`、`CocosFileWriter.cpp`。 |
| 自定义配置与声明 | 使用小写 `cocos-` 前缀，例如 `cocos-workspace.json`、`cocos-archive.spec`。CMake 和包管理器的标准入口使用 `CMakeLists.txt`、`CMakePresets.json`、`vcpkg.json`。 |

这些约定用于新工具。引擎的模块文件保留既有命名，例如 C++ 调用方包含 `ArchiveTypes.h`，TS 调用方导入 `./archive`。原生图模块以各自的 `*Types.h` 作为操作入口。

## 已验证的起点

兼容性基线取自旧生成器 `cocos-engine-generator@97a8215` 和引擎 `cocos4@81f9296d98`。

在 Windows x64 用 MSVC v143 Debug 从旧生成器的固定提交重新构建，给它准备临时引擎目录和一份当前的 `native/CMakeLists.txt`。从能让 `../../../cocos4` 指向临时引擎的工作目录运行，得到 **11 个模块、52 个生成文件**。41 个原生 C++ 文件、9 个引擎 TS 文件、1 个编辑器 TS 文件、1 个 SWIG 配置，全部与当前引擎逐字节相同。处理后的 CMake 文件也逐字节相同。再次执行后，这 53 个文件的散列和修改时间均相同。[旧入口及输出列表](../../cocos-engine-generator/Cocos/CocosBuilder/CocosBuilder.cpp)

**手写文件和生成文件在同一目录。** 旧入口把 38 个手写路径加入 CMake 源列表，其中包括 `NativePipeline.cpp`、`NativeResourceGraph.cpp` 和 `BinaryArchive.h`。引擎里的 `NativePipeline.cpp` 目前包含 `RenderGraphGraphs.h` 并调用自由函数 `addVertex` 和 `get(ResourceGraph::DescTag{}, resg, v)`。它在 `RenderGraph` 迁移时改用成员接口，在 `NativePipeline` 迁移时继续用作原生消费者。[旧类型描述](../../cocos-engine-generator/Cocos/CocosBuilder/NativePipeline.cpp) · [引擎实现](../../cocos4/native/cocos/renderer/pipeline/custom/NativePipeline.cpp)

## 生成流程

`cocos-codegen` 的构建、代码生成和 `--check` 均独立于 V8、Node.js 和 .NET。生成目标限定为引擎 C++17、Web 和编辑器所用的 TypeScript，以及 Cocos JSB 所需的 `se::Value` 转换和 `render.i`。引擎的 SWIG 步骤处理 `render.i`。Node.js/npm 用于 `cocos4` 侧的 Web 打包、TypeScript 检查和测试。[引擎绑定入口](../../cocos4/native/tools/swig-config/render.i) · [引擎构建与测试脚本](../../cocos4/package.json)

以 `ArchiveInterface` 为例，一份声明描述 `OutputArchive` 和 `InputArchive` 的八个 C++ 方法。类型图记录签名，语言投影把布尔值、数值、字符串的读写方法分别映射成 TS 接口上的 `b`、`n`、`s`，两个 `scratch()` 仅对原生代码可见。文件计划为这个模块列出三个原生文件和一个 TS 文件。后端分别按引擎的 C++17 和 `target: es6` 配置生成。[旧模块声明](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterface.cpp) · [原生接口](../../cocos4/native/cocos/renderer/pipeline/custom/ArchiveTypes.h) · [TS 接口](../../cocos4/cocos/rendering/custom/archive.ts) · [引擎 C++ 配置](../../cocos4/native/cmake/predefine.cmake) · [引擎 TS 配置](../../cocos4/tsconfig.json)

| 层 | 建议实现 |
| --- | --- |
| 声明输入 | 每个输出模块一份 `.spec`。把 `CocosCommon.cpp`、C++ 默认类型和 TS 映射中的引擎类型登记为外部模块，指向真实头文件和 TS 导出。按实际符号补全直接导入，例如 `RenderCommon` 的旧 `mRequires` 只列 `Gfx`，`LightInfo` 还引用了 `scene::Light` 和 `scene::ReflectionProbe`。 |
| 语义模型 | 解析器保留声明的源位置。从语法树构建模块图和类型图，解析导入、限定名与签名。模块声明写入 `cocos-archive.spec` 这样的文件，语言投影放在同名的 `cocos-archive.json` 中。规则绑定到具体成员和重载，诊断同时指出出错的规则和相关声明。 |
| Cocos 生成计划 | 模块间的类型依赖来自 `.spec` 的导入。项目配置描述输出种类、相对路径和前缀。准备阶段确定引擎容器、图存储、TS 对象池、序列化字段顺序与绑定规则，列全路径并检查冲突，再将只读模型和计划交给后端。 |
| 后端 | C++ 输出兼容 C++17，沿用 `ccstd`、Boost PMR 和 `IntrusivePtr`。图类型按声明的能力生成顶点、出边、组件和层级访问接口。TS 保留引擎 `./graph` 的导入路径，生成 Web 所需的类、枚举、短方法名和对象池。C++ 与 TS 序列化共用一份字段计划。Cocos JSB 生成 `se::Value` 转换和 SWIG 输入，由引擎的 `cc_gen_swig_files` 生成 `jsb_render_auto.*`。 |
| 工作区和写入 | 后端返回相对路径与内容。工作区先核对整批文件与计划一一对应，再执行比较或写入。`--check` 只读并报告缺失、差异和过期路径。写入时保留相同文件的时间戳，按文件暂存和替换，中断后可按同一清单重跑。 |

旧宏中存在方法正文、引擎专用 include、SWIG 所有权规则和中英文接口注释。能表达为类型、签名、投影或字段策略的内容迁入声明模型。确需逐字保留的 C++/TS 片段和 SWIG 指令按模块存放，以类型或方法关联，并通过对应消费者编译或绑定测试。[旧接口及 SWIG 规则](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterface.cpp) · [接口文档来源](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterfaceDocument.cpp)

建议的仓库布局如下，路径均相对于 `generator`。

```text
CMakeLists.txt / CMakePresets.json / vcpkg.json
spec/extern/             引擎、标准库和 Boost 外部类型
spec/render/             管线模块声明与语言投影
config/                  workspace、模块输出配置、文件归属和版本基线
src/cli/                 命令入口
src/parser/              声明解析与源位置诊断
src/model/               模块图、类型图和名字解析
src/codegen/             Cocos 语义准备、输出规划及各后端
src/workspace/           加载配置、比较和写入
tests/                   小型契约测试、跨语言用例、引擎对照驱动
build/staging/cocos4/    临时产物，镜像引擎相对路径
```

## JS 与 C++ 的共享缓冲区

对于同一脚本线程上的同步交换，建议沿用引擎 `Pass` 的做法。C++ 按字节数创建 `cc::ArrayBuffer`，按偏移访问其中的数据。JSB 在同一个 `ArrayBuffer` 上创建 `Float32Array` 等视图，JS 和 C++ 就能读写同一块内存。缓冲区由引擎创建和持有，生成器只需为有固定布局的类型从一份字段声明算出两端共用的字节偏移、对齐和记录长度。[缓冲区实现](../../cocos4/native/cocos/core/ArrayBuffer.cpp) · [Pass 的原生数据块](../../cocos4/native/cocos/scene/Pass.cpp) · [JSB 视图](../../cocos4/native/cocos/bindings/manual/jsb_scene_manual.cpp)

具体字段、容量以及是否跨调用复用视图，留到首个使用模块接入时确定。如果 C++ 需要跨调用访问，它用 `cc::ArrayBuffer::Ptr` 持有缓冲区，双方在使用期间保持缓冲区定长且不转移。验收时由 JS 写入一个字段，C++ 读出并改写，再由 JS 从原视图读回新值。

## 图接口与算法

**C++ 图操作生成在图类型上，取消每个模块的 `*Graphs.h`。** 当前引擎的 `RenderGraphTypes.h` 已有 `reserve` 等成员，`vertices(g)`、`out_edges(v, g)` 和按 tag 访问组件等操作仍在 `RenderGraphGraphs.h` 中以自由函数提供。迁移后分别写成 `g.vertices()`、`g.out_edges(v)` 和 `resg.get(ResourceGraph::DescTag{}, v)`。顶点与边的增删同样通过图对象调用。[现有图类型](../../cocos4/native/cocos/renderer/pipeline/custom/RenderGraphTypes.h) · [现有图操作](../../cocos4/native/cocos/renderer/pipeline/custom/RenderGraphGraphs.h)

成员声明、模板定义和短小的内联实现放在 `*Types.h`，较大的非模板实现放在配套的 `*Types.cpp`。通用遍历仍是手写算法，例如 `depthFirstSearch(g, …)` 调用图的成员接口。旧工具为 `LayoutGraph`、`RenderGraph`、`FGDispatcher` 和 `NativePipeline` 各生成一个 `*Graphs.h`，其中 `NativePipelineGraphs.h` 只有包含指令和头文件保护。这四个旧路径按模块退役。[旧文件规划](../../cocos-engine-generator/Cocos/AST/BuilderImpl.cpp) · [旧 NativePipeline 图头](../../cocos4/native/cocos/renderer/pipeline/custom/NativePipelineGraphs.h)

图算法由 Cocos 独立实现。生成器和后续迁移的图运行时从构建依赖中排除 `boost-graph`，算法直接使用 Cocos 的顶点、边和访问接口。现有 `boost::container::pmr` 分配器属于引擎的容器契约，按原生签名保留。旧生成器的 `CppBuilder.cpp` 在输出类型声明和定义时都调用了 `boost::depth_first_visit`，这两处遍历随生成器一起替换。[旧声明输出](../../cocos-engine-generator/Cocos/AST/CppBuilder.cpp) · [旧依赖清单](../../cocos-engine-generator/vcpkg.json)

| 职责 | 实现与可验证的行为 |
| --- | --- |
| 图与视图 | 图拥有顶点、邻接关系和组件。单源遍历只要求枚举顶点的出边、从边取得终点，全图遍历额外要求枚举顶点。引用视图枚举父子边，过滤视图只呈现入选顶点和边，两者都保留原图描述符。C++17 生成图可直接满足这些接口。 |
| 深度优先遍历 | 在 `CocosGraphDFS.h` 实现显式栈 DFS，调用方提供访问者、颜色表和临时存储。`depthFirstVisit` 从给定顶点出发并沿用颜色，`depthFirstSearch` 初始化颜色，按枚举顺序从剩余未访问顶点继续搜索。按出边顺序访问，遇到灰色终点时报告回边，子顶点完成后再完成对应的树边。 |
| 依赖分析 | 模块边从导入方指向依赖方。按 DFS 的顶点完成顺序输出模块，例如 A 导入 B 时先输出 B，菱形依赖中的公共模块只输出一次。遇到回边时用当前访问路径和导入位置报告完整环。类型构建按需遍历继承和按值成员依赖，指针或引用允许指向自身。 |

原生运行时按消费者切换。`NativeExecutor.cpp` 在过滤视图上利用发现和完成事件执行渲染命令，`FrameGraphDispatcher.cpp` 在启用 pass 重排时计算传递闭包。阶段 3 迁移图遍历、引用视图和过滤视图时，同批适配执行器等手写调用方。阶段 4 接管 `FGDispatcher` 时，再用 Cocos 的可达性实现验证重排约束。Web 的 `graph.ts` 也将现有 DFS 标注为源自 Boost.Graph，迁移时独立实现遍历，并用同一组图核对 C++ 和 TS 的访问事件序列。[原生执行器](../../cocos4/native/cocos/renderer/pipeline/custom/NativeExecutor.cpp) · [帧图调度](../../cocos4/native/cocos/renderer/pipeline/custom/FrameGraphDispatcher.cpp) · [Web 图接口与遍历](../../cocos4/cocos/rendering/custom/graph.ts)

## 模块迁移顺序

旧版 52 个文件作为历史基线。按这 11 个模块的新版计划，退役四个图头后生成 **48 个文件**，其中原生 C++ 文件为 37 个。C++ 默认目录是 `native/cocos/renderer/pipeline/custom/`，引擎 TS 默认目录是 `cocos/rendering/custom/`。模块名与文件前缀分别配置，尤其注意 `PrivateInterface → Private` 和 `PrivateTypes → Native`。

| 阶段 | 模块 | 旧文件数 | 目标文件数 | 优先验证的产物或行为 |
| --- | --- | ---: | ---: | --- |
| 1 | ArchiveInterface | 4 | 4 | `ArchiveFwd.h`、`ArchiveTypes.h`、`ArchiveTypes.cpp`、`archive.ts`。验证虚方法、原生 `scratch()` 与 TS 的 `b/n/s`。 |
| 2 | RenderSettings | 1 | 1 | 编辑器的 `editor/assets/default_renderpipeline/builtin-pipeline-types.ts`。用旧设置调用 `fillRequiredBloom`，验证 `material` 迁移到 `kawaseFilterMaterial`。 |
| 2 | RenderCommon | 10 | 10 | `RenderCommon*`、`types.ts`、`types-names.ts`。覆盖枚举数值、构造默认值、对象池复用、归档筛选和 `RenderCommonJsb` 转换。 |
| 3 | LayoutGraph | 9 | 8 | `LayoutGraphFwd.h`、`LayoutGraphTypes.h/.cpp`、`LayoutGraphNames.h`、`LayoutGraphSerialization.h/.cpp`、`layout-graph.ts`、`layout-graph-names.ts`。验证父子引用、名称与路径索引、多态顶点及已有布局数据的读取。 |
| 3 | RenderGraph | 5 | 4 | `RenderGraphFwd.h`、`RenderGraphTypes.h/.cpp`、`render-graph.ts`。分别验证 `ResourceGraph` 的别名引用关系、`RenderGraph` 的独立引用关系，以及 TS 可执行的组件访问和对象池。 |
| 4 | RenderInterface | 5 | 5 | `RenderInterface*`、`pipeline.ts`、`native/tools/swig-config/render.i`。覆盖重载、getter、条件宏、返回对象的 GC 规则和公开注释。 |
| 4 | PrivateInterface | 4 | 4 | `PrivateFwd.h`、`PrivateTypes.h/.cpp`、`private.ts`。使用 `RenderInterface` 和外部 `ProgramLib` 核对签名。 |
| 4 | Customization | 3 | 3 | `CustomFwd.h`、`CustomTypes.h/.cpp`。用引擎自定义渲染命令的派生类验证虚接口。 |
| 4 | FGDispatcher | 3 | 2 | `FGDispatcherTypes.h/.cpp`。用手写 `FrameGraphDispatcher.cpp` 验证索引与遍历，显式启用 pass 重排并对照顶点间的可达关系。 |
| 4 | PrivateTypes | 4 | 4 | `NativeFwd.h`、`NativeTypes.h/.cpp`、`web-types.ts`。此时 `LayoutGraph`、两个接口模块均已就绪。 |
| 5 | NativePipeline | 4 | 3 | `NativePipelineFwd.h`、`NativePipelineTypes.h/.cpp`。编译并链接手写 `NativePipeline.cpp` 与执行器，运行一次最小原生管线。 |

`CocosCommon` 提供外部类型登记，`RenderInterfaceDocument` 提供注释。`RenderSceneInterface` 在旧入口参与登记，其输出调用已注释。`RenderCompiler`、`ExecutorExample`、`RenderExample` 的构建和输出受关闭的分支控制。这些状态纳入基线，已有的同名引擎文件按其实际来源登记。[模块入口](../../cocos-engine-generator/Cocos/CocosBuilder/CocosBuilder.cpp) · [旧外部类型登记](../../cocos-engine-generator/Cocos/CocosBuilder/CocosCommon.cpp)

## 第一次合并及后续验收

1. **把本次基线做成可以重跑的测试。** 在 `generator` 记录旧生成器和引擎的基线提交、52 条引擎相对路径及 SHA-256，单列 CMake 源列表检查。Windows 对照任务固定检出和换行策略，在临时目录复建旧工具。日常测试读取固定引擎版本的文件，旧工具用于复核来源。
2. **交付一个能端到端工作的 Archive 模块。** CI 先单独检出 `generator`，用仓库中的 CMake 配置构建 `cocos-codegen`。第一批实现只需支持这两个接口所用的声明、外部类型、语言投影、文件输出和只读检查。生成表中的 4 个路径，与 Windows 基线逐字节对照。把这四个文件放进隔离的引擎副本，分别用真实的 `BinaryArchive.h` 和 `binary-archive.ts` 读写同一组布尔值、数值和 ASCII 字符串，互读对方写出的数据。[原生读写](../../cocos4/native/cocos/renderer/pipeline/custom/BinaryArchive.h) · [Web 读写](../../cocos4/cocos/rendering/custom/binary-archive.ts)
3. **验证 Cocos 图算法与运行时。** 在生成器阶段用空图、孤立顶点、菱形依赖、有环图和长链验证 DFS 的覆盖范围、依赖顺序、环诊断和显式栈。C++17 消费用例只包含对应的 `*Types.h`，构造顶点、调用成员访问组件并遍历出边，链接该模块的 `*Types.cpp` 后运行。TS 侧验证 `locateChild`、`getParent`。用例覆盖边与父子关系共享和独立存储，以及过滤后访问的顶点和顺序。CI 检查生成器和原生用例的实际头文件依赖，含预编译头。检测到 `boost/graph/` 时检查失败，并报告引入路径。[旧 LayoutGraph 描述](../../cocos-engine-generator/Cocos/CocosBuilder/LayoutGraph.cpp) · [旧 RenderGraph 描述](../../cocos-engine-generator/Cocos/CocosBuilder/RenderGraph.cpp)
4. **每批都放回引擎验证。** 在隔离的 `cocos4` 副本中按清单放入本批产物。图模块同批更新手写调用方、原生测试以及生成代码的包含和调用，例如重新生成 `LayoutGraphSerialization.cpp`。移除该批退役的图头和对应 CMake 条目，在全新构建目录编译并链接。Web 侧执行引擎的 `tsc --noEmit` 并运行图和对象池用例，编辑器资产由其消费端单独验证。绑定阶段执行引擎的 SWIG 步骤，编译生成的包装，并从 JS 调用一次渲染接口。布局归档用已有数据及跨 C++/TS 的样本验证字段顺序和枚举值。[原生测试调用方](../../cocos4/native/cocos/renderer/pipeline/custom/test/test.h) · [布局序列化](../../cocos4/native/cocos/renderer/pipeline/custom/LayoutGraphSerialization.cpp) · [Web 消费方](../../cocos4/cocos/rendering/custom/web-pipeline.ts) · [原生布局读取](../../cocos4/native/cocos/renderer/pipeline/custom/NativeFactory.cpp) · [引擎构建](../../cocos4/native/CMakeLists.txt)
5. **按模块切换生成入口。** 新工具默认写入 `generator/build/staging/cocos4/`。建议从 `generator` 运行 `cocos-codegen generate --workspace config/cocos-workspace.json --check --diff` 比较暂存目录。引擎对照任务按批核对目标产物和已退役路径。仅替换生成器内部算法时，继续要求产物字节一致。切换引擎图接口时，逐文件审查成员化与适配代码带来的差异，以消费者编译、遍历事件和调度结果验收，再更新对应基线。每次接管记录模块、输入版本和产物清单。回退时恢复该批产物、旧头、CMake 登记及配套调用方。

引擎文件归属按具体路径登记。同步时只接受本批标为 `generated` 的路径。`NativePipeline.cpp`、手写 Web 实现和编辑器 `.meta` 由引擎维护。`native/CMakeLists.txt` 的受管范围是 renderer 源列表。旧基线登记了 41 个原生生成文件，四个图头按批移出后核对 37 个原生生成文件的登记。

全量切换以 **48 个目标文件对照通过、4 个旧图头退役、重复生成零改写、Web 与原生消费测试通过** 为准。Windows 负责历史基线与新版目标清单的对照，Linux 和 macOS 可使用同一份声明验证新工具的构建与生成。若后续引入自举，可用已发布的 `cocos-codegen` 生成自身的数据模型，以该模型编译候选版本，再用候选版本重新生成并比较两次结果。
