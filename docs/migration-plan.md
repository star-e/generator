# Cocos 渲染管线生成器迁移方案

建议在 `generator` 建立独立的 C++20 命令行工具。固定版本复用 `star-codegen` 的声明解析、模块图和类型图，在这个仓库实现面向 Cocos 的输出规划与后端。第一版以当前 `cocos4` 为兼容目标，保留引擎使用的文件名、C++17 接口、可执行 TypeScript、归档格式和原生 JS 绑定。第一个交付选 `ArchiveInterface`，最后接管 `NativePipeline`。

## 已验证的起点

参考版本为 `star-codegen@8e0ec15e`、`cocos-engine-generator@97a8215`、`cocos4@81f9296d98`。目标仓库 `generator@398dc61` 原有一个已跟踪文件 `.gitignore`。

在 Windows x64 用 MSVC v143 Debug 从旧生成器的固定提交重新构建，给它准备临时引擎目录和一份当前的 `native/CMakeLists.txt`。从能让 `../../../cocos4` 指向临时引擎的工作目录运行，得到 **11 个模块、52 个生成文件**。41 个原生 C++ 文件、9 个引擎 TS 文件、1 个编辑器 TS 文件、1 个 SWIG 配置，全部与当前引擎逐字节相同。处理后的 CMake 文件也逐字节相同。再次执行后，这 53 个文件的散列和修改时间均相同。[旧入口及输出列表](../../cocos-engine-generator/Cocos/CocosBuilder/CocosBuilder.cpp)

**手写文件和生成文件在同一目录。** 旧入口把 38 个手写路径加入 CMake 源列表，其中包括 `NativePipeline.cpp`、`NativeResourceGraph.cpp` 和 `BinaryArchive.h`。引擎里的 `NativePipeline.cpp` 调用生成的 `addVertex` 和 `get(ResourceGraph::DescTag{}, …)`。同名的旧生成器源码描述 Native 类型，生成目标是 `NativePipelineFwd.h`、`NativePipelineTypes.h/.cpp`、`NativePipelineGraphs.h`。迁移时用手写实现验收这四个文件。[旧类型描述](../../cocos-engine-generator/Cocos/CocosBuilder/NativePipeline.cpp) · [引擎实现](../../cocos4/native/cocos/renderer/pipeline/custom/NativePipeline.cpp)

## 复用到哪一层

在一个独立的临时 CMake 工程里，已通过 `add_subdirectory(... EXCLUDE_FROM_ALL)` 引入当前 Star 源码，只链接 `StarTypeGraphBuilder`。Windows x64 上从源代码编译并运行了这个小程序，它用 `parseDeclarations` 和 `buildTypeGraph` 从一条真实签名 `OutputArchive::writeNumber(double)` 构造出了模块图和类型图。建议在 `generator` 锁定完整 Git 提交，允许开发时指定本地源码目录，CI 使用同一提交。Star 当前安装入口提供 CLI，源码内的 CMake target 是这次实测的集成方式。[Star 构建目标](../../../star/star-codegen/Star/CMakeLists.txt) · [TypeGraph 构建接口](../../../star/star-codegen/docs/type-graph-api.md)

另一次试验把 `InputArchive` 和 `OutputArchive` 的八个方法写成 Star `.spec`，用外部模块描述 `std::string_view` 和 Boost PMR，并给两个 `scratch()` 加上 `@cppOnly`。Star CLI 0.2.0 生成 6 个文件，自身的 `--check` 通过。它按配置生成了 `archive.ts`，但 TS 方法名是 `writeBool/readBool` 等，另外还规划了 `ArchiveDependencies.h` 和项目 `CMakeLists.txt`。Cocos 的产物有 4 个，TS 调用方实际使用 `b/n/s`。这个差异决定了 Cocos 需要自己的文件规划和方法投影。引擎原生构建还明确要求 C++17，TS 配置为 `target: es6`。[Star 输出规则](../../../star/star-codegen/docs/project-config.md) · [Cocos 归档接口](../../cocos4/cocos/rendering/custom/archive.ts) · [引擎 C++ 配置](../../cocos4/native/cmake/predefine.cmake) · [引擎 TS 配置](../../cocos4/tsconfig.json)

| 层 | 建议实现 |
| --- | --- |
| 声明输入 | 每个输出模块一份 `.spec`。把 `CocosCommon.cpp`、C++ 默认类型和 TS 映射中的引擎类型登记为外部模块，指向真实头文件和 TS 导出。按实际符号补全直接导入，例如 `RenderCommon` 的旧 `mRequires` 只列 `Gfx`，`LightInfo` 还引用了 `scene::Light` 和 `scene::ReflectionProbe`。 |
| 语义模型 | 复用 Star 的名字解析和语义检查，从 AST 构建 `ModuleGraph` 和 `TypeGraph`。通用声明写在 `.spec`。首版建议将 Cocos 专用规则放在同模块的 `.cocos.json`，例如将 `cc::render::OutputArchive::writeBool(bool)` 的 TS 名映射为 `b`。按限定名和签名绑定到语义成员，缺少成员或重载不明确时，诊断指向该条规则。 |
| Cocos 生成计划 | 模块间的类型依赖来自 `.spec` 的导入。项目配置描述输出种类、相对路径和前缀。准备阶段确定引擎容器、图存储、TS 对象池、序列化字段顺序与绑定规则，列全路径并检查冲突，再将只读模型和计划交给后端。 |
| 后端 | C++ 输出兼容 C++17，沿用 `ccstd`、Boost PMR、`IntrusivePtr` 和引擎 `details/GraphTypes.h` 的图接口。TS 使用引擎的 `./graph`，生成 Web 所需的类、枚举、短方法名和对象池。C++ 与 TS 序列化共用一份字段计划。Cocos JSB 生成 `se::Value` 转换和 SWIG 输入，由引擎的 `cc_gen_swig_files` 生成 `jsb_render_auto.*`。 |
| 工作区和写入 | 后端先返回相对路径与内容。建议将 Cocos 计划中的文件路径适配到 Star 底层的 `checkGeneratedFiles` / `writeGeneratedFiles`，在第一次合并验证。整批规划和生成成功后才比较或写入。相同内容保留时间戳，过期文件报告路径，逐文件替换中断时可按清单重跑。[Star 写入契约](../../../star/star-codegen/docs/generation.md) |

旧宏中存在方法正文、引擎专用 include、SWIG 所有权规则和中英文接口注释。能表达为类型、签名、投影或字段策略的内容迁入声明模型。确需逐字保留的 C++/TS 片段和 SWIG 指令按模块存放，以类型或方法关联，并通过对应消费者编译或绑定测试。[旧接口及 SWIG 规则](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterface.cpp) · [接口文档来源](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterfaceDocument.cpp)

建议的仓库布局如下，路径均相对于 `generator`。

```text
CMakeLists.txt / CMakePresets.json / vcpkg.json
spec/extern/             引擎、标准库和 Boost 外部类型
spec/render/             管线模块声明与语言投影
config/                  workspace、模块输出配置、文件归属和版本基线
src/cli/                 命令入口
src/codegen/             Cocos 语义准备、输出规划及各后端
src/workspace/           加载配置、比较和写入
tests/                   小型契约测试、跨语言用例、引擎对照驱动
build/staging/cocos4/    临时产物，镜像引擎相对路径
```

## 模块迁移顺序

表中计数均为旧工具实际写出的生成文件数。C++ 默认目录是 `native/cocos/renderer/pipeline/custom/`，引擎 TS 默认目录是 `cocos/rendering/custom/`。模块名与文件前缀分别配置，尤其注意 `PrivateInterface → Private` 和 `PrivateTypes → Native`。

| 阶段 | 模块 | 文件数 | 优先验证的产物或行为 |
| --- | --- | ---: | --- |
| 1 | ArchiveInterface | 4 | `ArchiveFwd.h`、`ArchiveTypes.h`、`ArchiveTypes.cpp`、`archive.ts`。验证虚方法、原生 `scratch()` 与 TS 的 `b/n/s`。 |
| 2 | RenderSettings | 1 | 编辑器的 `editor/assets/default_renderpipeline/builtin-pipeline-types.ts`。用旧设置调用 `fillRequiredBloom`，验证 `material` 迁移到 `kawaseFilterMaterial`。 |
| 2 | RenderCommon | 10 | `RenderCommon*`、`types.ts`、`types-names.ts`。覆盖枚举数值、构造默认值、对象池复用、归档筛选和 `RenderCommonJsb` 转换。 |
| 3 | LayoutGraph | 9 | `LayoutGraph*`、`layout-graph.ts`、`layout-graph-names.ts`。覆盖父子引用、名称与路径索引、多态顶点及已有布局数据的读取。 |
| 3 | RenderGraph | 5 | `RenderGraph*`、`render-graph.ts`。分别验证 `ResourceGraph` 的别名引用关系、`RenderGraph` 的独立引用关系，以及 TS 可执行的组件访问和对象池。 |
| 4 | RenderInterface | 5 | `RenderInterface*`、`pipeline.ts`、`native/tools/swig-config/render.i`。覆盖重载、getter、条件宏、返回对象的 GC 规则和公开注释。 |
| 4 | PrivateInterface | 4 | `PrivateFwd.h`、`PrivateTypes.h/.cpp`、`private.ts`。使用 `RenderInterface` 和外部 `ProgramLib` 核对签名。 |
| 4 | Customization | 3 | `CustomFwd.h`、`CustomTypes.h/.cpp`。用引擎自定义渲染命令的派生类验证虚接口。 |
| 4 | FGDispatcher | 3 | `FGDispatcherTypes.h/.cpp`、`FGDispatcherGraphs.h`。用手写 `FrameGraphDispatcher.cpp` 和启用的图用例验证索引与遍历。 |
| 4 | PrivateTypes | 4 | `NativeFwd.h`、`NativeTypes.h/.cpp`、`web-types.ts`。此时 `LayoutGraph`、两个接口模块均已就绪。 |
| 5 | NativePipeline | 4 | `NativePipelineFwd.h`、`NativePipelineTypes.h/.cpp`、`NativePipelineGraphs.h`。编译并链接手写 `NativePipeline.cpp` 与执行器，运行一次最小原生管线。 |

`CocosCommon` 提供外部类型登记，`RenderInterfaceDocument` 提供注释。`RenderSceneInterface` 在旧入口参与登记，其输出调用已注释。`RenderCompiler`、`ExecutorExample`、`RenderExample` 的构建和输出受关闭的分支控制。这些状态纳入基线，已有的同名引擎文件按其实际来源登记。[模块入口](../../cocos-engine-generator/Cocos/CocosBuilder/CocosBuilder.cpp) · [旧外部类型登记](../../cocos-engine-generator/Cocos/CocosBuilder/CocosCommon.cpp)

## 第一次合并及后续验收

1. **把本次基线做成可以重跑的测试。** 在 `generator` 记录三个源提交、52 条引擎相对路径及 SHA-256，单列 CMake 源列表检查。Windows 对照任务固定检出和换行策略，在临时目录复建旧工具。日常测试读取固定引擎版本的文件，旧工具用于复核来源。
2. **交付一个能端到端工作的 Archive 模块。** 第一次合并包含 CMake 工具目标、所需外部类型、一个 Cocos 模块配置、两种语言的输出和只读检查。恰好生成表中的 4 个路径，与 Windows 基线逐字节对照。把这四个文件放进隔离的引擎副本，分别用真实的 `BinaryArchive.h` 和 `binary-archive.ts` 读写同一组布尔值、数值和 ASCII 字符串，互读对方写出的数据。[原生读写](../../cocos4/native/cocos/renderer/pipeline/custom/BinaryArchive.h) · [Web 读写](../../cocos4/cocos/rendering/custom/binary-archive.ts)
3. **尽早验证图模型的差异。** Archive 通过后，先用一个含父节点、具名组件和多态值的小图验证 Star 图语义能否表达 Cocos 的存储与调用约定，再批量翻译两个图模块。C++ 侧运行 `addVertex` 和 tag 访问，TS 侧运行 `locateChild`、`getParent`。用例覆盖边和父子关系共用存储、独立存储两种情形。发现的语义差异进入 Cocos 生成计划和回归用例。[旧 LayoutGraph 描述](../../cocos-engine-generator/Cocos/CocosBuilder/LayoutGraph.cpp) · [旧 RenderGraph 描述](../../cocos-engine-generator/Cocos/CocosBuilder/RenderGraph.cpp)
4. **每批都放回引擎验证。** 在隔离的 `cocos4` 副本中只覆盖本批已登记文件。Web 侧执行引擎的 `tsc --noEmit` 并运行图和对象池用例，编辑器资产由其消费端单独验证。原生侧使用引擎自己的 CMake 配置构建。绑定阶段执行现有 SWIG 步骤，编译生成的包装，并从 JS 调用一次渲染接口。布局归档用已有数据及跨 C++/TS 的样本验证字段顺序和枚举值。[Web 消费方](../../cocos4/cocos/rendering/custom/web-pipeline.ts) · [原生布局读取](../../cocos4/native/cocos/renderer/pipeline/custom/NativeFactory.cpp) · [引擎 SWIG 构建](../../cocos4/native/CMakeLists.txt)
5. **按模块切换生成入口。** 新工具默认写入 `generator/build/staging/cocos4/`。建议 CLI 提供 `generate --workspace … --check --diff` 比较 staging，再由单独的引擎对照任务比较登记路径。首版以路径和字节一致为目标。有意调整的文本差异逐文件记录原因，并通过同一组消费测试。每次接管记录模块、输入版本和产物清单。回退时恢复该模块上一批产物及生产者，旧生成器仍可在独立目录重放。

引擎文件归属按具体路径登记。同步时只接受本批标为 `generated` 的路径。`NativePipeline.cpp`、手写 Web 实现和编辑器 `.meta` 由引擎维护。`native/CMakeLists.txt` 的受管范围是 renderer 源列表，当前 41 个原生文件均已登记，首版检查列表成员即可。以后新增路径再提交对应的列表变更。[Star 文件归属](../../../star/star-codegen/docs/codegen-manifest.md)

全量切换以 **52 个文件对照通过、重复生成零改写、Web 与原生消费测试通过** 为准。Windows 负责与旧工具的逐字节对照，Linux 和 macOS 可使用同一份声明验证新工具的构建与生成。若后续引入自举，等 Cocos 的语义和输出稳定后，再按 Star 的 A/B 流程迁移生成器自身的数据模型。[Star 自举流程](../../../star/star-codegen/docs/self-hosting.md)
