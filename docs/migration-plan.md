# Cocos 渲染管线生成器迁移方案

在 `generator` 仓库实现独立的 C++20 命令行工具 `cocos-codegen`。仓库包含声明解析器、模块图、类型图、输出规划和各语言后端，单独检出即可用 CMake 和声明的第三方依赖构建。第一版以当前 `cocos4` 的行为为兼容目标，输出 C++17、可执行 TypeScript、现有归档格式和原生 JS 绑定。原生图操作迁入图类型的成员函数，相应调用方随模块切换。引擎模块从 `ArchiveInterface` 开始，最后接管 `NativePipeline`。迁移完成还要求 `cocos-codegen` 能生成自身使用的模型源码，并用这些源码重新构建出能复现同一结果的工具。

## 命名约定

| 对象 | 约定与示例 |
| --- | --- |
| 命令行与产物来源 | 可执行文件、帮助文本和清单中的生成器名称统一为 `cocos-codegen`。 |
| 构建 | CMake 可执行目标为 `CocosCodegen`，输出文件名设为 `cocos-codegen`。项目专用构建选项使用 `COCOS_CODEGEN_` 前缀。 |
| C++ 接口 | 生成器代码放在 `cc::codegen`，例如 `cc::codegen::TypeGraph`。引擎类型沿用所属命名空间，例如 `cc::render::OutputArchive`。 |
| 新增 C++ 文件 | 文件名按职责使用 PascalCase，例如 `TypeGraph.cpp`、`SpecParser.h`、`Workspace.cpp`。 |
| 自定义配置与声明 | 文件名使用小写，多个单词用连字符分隔，例如 `workspace.json`、`archive.spec`。CMake 和包管理器的标准入口使用 `CMakeLists.txt`、`CMakePresets.json`、`vcpkg.json`。 |

这些约定用于新工具。引擎的模块文件保留既有命名，例如 C++ 调用方包含 `ArchiveTypes.h`，TS 调用方导入 `./archive`。原生图模块以各自的 `*Types.h` 作为操作入口。

## 已验证的起点

兼容性基线取自旧生成器 `cocos-engine-generator@97a8215` 和引擎 `cocos4@81f9296d98`。

在 Windows x64 用 MSVC v143 Debug 从旧生成器的固定提交重新构建，给它准备临时引擎目录和一份当前的 `native/CMakeLists.txt`。从能让 `../../../cocos4` 指向临时引擎的工作目录运行，得到 **11 个模块、52 个生成文件**。41 个原生 C++ 文件、9 个引擎 TS 文件、1 个编辑器 TS 文件、1 个 SWIG 配置，全部与当前引擎逐字节相同。处理后的 CMake 文件也逐字节相同。再次执行后，这 53 个文件的散列和修改时间均相同。[旧入口及输出列表](../../cocos-engine-generator/Cocos/CocosBuilder/CocosBuilder.cpp)

**手写文件和生成文件在同一目录。** 旧入口把 38 个手写路径加入 CMake 源列表，其中包括 `NativePipeline.cpp`、`NativeResourceGraph.cpp` 和 `BinaryArchive.h`。引擎里的 `NativePipeline.cpp` 目前包含 `RenderGraphGraphs.h` 并调用自由函数 `addVertex` 和 `get(ResourceGraph::DescTag{}, resg, v)`。它在 `RenderGraph` 迁移时改用成员接口，在 `NativePipeline` 迁移时继续用作原生消费者。[旧类型描述](../../cocos-engine-generator/Cocos/CocosBuilder/NativePipeline.cpp) · [引擎实现](../../cocos4/native/cocos/renderer/pipeline/custom/NativePipeline.cpp)

## 运行生成器

默认工作区暂存 60 个文件。归档接口占四个文件，编辑器设置占一个文件，`RenderCommon` 占十个文件。`LayoutGraph` 生成 `LayoutGraphFwd.h`、`LayoutGraphNames.h`、`LayoutGraphTypes.h/.cpp`、`LayoutGraphSerialization.h/.cpp`、`layout-graph.ts` 和 `layout-graph-names.ts`，并通过模块的 `runtimeFiles` 一同暂存手写的 `GraphDescriptor.h`、`GraphDFS.h`、`GraphIterator.h`、`GraphView.h`、`LayoutGraphPrint.h/.cpp`、`LayoutGraphDescriptorSet.h/.cpp` 和 `graph-dfs.ts`。各图共用的名称和布局组件标签 `Name_`、`Layout_` 定义在 `RenderCommonTypes.h`，对应的 `getName` 重载在 `RenderCommonNames.h`。`RenderGraph` 生成 `RenderGraphFwd.h`、`RenderGraphTypes.h/.cpp` 和 `render-graph.ts`，并暂存有向边遍历所用的 `GraphEdges.h`。在 `generator` 根目录运行以下命令。Windows 使用 x64 开发者命令行，示例选择 Ninja 单配置构建。

```sh
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
build/host/cocos-codegen generate --workspace config/workspace.json
build/host/cocos-codegen generate --workspace config/workspace.json --check --diff
ctest --test-dir build/host --output-on-failure
```

使用 clangd 时，在 `generator` 根目录运行 `cmake --preset clangd`。CMake 会在同一个 `build/host` 目录导出 `compile_commands.json`，供 clangd 读取项目的 C++20 编译选项和头文件路径。

默认暂存目录为 `build/staging/cocos4/`。可用 `--output DIR` 指定另一处专用目录。`--check` 只读，退出码 0 表示与计划相符，1 表示文件缺失、变化或多出，2 表示输入或执行出错。生成器使用固定版本的 JSON 库，CMake 可使用已安装的包或按 SHA-256 下载源码。[工作区](../config/workspace.json) · [模块声明](../spec/render/archive.spec)

### 按模块选择输出

从 `generator` 目录运行下面的命令，可在引擎检出中逐字节核对一个模块的产物。

```sh
build/host/cocos-codegen generate --workspace config/workspace.json --output ../cocos4 --check --module ArchiveInterface
```

`--module` 取 `.spec` 声明的模块名，区分大小写，只能与 `--check` 一起使用。生成器加载完整工作区以解析导入，按模块归属检查生成文件和 `runtimeFiles` 副本。目标目录可包含手写代码，也可省略工作区清单。匹配时显示文件数，缺失或变化时列出路径并返回 1，加 `--diff` 可查看第一处字节差异。引擎代码、自举代码、运行时副本和 JSON 清单默认使用运行生成器的平台换行，Windows 为 CRLF，其他平台为 LF。工作区可用 `lineEnding` 显式选择 `native`、`lf` 或 `crlf`。未知模块返回 2。

引擎和自举工作区都配置了 `"manifest": { "sourceRoot": ".." }`。生成器在输出根目录写入 `codegen-generated-manifest.json`。版本 1 的 `files` 以相对输出路径为键，每项的 `module` 和 `source` 指向声明该产物的模块和 `.spec`。通过 `runtimeFiles` 复制的文件另有 `copiedFrom`，例如 `cocos/rendering/custom/graph-dfs.ts` 来源于 `runtime/typescript/graph-dfs.ts`。`source` 和 `copiedFrom` 都相对于 `sourceRoot`，该目录相对于工作区配置文件解析，省略时取配置文件所在目录。清单按路径排序，与同一工作区的源码采用相同的换行规则，`--check --diff` 同样检查清单。自定义工作区可省略整个 `manifest` 配置以仅输出代码。[引擎工作区](../config/workspace.json) · [自举清单](../src/codegen-generated-manifest.json)

工作区通过 `ModuleInput` 把每个模块的声明、C++ 定义头、前置声明头和 TS 输出信息交给建图器。建图时，标量别名使用前置声明头，模板实例别名和图标签使用定义头。外部类型的头文件和 TS 导入来源沿用 `extern module` 的声明与配置。[建图输入](../src/model/TypeGraphBuilder.h) · [工作区](../src/workspace/Workspace.cpp)

TypeGraph 预置 `void`、`bool`、`char`、`int`、`float`、`double` 以及当前使用的五种固定宽度整数（`int32_t`、`uint8_t`、`uint16_t`、`uint32_t`、`uint64_t`）。所有模块都能直接使用这些标量，固定宽度整数的 C++ 头文件是 `<cstdint>`。引擎模块通过 `import EngineTypes;` 使用外部声明，例如 `Material`、`IntrusivePtr<T>` 和 `Array4<T>`。自举模块通过 `import HostTypes;` 使用 `std::filesystem::path`、`std::size_t` 和 `std::string`。迁移旧工作区时，把类型声明和头文件放入 `.spec` 的 `extern module`，由消费模块直接导入，然后删除工作区或模块上的 `types` 配置项。[引擎外部类型](../spec/extern/types.spec) · [自举外部类型](../spec/codegen/host-types.spec)

`modules` 中的每一项引用一个 `.spec`。`extern module` 的 `typescript` 字符串指定已有 TS 模块的导入路径，例如 `{ "spec": "../spec/extern/gfx.spec", "typescript": "cocos/gfx" }`。生成器只在消费方写入导入语句。没有统一 TS 来源的外部模块可以只填 `spec`。普通模块可以单独或同时配置 `cpp` 和 `typescript`。例如，`RenderCommon` 的生成配置为

```json
{
  "spec": "../spec/render/render-common.spec",
  "cpp": { "names": true, "serialization": true, "jsb": true },
  "typescript": "cocos/rendering/custom/types.ts"
}
```

`"cpp": {}` 生成 `<模块名>Fwd.h`、`<模块名>Types.h` 和 `<模块名>Types.cpp`。`names`、`serialization`、`jsb` 分别启用 `Names.h`、`Serialization.h`、`Jsb.h`，后两项也生成相应的 `.cpp`。每项都可以直接写文件名。`prefix` 覆盖命名前缀，`header` 覆盖主头文件名。文件写到 `<outputRoot>/<root>/<dir>/`，其中 `dir` 是头文件的 include 路径前缀。工作区的 `cppDefaults` 可统一设置 `root` 和 `dir`，单个模块可覆盖它们。每个 C++ 模块的 `Fwd.h` 自动包含 `.spec` 直接 `import` 的、也生成 C++ 的模块的前置头，路径取自依赖模块的输出配置。外部类型按声明的头文件映射按需包含。旧配置中显式指定的 `forwardIncludes` 仍作为补充，与自动包含的头文件去重。

外部模块的 `header "path/{name}.h" { ... }` 指定组内类型的定义头。普通外部类和结构体只作为裸指针或引用出现在字段或方法签名中时，C++ 后端按命名空间合并前置声明。需要完整定义的用法（例如基类、按值存储、模板实参或 `{}` 默认实参）优先使用定义头。模块内有手写 C++ 函数体时也保留可能使用的定义。外部别名可写成 `type MacroRecord;`，其声明由所属头文件提供。标准库类型使用标准头。[Gfx 分组](../spec/extern/gfx.spec) · [外部别名](../spec/extern/pipeline-externals.spec)

`cppDefaults.spaceBeforeReference` 默认是 `false`，沿用生成器原有的声明格式。cocos4 工作区将它设为 `true`，让接口和普通成员函数写成 `T *get()`、`void set(const T &value)` 和 `const std::vector<T *> &items`。记录的指针和引用字段也使用这个设置，例如 `const RenderGraph *renderGraph`。接口的拷贝、移动声明和 flags 赋值运算符也使用这个设置，例如 `T &operator=(T &&rhs)`。单个模块可以通过 `cpp.spaceBeforeReference` 覆盖工作区设置。

工作区可设置 `"typescriptDefaults": { "compactGraphNames": true }` 缩短 TS 图的私有存储名。默认关闭，cocos4 工作区已开启。出边和入边用 `o/i`，子节点和父节点用 `c/p`，载荷标签用 `t`，载荷数组用 `a`。短名与已声明成员重名时按顺序追加数字。组件数组沿用 `.spec` 的 `member` 名，字段和方法沿用公开名称。绑定方法中直接使用的私有存储名也会保留。

工作区顶层的 `transactionalGraphLoad` 控制 C++ 和 TS 的图反序列化，默认值为 `false`。默认模式直接用归档内容覆盖目标图，保留图上的 `@transient` 字段。读取失败后应丢弃目标图。设置 `"transactionalGraphLoad": true` 后，读取器先填充临时图，全部读取成功后再替换目标图。读取失败时，调用方仍可使用原图。两种模式使用相同的归档格式。

启用 `cpp` 的每个模块都生成与主头文件同目录、同名的 `.cpp`。头文件之外的定义写入这个源文件。模块只有声明时，源文件包含主头文件，例如 `ArchiveTypes.cpp` 包含 `ArchiveTypes.h`。

普通模块的 `typescript` 字符串指定要生成的 `.ts` 文件。编辑器设置使用 `{ "file": ".../builtin-pipeline-types.ts", "format": "settings" }`，可与同一模块的 C++ 输出组合。在 `.spec` 的模块声明上用 `@pool` 启用 TS 对象池。枚举注册和 JSB 导出类型写在绑定块里。`runtimeFiles` 按显式列出的源和目标复制配套文件。旧工作区的 `backend` 和扁平文件路径仍可读取。旧版 `forward` 的布尔值按默认规则处理，字符串指定的文件名继续有效。[工作区输出测试](../tests/WorkspaceOutputs.cmake)

例如，`@pool module RenderGraph;` 会将本模块中支持 `new T()` 的 TS 记录类和可修改的图加入 `RenderGraphObjectPool`。枚举、标签、接口、映射为数值的记录以及只在 C++ 中可见的声明均跳过。池成员名取类型名中每段连续大写字母的首字母，再转成小写，如 `LightInfo → li`、`DescriptorDB → dd`。全小写类型沿用原名。两个类型推导出同一成员名时生成器会报告两者，在 `binding typescript` 中为其中一个指定名字即可。例如 `ComputeView => class { pool cv1; };` 让 `ClearValue` 继续使用 `cv`。对象池默认从 `cocos/core/memop` 导入 `RecyclePool`，其他运行库路径可用 `poolModule` 指定。模块生成 TS 对象池时，按 `.spec` 的 `import` 顺序为直接依赖中也生成 TS 对象池的模块添加构造参数和只读属性。默认成员名取模块名的小驼峰形式，如 `RenderCommon → renderCommon`。遇到重名或需要自定义名称时可写 `poolDependency common: RenderCommon;`。显式参数按标注顺序排在前面，其余参数按导入顺序追加。

独立构建运行 `cocos.typegraph`、`cocos.modules.imports`、`cocos.archive.generate`、`cocos.settings.generate`、`cocos.records.generate` 和 `cocos.selfhost`。配置时设置 `COCOS_CODEGEN_ENGINE_ROOT` 为引擎目录，会额外注册历史基线、归档互读、编辑器设置和 RenderCommon 消费测试。互读测试使用引擎已安装的 TypeScript、Node.js 和 C++17 编译器，Boost.Container 可通过 `CMAKE_PREFIX_PATH` 指向其安装前缀。Windows 从构建目录运行 `cmake -DLEGACY=<旧仓库路径> -DENGINE=<引擎路径> -DDEPS=<x64-windows依赖前缀> -DMSBUILD=<MSBuild.exe路径> -P <generator路径>/tests/LegacyReplay.cmake`，可从固定提交重建 v143 工具并连续核对两次输出。[基线清单](../config/baseline.json) · [历史重建测试](../tests/LegacyReplay.cmake) · [引擎互读测试](../tests/Interop.cmake) · [编辑器消费测试](../tests/SettingsInterop.cmake)

## 剩余六个旧模块的生成范围

| 旧模块 | 当前可生成的内容 | 后续接管条件 |
| --- | --- | --- |
| RenderInterface | `pipeline.ts` 默认导出旧版全部 22 个类型，包含运行时、pass、subpass 和管线构建器。`PipelinePassBuilder` 的三个 Web 回调由 TS 绑定声明，`Factory` 在类型声明上标记为 `@cppOnly`。`render.i` 按 C++ 方法的 `@getter`、`@setter`、`@cppOnly` 生成属性和忽略项。`binding swig` 中的 `macro` 引用属性或方法，`releaseInGC` 指定由 JS GC 释放返回对象的方法。 | 公开 API 的中英文说明随声明输出到 C++ 和 TS，包括方法绑定成的 TS 属性。 |
| PrivateInterface | `PrivateFwd.h`、`PrivateTypes.h/.cpp` 和 `private.ts` 已接入 cocos4。`getProgramVariant` 和 `getComputePipelineState` 省略 `key` 时均转发 `nullptr`。Web 仅公开前者，返回值可为 `null`。 | 用 `--module PrivateInterface --output ../cocos4 --check --diff` 核对四个产物。原生实现为 `NativeProgramProxy` 和 `NativeProgramLibrary`。 |
| Customization | `CustomFwd.h`、`CustomTypes.h/.cpp` 已接入 cocos4。六组回调使用 `RenderGraphVertex`。该别名在 .spec 中引用新版 `RenderGraph::vertex_descriptor`，生成的前置声明头将其展开为 `uint32_t`，与旧图的 `vertex_descriptor` 一致。 | 用 `--module Customization --output ../cocos4 --check --diff` 核对三个产物。原生消费者包括 `NativePipeline.cpp` 的两个自定义渲染命令和 `NativeExecutor.cpp` 的回调调用点。 |
| PrivateTypes | `NativeFwd.h`、`NativeTypes.h/.cpp` 和 `web-types.ts` 已接入 cocos4。`ProgramInfo` 的字段构造要求传入五个参数，C++ 另有只接收分配器的重载。`ProgramGroup` 在 TS 字段声明处初始化两张 Map。 | 用 `--module PrivateTypes --output ../cocos4 --check --diff` 核对四个产物。原生消费测试按 `NativeProgramLibrary` 的插入方式验证分配器传递，Web 消费测试覆盖五参数构造和 Map 的实例隔离。 |
| FGDispatcher | `FGDispatcherFwd.h` 提供前置声明，`FGDispatcherTypes.h/.cpp` 覆盖 14 个辅助记录、两张双向图，以及 `FrameGraphDispatcher` 的声明和构造器。原生消费测试编译并运行了引擎手写的设置和查询方法。 | 适配 `run()` 调用的重排、别名和屏障算法，在启用 pass 重排时验证可达性，再切换引擎头文件。 |
| NativePipeline | `NativePipelineFwd.h` 和 `NativePipelineTypes.h/.cpp` 覆盖旧版全部 50 个顶层类型，包括 9 个具体构建器、原生管线、程序库、资源上下文和绘制队列。 | 引擎切换时，手写执行器和管线代码中引用旧图自由函数与嵌套标签的调用仍需适配。 |

`NativeSetter : NativeRenderNode implements Setter` 为实现类生成 `Setter` 的成员声明。`NativeRenderQueueBuilder final : RenderQueueBuilder, NativeSetter` 从公开接口推导 `override` 签名，并将已有实现转发到 `NativeSetter`。C++ 派生类的 `constructor(...)` 只列自己的字段。生成器按基类声明顺序收集必需的构造参数，放在本类参数之前，并在初始化列表中分别传给直接基类。派生类省略 `constructor` 时也会转发，允许零业务参数构造的基类则省略其字段参数。`constructor() noexcept = custom;` 为引擎手写的 `NativePipeline` 构造函数生成声明。[原生管线声明](../spec/render/native-pipeline.spec) · [消费测试](../tests/consumer/records/MigrationConsumer.cpp)

`struct` 和 `class` 可直接声明 `~T();` 或 `~T() noexcept;`。生成器将声明写入 C++ 头文件，定义由手写源文件提供。例如 `ResourceGroup` 在析构时调用各实例化缓冲区的 `clear()`。[模块声明](../spec/render/native-pipeline.spec) · [引擎实现](../../cocos4/native/cocos/renderer/pipeline/custom/NativeResourceGraph.cpp)

在 `extern module` 中写 `class RefCounted { public: virtual ~RefCounted(); };`，可告诉生成器该外部基类已有虚析构函数。直接派生的 `ProgramProxy` 因此生成 `~ProgramProxy() noexcept override = default;`。[外部基类声明](../spec/extern/pipeline-externals.spec)

`.spec` 的 `struct` 和 `class` 成员默认公开。`private:` 或 `public:` 作用于后续声明，直到下一个访问标签或类型结束，包含静态成员、构造和析构函数。字段保留声明顺序，生成的比较、哈希和归档函数可以访问其使用的私有字段。JSB 转换只读写公开字段，TypeScript 成员仍按原有绑定规则生成。[原生管线声明](../spec/render/native-pipeline.spec)

C++ 图可以在 `.spec` 中用 `component_index { component: PassID_; }` 为一个整数组件建立唯一索引。`add_vertex(key)` 和 `add_vertex(std::piecewise_construct, ...)` 遇到重复键会抛出 `std::invalid_argument`，并回滚本次插入。键通过只读访问器读取。`find(key)` 找不到时返回 `null_vertex()`，`vertex(key)` 则抛出 `std::out_of_range`。原生平面组件图可用 `mutable_graph { remove_vertex; }` 启用删点，组件须支持无异常移动赋值。删除顶点 `v` 会清除所有关联边，大于 `v` 的描述符减一，索引及剩余边同步更新。外部持有的描述符可用稳定键重新查找。[原生消费测试](../tests/consumer/records/DispatcherGraphConsumer.cpp)

`FGDispatcher` 和 `NativePipeline` 在暂存目录沿用引擎文件名。消费测试优先包含暂存头文件。切换引擎中的同名文件前，还需完成表中列出的引擎实现适配。三个接口模块继续输出引擎 CMake 已登记的 `*Types.cpp`，这些只包含头文件的翻译单元与旧版基线逐字节核对。声明见 [RenderInterface](../spec/render/render-interface.spec)、[PrivateInterface](../spec/render/private-interface.spec)、[Customization](../spec/render/customization.spec)、[PrivateTypes](../spec/render/private-types.spec)、[FGDispatcher](../spec/render/fg-dispatcher.spec) 和 [NativePipeline](../spec/render/native-pipeline.spec)。

## 生成流程

`cocos-codegen` 的构建、自举、代码生成和 `--check` 均独立于 V8、Node.js 和 .NET。引擎工作区生成 C++17、Web 和编辑器所用的 TypeScript，以及 Cocos JSB 的 `se::Value` 转换。`RenderInterface` 的 `cpp.swig` 指定相对于原生输出根目录的 `.i` 路径。引擎的 SWIG 步骤读取 `render.i` 和生成的 C++ 头，产生 `jsb_render_auto.h/.cpp`。Node.js/npm 用于 `cocos4` 侧的 Web 打包、TypeScript 检查和测试。生成器自身的工作区复用同一套 `.spec` 语法、解析器和 C++ 后端，产物编入 C++20 宿主工具，只使用 `generator` 声明的依赖。[引擎绑定入口](../../cocos4/native/tools/swig-config/render.i) · [引擎构建与测试脚本](../../cocos4/package.json)

以 `ArchiveInterface` 为例，一份声明描述 `OutputArchive` 和 `InputArchive` 的八个 C++ 方法。类型图记录签名，语言绑定把布尔值、数值、字符串的读写方法分别映射成 TS 接口上的 `b`、`n`、`s`，两个 `scratch()` 在接口声明中标注 `@cppOnly`，由 TypeGraph 记录其原生可见性。文件计划为这个模块列出三个原生文件和一个 TS 文件。后端分别按引擎的 C++17 和 `target: es6` 配置生成。[旧模块声明](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterface.cpp) · [原生接口](../../cocos4/native/cocos/renderer/pipeline/custom/ArchiveTypes.h) · [TS 接口](../../cocos4/cocos/rendering/custom/archive.ts) · [引擎 C++ 配置](../../cocos4/native/cmake/predefine.cmake) · [引擎 TS 配置](../../cocos4/tsconfig.json)

归档模块在同一份 `.spec` 中用 `@archive(output)` 标记写接口，用 `@archive(input)` 标记读接口，每个角色各有一个接口。生成 `@serialize` 记录或图的模块直接 `import` 这个模块。生成器从直接依赖中选择唯一提供这对角色的模块，通过 TypeGraph 中的类型引用生成 `save` 和 `load` 的参数，并按提供方在工作区中配置的输出路径生成 C++ 和 TS 导入。[归档角色声明](../spec/render/archive.spec) · [模块导入测试](../tests/ModuleImports.cmake)

### 在模块中声明方法

在声明及其标注之前用 `/** ... */` 编写公开文档。`//`、`///` 和 `/* ... */` 用于维护备注。文档可以是单行或多行，正文按原顺序保留段落和 `@en`、`@zh`、`@param` 等标签。C++ 方法和 TS 方法共用同一份说明。TS 将 getter 和 setter 绑定为一个属性时优先采用 getter 的文档，getter 缺少文档时采用 setter 的说明。每个显式重载可写自己的文档。

```cpp
// C++ 采用引擎的 getter 命名。
/** @zh 渲染管线使用的图形设备。 */
@getter virtual gfx::Device* getDevice() const = 0;
```

在接口的 C++ 风格声明上写 `@getter virtual ccstd::string getName() const = 0;`，TS 会生成只读属性 `readonly name: string`。同一接口再声明 `@setter virtual void setName(const ccstd::string& name) = 0;`，属性就可写。生成器按属性名配对，声明顺序不限。`getURLValue` 对应 `urlValue`。getter 须为无参数、返回值非 `void` 的 `const` 方法。setter 须为非 `const` 方法，返回 `void`，唯一参数与属性的 TS 类型一致。未标注的纯虚方法仍生成普通 TS 方法。已有的 `binding typescript` 属性规则也可继续使用。[管线接口](../spec/render/render-interface.spec) · [只读属性](../spec/render/private-interface.spec)

接口的纯虚方法在参数声明中写默认值。例如 `useLightFrustum` 的末尾两个参数写作 `uint32_t csmLevel = 0, const scene::Camera* optCamera = nullptr`。C++ 生成完整的纯虚签名，以及接收一个或两个参数的非虚转发重载。TS 生成 `useLightFrustum(light: Light, csmLevel?: number, optCamera?: Camera)`，运行时的默认值由 Web 实现提供。仅在 Web 侧可省略的参数仍可使用 `optional(...)`。普通 C++ 成员保留声明上的默认实参。支持数值、布尔值、空指针、字符串字面量、`{}`、枚举成员和同一 flags 类型的按位或组合。[公开接口](../spec/render/render-interface.spec) · [私有接口](../spec/render/private-interface.spec)

`interface` 中的 `virtual void writeBool(bool value) = 0;` 由 C++ 派生类实现，默认也生成 TS 接口方法。需要只在原生侧公开时写 `@cppOnly virtual ... = 0;`。普通成员采用 `ReturnType name(Type arg)`，支持 `static`、`const`、`noexcept`。以分号结尾会生成 C++ 声明，以花括号提供的 C++ 函数体会随头文件输出。普通成员在 `interface`、`struct`、`graph` 中均只生成 C++；字段和构造器继续使用各自的声明语法。[归档接口](../spec/render/archive.spec) · [原生转发重载](../spec/render/private-interface.spec)

C++ 方法可为尾部参数声明默认值，例如 `uint32_t lightByteOffset = 0xFFFFFFFF`。支持整数、十进制浮点数、布尔值、`nullptr` 和带类型名的枚举值。TypeGraph 检查整数范围和参数顺序，头文件保留默认实参。Web 端的可选参数继续由 TypeScript 绑定声明。[队列声明](../spec/render/native-pipeline.spec) · [原生消费测试](../tests/consumer/records/RenderQueueConsumer.cpp)

原生 ID 可以直接声明 `explicit operator uint32_t() const { return value; }`。TypeGraph 记录目标类型，C++ 调用方使用 `static_cast<uint32_t>(id)` 取得数值，TypeScript 绑定跳过该操作。三个 ID 均沿用引擎的 `0xFFFFFFFF` 无效值。[裁剪与渲染队列声明](../spec/render/native-pipeline.spec)

`RenderGraphFilter` 在 `.spec` 中直接定义 `bool operator()(RenderGraph::vertex_descriptor pass) const`，C++ 调用方用 `filter(pass)` 查询对应的 pass。调用运算符沿用成员函数的参数、重载和函数体规则，仅参与 C++ 生成。[过滤器声明](../spec/render/native-pipeline.spec)

原生记录可以声明 `T&` 或 `const T&` 字段，每个 `constructor(...)` 都必须列出这些字段。生成的构造器直接绑定调用方提供的对象。生成器也会省去无参构造器和只接收分配器的构造器。`@immovable` 删除记录的复制、移动及赋值操作。`FrameGraphDispatcher` 通过引用借用资源图、渲染图和布局图，以调用方的分配器原位构造 `resourceAccessGraph` 和 `relationGraph`。[调度器声明](../spec/render/fg-dispatcher.spec) · [消费测试](../tests/consumer/records/DispatcherGraphConsumer.cpp)

### 记录构造函数

普通记录的 C++ 和 TS 构造签名来自结构体中的同一条声明。

| 结构体里的写法 | C++ | TypeScript |
| --- | --- | --- |
| 省略构造声明 | 普通可复制记录保留聚合初始化。PMR、仅移动或不可移动的记录按需生成构造器。 | 在字段声明处初始化，通常使用隐式构造器。需要安装 `uint64_t` 写入校验时生成无参构造器。 |
| `constructor();` | 声明零字段参数的构造器。PMR 记录生成 `T(alloc)`。 | 生成显式的零参构造器。 |
| `constructor(a, b);` | 两个字段参数必填。 | 两个字段参数必填，即使字段本身有初值。 |
| `constructor(a, b) defaults;` | 每个参数都取字段初值或类型默认值，允许传入零到两个字段参数。无法确定某个默认值时，生成器报告字段名。 | 每个参数都取字段绑定、字段初值或类型默认值。数组、Map 和可无参构造的记录在每次调用时分别创建。无法提供默认值时，生成器报告字段名。 |

PMR 记录由字段推导，分配器由 C++ 后端加入签名。可移动的 PMR 记录和图生成 `T(T&&) noexcept`。支持移动赋值的类型也生成 `T& operator=(T&&) noexcept`。异常逃出这两个操作时会调用 `std::terminate`，包括跨资源移动赋值时的分配失败。显式接受分配器的构造函数允许异常传播给调用方，包括 `T(T&&, alloc)`。省略构造声明或只声明 `constructor();` 时，生成 `T(alloc)`，按字段初值或值初始化构造成员，并把资源传给需要分配器的成员。`constructor(a, b);` 要求传入 `a, b, alloc`。`constructor() defaults;` 生成 `T(alloc = boost::container::pmr::get_default_resource())`。[RasterSubpass 声明](../spec/render/render-graph.spec) · [消费测试](../tests/consumer/records/PassConsumer.cpp)

PMR 的 `constructor(a, b) defaults;` 生成两组构造。`T(alloc)` 按字段初值构造，调用方传入分配器。只有在 `.spec` 中另行声明 `constructor() defaults;` 时，分配器才有默认值。`T(std::allocator_arg, alloc, ...)` 要求显式传入分配器，随后可传零个、一个或两个字段参数。若省略的字段是必须传入分配器的嵌套记录，生成的重载会用本次调用的 `alloc` 就地构造该成员。例如，`CopyPair{std::allocator_arg, alloc, "src"}` 使用指定资源构造两个字符串，`target` 取空字符串。引擎的 `ccstd::pmr::vector<CopyPair>` 在 `emplace_back("src")` 时自动传入容器的分配器。[渲染公共类型](../spec/render/render-common.spec) · [C++ 消费测试](../tests/consumer/records/RecordConsumer.cpp)

一个记录可以声明多个 C++ 构造重载。若两个 PMR 字段构造都标了 `defaults`，调用 `T(std::allocator_arg, alloc)` 会有歧义，生成器报告两条冲突的声明。普通记录也会检查省略字段后产生的签名冲突。[生成检查](../tests/RecordTest.cmake)

给原生专用重载加 `@cppOnly`，TS 使用剩下的唯一可见声明。旧绑定仍可用 `constructor(...)` 选择重载。绑定的 `required` 和所选声明的 `defaults` 同时出现时，生成器报错。加入 TS 对象池的记录必须支持 `new T()`。[ProgramInfo 与 ProgramGroup](../spec/render/private-types.spec)

### 在模块中声明绑定

在 `module` 和 `import` 之后，把 `binding typescript` 或 `binding jsb` 写在命名空间外。一个文件可以包含多个绑定块。类型规则以分号结束。声明所属模块可以使用短名、限定名或从根命名空间开始的 `::` 名称。每个原生类型只能有一条 TS 绑定，同一类型的选项写在同一个规则内。

模块默认导出所有支持的 TS 类型，顺序沿用声明顺序。原生专用类型在声明上标记，例如 `@cppOnly class Factory`。已有的 `binding typescript { export { ... }; }` 仍可用来选择部分类型。[RenderInterface 声明](../spec/render/render-interface.spec)

在纯虚方法上，`@optionalMethod` 允许 Web 实现省略该方法，`@optionalResult` 和 `@nullable` 分别将指针返回值绑定为 `T | undefined` 和 `T | null`。仅在 Web 侧存在的接口成员写在 `binding typescript` 的接口规则中，例如 `callback setup?(...): BasicRenderPassBuilder | undefined;`。回调中的本地和导入类型通过 TypeGraph 解析，可在参数上使用 `?`，也可用 `Readonly` 和 `Record` 描述配置对象。[管线构建器](../spec/render/render-interface.spec) · [程序库接口](../spec/render/private-interface.spec)

```text
binding typescript {
    OutputArchive => interface {
        method writeBool => b;
    };
}
```

| 用途 | 模块内的实际规则 |
| --- | --- |
| 外部类型 | 工作区的 `"typescript": "cocos/gfx"` 为同名导出指定默认来源。`Camera => Camera from "cocos/render-scene/scene/camera";` 单独指定类型来源。路径相对于工作区输出根目录，后端按消费文件的位置生成相对导入。 |
| 具体模板实例 | `ccstd::vector<uint8_t> => Uint8Array;` 只匹配该模板和这组类型实参。`ccstd::vector<uint32_t>` 可以单独绑定到 `Uint32Array`。目标使用 ES2015 内置 TypedArray 名称，字段默认值为相应类型的零长度新数组。 |
| 普通数组 | 在外部类型声明上写 `@container("array") template<T> class vector;`。TypeGraph 递归解析元素的 TS 类型，字段使用 `T[]` 和独立的空数组。具体实例的显式绑定优先，例如 `ccstd::vector<ccstd::vector<uint8_t>>` 对应 `Uint8Array[]`。 |
| Map | 在外部类型声明上写 `@container("map") template<K, V> class map;`。TypeGraph 按键、值各自的绑定生成 `Map<K, V>`，从两侧递归收集导入。每个字段用 `new Map<K, V>()` 初始化。 |
| Set | 在外部类型声明上写 `@container("set") template<T> class PmrTransparentSet;`。TypeGraph 按元素绑定生成 `Set<T>` 并收集元素类型的导入。每个字段用 `new Set<T>()` 初始化，池化重置时调用 `.clear()` 原地清空。 |
| JSON 字符串键 | `DescriptorBlockIndex => class { mapKey json; };` 指定这个类型用作 TS Map 的键时表示为 `string`。引擎调用方用 `JSON.stringify(index)` 写入和查找。普通字段、参数和 Map 的值仍使用索引类。 |
| 枚举 | 枚举和 `flags` 默认生成 TS `const enum`。`QueueHint => enum;` 保留运行时枚举对象。`DescriptorTypeOrder => const enum { names; };` 为常量枚举生成名称查询。`BloomType => enum { register; };` 将运行时枚举注册到编辑器。 |
| 对象池和重置 | 模块中入池的记录按 TS 构造参数的顺序选取可写字段作为 `reset` 参数。没有可传入的字段时生成无参 `reset()`。原地重置的对象字段须在 `.spec` 中声明为 `readonly`。非空的内嵌外部对象调用 `this.field.reset()`，可空资源引用仍可作为参数传入。按值持有的 `Int8Array`、`Uint8Array`、`Int16Array`、`Uint16Array`、`Int32Array`、`Uint32Array` 在复用时自动保留原数组和内容，并从 `reset` 和池工厂的参数中省略。其他需要保留的字段可用 `keep(...)` 指定。 |
| 手写 TS 方法 | 在 `binding typescript` 的 `Type => class { ... };` 中直接写方法，例如 `getSet (frequency: UpdateFrequency): DescriptorSetData \| undefined { ... }`。生成器解析方法名、具名参数和返回类型，支持可选参数、泛型类型、数组和联合类型。方法体由 TypeScript 编译器检查。作用域外的依赖继续用绑定块的 `import` 或 `import type` 引入。已有的 `methods R"ts(...)ts";` 仍可读取。 |
| 编辑器设置 | `ForwardPassConfigs => interface;` 生成接口。`Bloom => interface { helpers; field kawaseFilterMaterial renamedFrom material; };` 生成工厂和补全函数并迁移旧字段。 |
| JSB 转换 | 在 `binding jsb` 内用 `record ResolvePair;` 选择类型，用 `include "cocos/renderer/pipeline/custom/details/JsbConversion.h";` 指定转换所需头文件。 |

TypeGraph 为已登记的全局标量提供默认 TS 绑定。`void` 映射到 `void`，`bool` 映射到 `boolean`，`double`、`float`、`uint8_t` 和 `uint32_t` 映射到 `number`。需要覆盖默认值时，在 `binding typescript` 中声明显式规则。

外部模块的默认 TS 来源取自工作区，路径从输出根目录算起。可以写目录入口 `cocos/gfx`，也可以写具体的 `.ts` 或 `.d.ts` 文件，消费方的导入省略该后缀。旧 `.spec` 中的 `module Gfx from "...";` 在工作区未指定路径时仍然有效。同一模块内的普通非模板类、结构体、枚举和标志枚举按声明名绑定，消费方用到才导入。`Type => extern enum DeviceType;` 沿用默认来源并改用导出名 `DeviceType`。逐类型的 `from` 可覆盖来源。归档字段使用外部对象时，生成器默认从消费方 `.ts` 文件同目录的 `./serialization` 导入 `save<导出名>` 和 `load<导出名>`。放在其他目录的函数可用 `UniformBlock archive from "cocos/codecs/uniform";` 覆盖路径。TypeScript 编译会检查所需函数是否已导出。[Gfx 声明](../spec/extern/gfx.spec)

在 `extern module` 的无定义 `class` 或 `struct` 上用 `@string` 声明拥有 UTF-8 文本的原生类型。字段默认绑定为 TS `string`，初值为 `''`，序列化使用字符串读写接口。显式 TS 绑定可以指定兼容的字符串别名及其导入路径。需要传递内存资源的字符串另加 `@pmr`。例如 `ccstd::pmr::string` 同时声明这两个注解。借用视图 `std::string_view` 在 `EngineTypes` 的 `header "string_view"` 块中声明，使用方直接导入该模块。它通过显式绑定映射为 TS `string`。[原生字符串声明](../spec/extern/strings.spec) · [借用视图声明与绑定](../spec/extern/types.spec)

外部模块的声明共用一个 C++ 头文件时，在模块的 `import` 之后写一次 `cpp { header "..."; }`。一个模块涉及多个头文件时，在 `namespace` 内用 `header "..." { ... }` 包住相关声明。固定路径供整组共用，路径中的 `{name}` 替换为声明的非限定名，保留大小写。例如 `GFX{name}.h` 将 `Buffer` 映射到 `GFXBuffer.h`。组内路径优先于模块默认值，组外声明继续使用模块默认值。消费方按模块名导入，TS 默认路径由工作区提供，逐类型例外写在 `binding typescript` 中。[Gfx 默认头文件与分组](../spec/extern/gfx.spec) · [Scene 命名模板](../spec/extern/scene.spec)

`DescriptorBlock` 的两个 Map 以字符串为键，值分别为本模块的 `Descriptor` 和外部的 `UniformBlock`。`DescriptorBlockFlattened` 用四个数组存放名称和值。这两个类型的池化 `reset()` 通过 `.clear()` 清空 Map、通过 `length = 0` 清空数组，保留已有引用并将 `capacity`、`count` 恢复为零。[容器声明](../spec/extern/containers.spec) · [布局声明](../spec/render/layout-graph.spec)

`DescriptorDB` 的 `blocks` 和 `groupBlocks` 在 C++ 中分别以 `DescriptorBlockIndex`、`DescriptorGroupBlockIndex` 为键，在 TS 中都绑定为 `Map<string, DescriptorBlock>`。其规格省略构造声明，C++ 调用方通过 `DescriptorDB(alloc)` 传入资源。两个 PMR map 都使用传入的资源，带 allocator 的复制和移动构造使用目标资源。[PMR 容器声明](../spec/extern/pmr-containers.spec) · [布局声明](../spec/render/layout-graph.spec)

`RenderPhase.shaders` 在 C++ 中使用引擎的 `PmrTransparentSet<ccstd::pmr::string>`。集合支持以 `std::string_view` 直接查找，带 allocator 的复制和移动构造使集合及其字符串使用目标资源。TS 将其绑定为 `Set<string>`。[集合声明](../spec/extern/sets.spec) · [布局声明](../spec/render/layout-graph.spec)

| 层 | 建议实现 |
| --- | --- |
| 声明输入 | 每个输出模块一份 `.spec`。把 `CocosCommon.cpp`、C++ 默认类型和 TS 映射中的引擎类型登记为外部模块，指向真实头文件和 TS 导出。按实际符号补全直接导入，例如 `RenderCommon` 的旧 `mRequires` 只列 `Gfx`，`LightInfo` 还引用了 `scene::Light` 和 `scene::ReflectionProbe`。 |
| 语义模型 | 解析器保留声明的源位置。从语法树构建模块图和类型图，解析导入、限定名与签名。模块的 .spec 文件同时承载原生声明和顶层语言绑定。生成器先登记声明、解析导入，再按限定名查找类型和方法。 |
| Cocos 生成计划 | 模块间的类型依赖来自 `.spec` 的导入。项目配置描述输出种类、相对路径和前缀。准备阶段确定引擎容器、图存储、TS 对象池、序列化字段顺序与绑定规则，列全路径并检查冲突，再将只读模型和计划交给后端。 |
| 后端 | 引擎的 C++ 输出兼容 C++17，沿用 `ccstd`、Boost PMR 和 `IntrusivePtr`。图类型按声明的能力生成顶点、出边、组件和层级访问接口。TS 保留引擎 `./graph` 的导入路径，生成 Web 所需的类、枚举、短方法名和对象池。C++ 与 TS 序列化共用一份字段计划。Cocos JSB 生成 `se::Value` 转换和 SWIG 输入，由引擎的 `cc_gen_swig_files` 生成 `jsb_render_auto.*`。 |
| 工作区和写入 | 后端返回相对路径与内容。工作区先核对整批文件与计划一一对应，再执行比较或写入。`--check` 只读并报告缺失、差异和过期路径。写入时保留相同文件的时间戳，按文件暂存和替换，中断后可按同一清单重跑。 |

旧宏中存在方法正文、引擎专用 include、SWIG 所有权规则和中英文接口注释。能表达为类型、签名、绑定或字段策略的内容迁入声明模型。确需逐字保留的 C++/TS 片段和 SWIG 指令按模块存放，以类型或方法关联，并通过对应消费者编译或绑定测试。[旧接口及 SWIG 规则](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterface.cpp) · [接口文档来源](../../cocos-engine-generator/Cocos/CocosBuilder/RenderInterfaceDocument.cpp)

建议的仓库布局如下，路径均相对于 `generator`。

```text
CMakeLists.txt / CMakePresets.json / vcpkg.json
spec/extern/             引擎、标准库和 Boost 外部类型
spec/render/             管线模块声明与语言绑定
spec/codegen/            生成器自身的模型声明
config/                  workspace、模块输出配置、文件归属和版本基线
runtime/typescript/      随模块暂存的手写 TS 运行时
runtime/cpp/             随模块暂存的手写 C++17 运行时
src/cli/                 命令入口
src/parser/              声明解析与源位置诊断
src/model/               建图、名字解析和语义检查
src/generated/           从自身声明生成并纳入版本控制的 C++ 模型源码
src/codegen/             Cocos 语义准备、输出规划及各后端
src/workspace/           加载配置、比较和写入
tests/                   小型契约测试、跨语言用例、引擎对照驱动
build/staging/cocos4/    临时产物，镜像引擎相对路径
```

## 生成器自举

日常构建编译 `src/generated/` 里的自举快照。运行 `cocos-codegen generate --workspace config/self-host.json` 会在 `build/self-host/generated/` 生成 `SourceLocation`、`TypeGraph`、`Syntax` 和 `Workspace` 四个模块的十二个 C++ 文件。解析器返回 `Syntax` 中的 `Module`，其中 `ModuleImport` 保存导入，`Declaration` 保存声明，`ModuleBindings` 保存 TypeScript、对象池、JSB 和 SWIG 的绑定。各声明通过 `TypeRef` 记录类型名、源码位置和嵌套模板实参。工作区将模块和选定的输出文件交给建图器，再通过 `RecordOutput` 和 `SettingsBinding` 向后端传递输出路径与绑定规则。[自举配置](../config/self-host.json) · [语法声明](../spec/codegen/syntax.spec) · [类型图声明](../spec/codegen/typegraph.spec) · [工作区声明](../spec/codegen/workspace.spec)

开始修改生成器自身的声明前，运行 `cmake --build build/host --target begin-codegen-update`。该目标构建当前生成器，验证其可执行文件副本，通过自举测试后将副本保存为本轮的 A，放在 `build/bootstrap/`。若构建或验证失败，上一次保存的 A 仍可用。

修改 `.spec` 后，运行 `cmake --build build/host --target generate-selfhost`。该目标用 A 刷新 `src/generated/` 和 `src/codegen-generated-manifest.json`，二者应一起提交。CMake 从这份清单读取模型的 C++ 源文件和头文件，所以首次检出可直接编译生成器。再用 `cmake --build build/host` 编译开发版 B，运行下文的自举测试。下一轮开始时，再用 `begin-codegen-update` 将 B 保存为新的 A。

外部模板声明支持末尾的类型参数包。比如 `template<First, Rest...> class variant;` 至少需要一个类型，`template<Ts...> class tuple;` 也接受空参数列表 `tuple<>`。通过 `using` 声明 C++ 模板实例的别名，生成器会收集所需头文件并先生成别名的依赖。`TemplateArgument` 和 `TypeData` 都遵循这条规则。[宿主类型](../spec/codegen/host-types.spec) · [类型别名](../spec/codegen/typegraph.spec)

`TypeGraph` 用 `vertex_storage: indexed(graph::BidirectionalStorage<uint32_t>);` 指定宿主的双向边存储。它适用于声明了顶点遍历、出边和入边的平面组件图。存储模板按顶点编号类型实例化，提供顶点与边的遍历、查询和 `reserve`。生成器为每个组件生成 `std::vector` 和访问方法，为图生成字段和方法声明。`appendNode` 的登记与回滚、按名称查找和类型复用由 `TypeGraph.cpp` 实现。[图存储](../src/model/GraphSupport.h) · [图操作](../src/model/TypeGraph.cpp)

1. 运行 `ctest --test-dir build/host -R cocos.selfhost --output-on-failure`。测试在空目录生成第一轮，逐文件检查路径和平台换行，再与 Git 中的快照核对内容。
2. 在新的构建目录将 `COCOS_CODEGEN_SELF_HOST_ROOT` 指向第一轮输出，用该目录中的模型编译候选生成器。
3. 候选生成器在第二个空目录再生成一次，逐字节比较两轮的自举文件，并核对引擎工作区的生成结果，然后运行当前配置启用的其余回归测试。[自举测试](../tests/SelfHost.cmake)

修改模型或生成规则时，将声明和刷新后的快照一同提交。若模型需要新语法，先用现有快照构建能解析该语法的工具，再用它生成下一轮快照。

## JS 与 C++ 的共享缓冲区

对于同一脚本线程上的同步交换，建议沿用引擎 `Pass` 的做法。C++ 按字节数创建 `cc::ArrayBuffer`，按偏移访问其中的数据。JSB 在同一个 `ArrayBuffer` 上创建 `Float32Array` 等视图，JS 和 C++ 就能读写同一块内存。缓冲区由引擎创建和持有，生成器只需为有固定布局的类型从一份字段声明算出两端共用的字节偏移、对齐和记录长度。[缓冲区实现](../../cocos4/native/cocos/core/ArrayBuffer.cpp) · [Pass 的原生数据块](../../cocos4/native/cocos/scene/Pass.cpp) · [JSB 视图](../../cocos4/native/cocos/bindings/manual/jsb_scene_manual.cpp)

具体字段、容量以及是否跨调用复用视图，留到首个使用模块接入时确定。如果 C++ 需要跨调用访问，它用 `cc::ArrayBuffer::Ptr` 持有缓冲区，双方在使用期间保持缓冲区定长且不转移。验收时由 JS 写入一个字段，C++ 读出并改写，再由 JS 从原视图读回新值。

## 图接口与算法

**C++ 图操作生成在图类型上，取消每个模块的 `*Graphs.h`。** 图按声明的能力提供成员函数。遍历和计数使用 `g.vertices()`、`g.out_edges(v)`、`g.num_vertices()`，增删使用 `g.add_vertex(…)`、`g.remove_vertex(v)`、`g.add_edge(u, v)`、`g.remove_edge(e)`，层级查询使用 `g.get_parent(v)`、`g.num_children(v)`。

生成的 C++ 图用 `Graph::edge_descriptor` 表示有向边，其共用实现为 `cc::graph::EdgeDescriptor<vertex_descriptor>`。`source` 和 `target` 分别保存起点和终点。顶点编号类型相同的图使用同一描述符类型，调用方应将边交回所属图。[边描述符](../runtime/cpp/GraphDescriptor.h)

C++ 图存储扩容后，已有的遍历区间和迭代器（包括 `end()`）失效。森林图的 `child_links(v)`、`out_edges(v)` 和 `adjacent_vertices(v)` 直接借用父子关系数组。若添加顶点导致扩容，调用方应重新取得区间再继续遍历。[迭代器实现](../runtime/cpp/GraphIterator.h)

C++ 图的组件列、多态载荷列、边和索引存储都是公开成员。图内普通字段和方法默认公开，可用 `private:`、`public:` 切换访问级别。组件标签在命名空间中用 `@tag struct` 声明。图内的 `component Node_: TypeNode { member: nodes; }` 引用已声明的标签，TypeGraph 按作用域和直接导入解析它。多个图可复用同一个标签类型。组件访问提供两种成员调用，例如自举模型中的 `g.get(Node_{}, v)` 和 `g.getNode(v)`。具名访问器采用 `get` 加组件名的形式。[自举模型声明](../spec/codegen/typegraph.spec) · [生成的成员接口](../src/generated/TypeGraphTypes.h)

多态顶点在 `polymorphic_graph` 块内按 `RenderStage_: RenderPassType { member: stages; }` 声明。C++ 顶点保存类别和在该类别数组中的索引。用 `g.add_vertex(RenderPhase_{}, "shadow", parent)` 创建相位，用 `g.kind(v)` 或 `g.holds<RenderPhase_>(v)` 查询类别，用 `g.value(RenderPhase_{}, v)` 读写载荷。类别不符或顶点越界时，`holds` 返回 false，`value` 抛出 `std::bad_variant_access`。创建多态顶点时必须显式传入标签。`g.add_vertex(RenderStage_{}, name, parent)` 创建阶段，载荷初值为 `SINGLE_RENDER_PASS`。[布局图声明](../spec/render/layout-graph.spec)

可变图用 `add_vertex` 的 `std::piecewise_construct` 重载原地构造。多态图先传标签和 `std::piecewise_construct`，再传载荷的构造参数元组，最后按 `.spec` 声明顺序传各组件的构造参数元组。非多态图从 `std::piecewise_construct` 起直接传各组件元组。层级图可在末尾传入父顶点，省略时创建根顶点。`std::tuple<>{}` 表示默认构造，`std::forward_as_tuple(std::move(value))` 可传入仅移动对象。PMR 组件和载荷在图的内存资源上构造。例如下面的调用创建一个透明队列，同时写入名称、布局和有效标记。[RenderGraph 消费测试](../tests/consumer/records/RenderGraphConsumer.cpp)

```cpp
auto queue = graph.add_vertex(Queue_{}, std::piecewise_construct,
    std::make_tuple(QueueHint::BLEND, phaseID, passLayoutID),
    std::forward_as_tuple("transparent"), std::forward_as_tuple("main"),
    std::tuple<>{}, std::make_tuple(true), pass);
```

TS 后端从同一声明生成 `LayoutGraphValue`、类别到载荷的类型映射和 `LayoutGraph`。编辑器通过 `addVertex(id, object, name, descriptors, parent)` 创建顶点，省略父节点时创建根。传入的相位与描述符对象保留原引用。`locateChild(parent, name)` 按插入顺序返回该父节点下第一个同名顶点，未找到时返回 `g.N`。消费测试直接编译并运行引擎编辑器的 `getPassID`、`getSubpassID` 和 `getPhaseID`。[TS 图后端](../src/codegen/TypeScriptGraphEmitter.cpp) · [编辑器建图用例](../tests/consumer/records/layout-graph-consumer.ts.in)

声明 `hierarchy_graph` 后，TS 的 `g.children(v)` 通过引擎的 `OutEI` 按插入顺序遍历直接子边，`g.child(e)` 和 `g.numChildren(v)` 分别返回子顶点和直接子节点数。`parent_graph` 另行提供按名字查找子节点的能力。多态图的 `j<T>(v)` 直接返回载荷，调用方负责先用 `w(v)` 确认类别。消费测试运行编辑器的阶段遍历循环和描述符构建方法，核对同名相位从各自的父阶段取得描述符。[编辑器阶段遍历](../../cocos4/cocos/rendering/custom/layout-graph-editor.ts) · [生成代码消费测试](../tests/RecordInterop.cmake)

成员声明、模板定义和短小的内联实现放在 `*Types.h`，较大的非模板实现放在配套的 `*Types.cpp`。通用遍历仍是手写算法，例如 `depthFirstSearch(g, …)` 调用图的成员接口。旧工具为 `LayoutGraph`、`RenderGraph`、`FGDispatcher` 和 `NativePipeline` 各生成一个 `*Graphs.h`，其中 `NativePipelineGraphs.h` 只有包含指令和头文件保护。这四个旧路径按模块退役。[旧文件规划](../../cocos-engine-generator/Cocos/AST/BuilderImpl.cpp) · [旧 NativePipeline 图头](../../cocos4/native/cocos/renderer/pipeline/custom/NativePipelineGraphs.h)

图算法由 Cocos 独立实现。生成器和后续迁移的图运行时从构建依赖中排除 `boost-graph`，算法直接使用 Cocos 的顶点、边和访问接口。现有 `boost::container::pmr` 分配器属于引擎的容器契约，按原生签名保留。旧生成器的 `CppBuilder.cpp` 在输出类型声明和定义时都调用了 `boost::depth_first_visit`，这两处遍历随生成器一起替换。[旧声明输出](../../cocos-engine-generator/Cocos/AST/CppBuilder.cpp) · [旧依赖清单](../../cocos-engine-generator/vcpkg.json)

| 职责 | 实现与可验证的行为 |
| --- | --- |
| 图与视图 | 图拥有顶点、邻接关系和组件。单源遍历只要求枚举顶点的出边、从边取得终点，全图遍历额外要求枚举顶点。引用视图枚举父子边，过滤视图只呈现入选顶点和边，两者都保留原图描述符。C++17 生成图可直接满足这些接口。 |
| 深度优先遍历 | 在 `GraphDFS.h` 实现显式栈 DFS，调用方提供访问者、颜色表和临时存储。`depthFirstVisit` 从给定顶点出发并沿用颜色，`depthFirstSearch` 初始化颜色，按枚举顺序从剩余未访问顶点继续搜索。按出边顺序访问，遇到灰色终点时报告回边，子顶点完成后再完成对应的树边。 |
| 依赖分析 | 模块边从导入方指向依赖方。按 DFS 的顶点完成顺序输出模块，例如 A 导入 B 时先输出 B，菱形依赖中的公共模块只输出一次。遇到回边时用当前访问路径和导入位置报告完整环。类型构建按需遍历继承和按值成员依赖，指针或引用允许指向自身。 |

C++ 图声明 `hierarchy_graph { storage: shared_edges; }` 后，`out_edges(v)` 和 `out_degree(v)` 直接复用子边迭代器和计数。用 `source(e)`、`target(e)` 读取端点，`edge(u, v)` 通过父节点在 O(1) 时间内返回 `std::pair<Edge, bool>`。未命中时两个端点均为 `null_vertex()`。

包含 `cocos/base/GraphDFS.h` 后可调用 `cc::graph::depthFirstSearch(g, visitor, colors, stack)`。颜色表支持按顶点下标访问，栈可用 `std::vector<cc::graph::DfsFrame<LayoutGraph>>` 或 PMR vector。`depthFirstVisit(g, root, visitor, colors, stack, terminate)` 沿用已有颜色，可选谓词在发现顶点后决定是否跳过其出边。互操作测试将原生图拓扑和每个回调写入样本，TS 用同一拓扑逐项比较，包括 25,000 层的长链。[C++ DFS](../runtime/cpp/GraphDFS.h) · [原生消费者](../tests/consumer/records/GraphDFSConsumer.cpp) · [TS 消费者](../tests/consumer/records/graph-dfs-consumer.ts)

在 C++ 中，`cc::graph::ReferenceGraphView refs{graph}` 让 DFS 沿父子引用遍历。再用 `cc::graph::FilteredGraphView visible{refs, keep}` 按顶点谓词筛选，两个视图都借用底层图。颜色表按原图的顶点编号分配。被过滤的起点不触发回调。若共享子节点还挂在另一个可见父节点下，从该父节点仍能访问它。TS 使用引擎的 `ReferenceGraphView`。互操作用例逐项对照两端的进入、退出及边事件。[原生视图](../runtime/cpp/GraphView.h) · [C++ 用例](../tests/consumer/records/GraphDFSConsumer.cpp) · [TS 用例](../tests/consumer/records/graph-dfs-consumer.ts)

原生运行时按消费者切换。`NativeExecutor.cpp` 在过滤视图上利用发现和完成事件执行渲染命令，`FrameGraphDispatcher.cpp` 在启用 pass 重排时计算传递闭包。阶段 3 迁移图遍历、引用视图和过滤视图时，同批适配执行器等手写调用方。阶段 4 接管 `FGDispatcher` 时，再用 Cocos 的可达性实现验证重排约束。[原生执行器](../../cocos4/native/cocos/renderer/pipeline/custom/NativeExecutor.cpp) · [帧图调度](../../cocos4/native/cocos/renderer/pipeline/custom/FrameGraphDispatcher.cpp)

TS 图声明 `hierarchy_graph { storage: shared_edges; }` 后，`oe`、`od`、`edge`、`source` 和 `target` 共用父子关系的存储。编辑器迁入时从 `./graph-dfs` 导入 `depthFirstVisit` 和 `depthFirstSearch`，访问器、颜色和边类型继续从 `./graph` 导入。互操作测试用编辑器的 `print()` 对照完整布局文本，并验证 DFS 事件顺序和长链。[TS 遍历源码](../runtime/typescript/graph-dfs.ts) · [Web 图接口](../../cocos4/cocos/rendering/custom/graph.ts)

对声明了 `polymorphic_graph` 的图，C++ 生成成员函数 `g.visit_object(v, visitor)`，也接受多个按载荷类型重载的 lambda。图按顶点类别调用一次处理函数，并保留其返回值或引用。可修改图传入 `T&`，只读图传入 `const T&`，两者都直接访问该顶点的载荷。无效编号会在调用访问器之前抛出 `std::out_of_range`。原生 DFS 用这一接口分派 Pass、Queue、Scene 的进入与退出操作。[渲染图访问用例](../tests/consumer/records/RenderGraphConsumer.cpp) · [DFS 用例](../tests/consumer/records/GraphDFSConsumer.cpp)

TS 多态图根据 `polymorphic_graph` 生成引擎的 `PolymorphicGraph` 接口。`h(kind, v)` 检查顶点类型，`object(v)` 返回载荷，`visitVertex(visitor, v)` 调用该标签的访问方法并返回其结果。声明 `mutable_graph` 的图还生成 `clear()`。编辑器重建 `LayoutGraphData` 时会从头分配顶点编号，原地清空 `valueNames` 和四个 Map，并将 `constantMacros` 还原为空字符串。消费测试运行引擎编辑器的阶段、相位、着色器构建方法以及 Web 程序库的相位筛选。[TS 消费测试](../tests/consumer/records/layout-graph-consumer.ts.in)

## 模块迁移顺序

### 在 RenderSettings 之前接入 TypeGraph

`cc::codegen::TypeGraph` 是 C++、TS 和 JSB 后端共用的语义类型图。`ModuleGraph` 负责模块导入和依赖顺序，`TypeGraph` 记录作用域、声明归属和解析后的类型关系。类型绑定按类型 ID 解析，方法重命名按具体成员绑定。

首份声明覆盖命名空间、模块归属、内建和外部类型、枚举、结构体、接口与图，也描述指针、引用、限定类型、函数和模板实例。当前 `.spec` 前端已接入接口方法、结构体字段、枚举、指针和类型模板实参，其他声明随引擎模块接入。`cocos.typegraph` 检查两个归档接口的 `scratch()` 复用同一个指向 `memory_resource` 的指针节点，并保留各自的诊断位置。Archive 的四个产物和 C++/TS 互读继续使用原有回归用例。[语义验证](../tests/TypeGraphTest.cpp) · [归档接口声明](../spec/render/archive.spec)

`RenderSettings` 的 `PipelineSettings.bloom` 和 `Bloom.type` 分别引用 `Bloom` 和 `BloomType`。设置后端按字段生成 TS 接口、默认值工厂和递归补全函数。旧版材质迁移规则在绑定中声明。`cocos.settings.editor` 编译引擎的 `BuiltinPipelineSettings` 并在隔离环境调用 `onEnable()`，检查新字段缺失、已有材质和显式 `null` 三种情况。[设置声明](../spec/render/render-settings.spec)

`RenderCommon` 的全部十三个枚举和 `LightInfo` 已生成到暂存目录的 `cocos/rendering/custom/types.ts` 和 `native/cocos/renderer/pipeline/custom/RenderCommonTypes.h`。`.spec` 支持十进制、十六进制整数初值、两个非负整数字面量的左移，以及对同一枚举中先前成员的引用。TypeGraph 保存初值表达式和求值结果，检查移位量与底层类型的取值范围。`ResourceFlags`、`SceneFlags` 和 `ResolveFlags` 共用 C++ 位运算符和 `any()` 的生成逻辑。`ResolveFlags` 按默认规则生成 TS 常量枚举。`ResourceFlags` 和 `SceneFlags` 显式绑定为 `enum`，供 JavaScript 在运行时访问。`names;` 单独控制名称查询。[模块声明](../spec/render/render-common.spec) · [模块绑定](../spec/render/render-common.spec)

`SceneFlags` 用 `/** @deprecated Please use OPAQUE. */` 在枚举项上说明弃用原因，和方法使用相同的文档语法。C++ 和 TS 都保留这条说明。TS 运行时保留旧别名的反向映射，例如 `SceneFlags[1]` 返回 `"OPAQUE_OBJECT"`。

`ResolveFlags` 的三个低位在 C++ 和 TS 中均保留 `1 << n`。例如 `DEPTH | STENCIL` 等于 `6`。对于 `uint32_t` 的 `1 << 31`，两端输出求值后的整数 `2147483648`，避免 TS 按有符号 32 位整数移位得到负数。TS 输出仍检查安全整数范围。

`LightInfo` 用一条 `constructor(light, level, culledByLight, probe) defaults;` 声明构造函数。C++ 和 TS 均可传入零到四个参数，省略的尾部参数使用字段默认值。外部场景类型由 `Light` 和 `ReflectionProbe` 模块提供，它们的 `.spec` 指定原生头文件和 TS 导入路径。

`ResolvePair` 的两个 PMR 字符串在构造、带分配器的复制和移动时都使用目标内存资源。后端将这些构造函数写入暂存目录的 `native/cocos/renderer/pipeline/custom/RenderCommonTypes.cpp`，并按声明上的 `@comparison("==")`生成五个字段的相等比较。TS 从引擎的 gfx 模块导入 `ResolveMode` 作为运行时值，供构造函数和 `createResolvePair()` 的默认参数使用。消费测试逐段对照引擎实现，验证两个 C++ 内存资源之间的复制与移动，以及 TS 对象池复用后五个字段的默认值。[Gfx 声明](../spec/extern/gfx.spec) · [消费测试](../tests/RecordInterop.cmake)

`CopyPair` 和 `MovePair` 共用上述 PMR 构造和 TS 对象池生成逻辑。两者的 `mipLevels` 和 `numSlices` 默认值均为 `0xFFFFFFFF`。TypeGraph 按 `uint8_t` 或 `uint32_t` 的范围校验十六进制字段初值，C++ 和 TS 保留声明中的写法。消费测试为各个 mip 和 slice 参数传入不同的值，并检查跨内存资源复制、移动以及对象池重置后的所有字段。[模块声明](../spec/render/render-common.spec) · [消费测试](../tests/RecordInterop.cmake)

`UploadPair` 在声明上标注 `@moveOnly`。TypeGraph 将其保存为 `RecordType.moveOnly`，C++ 后端据此生成移动操作并禁用复制。带分配器移动时，`source` 转移字节存储，`target` 使用传入的内存资源。TS 构造函数接收只读字段 `source` 的初值，默认为 `new Uint8Array(0)`。生成器根据 `source` 的 `Uint8Array` 类型推导出保留行为。`reset()` 和 `createUploadPair()` 接收其余六个字段，复用时保留原数组及内容。[模块声明](../spec/render/render-common.spec)

`cocos.records.interop` 逐个对照引擎的 C++ 和 TS 枚举声明，并逐字节对照 `types-names.ts`。C++17 消费者在编译时检查枚举底层类型和数值。TS 消费者编译后验证常量枚举成员的值、运行时枚举对象和更新频率名称查询。测试还使用引擎的 `RecyclePool` 验证 `createLightInfo()` 在复用时重置全部字段。[消费测试](../tests/RecordInterop.cmake)

`PipelineStatistics` 的 11 个计数器在声明中各自指定默认值。C++ 生成带字段初值的聚合类型。该类型没有构造参数，模块的 `@pool` 会为它生成无参 `reset()`。`RenderCommonObjectPool.createPipelineStatistics()` 每次从池中取出对象时都调用该方法。消费测试用引擎的 `RecyclePool` 核对对象复用和全部计数器，并将临时声明中的 `numRenderPasses` 改为 `7`，验证新建和复用的对象都采用声明值。[声明](../spec/render/render-common.spec)

### 引擎模块和产物

`LayoutGraph` 已接入 `LayoutType`、`DescriptorTypeOrder`、`RenderPassType` 和 `Descriptor`。三个枚举在 C++ 使用 `uint8_t`，在 TS 使用 `const enum`。`Descriptor` 的构造函数只接收 `gfx::Type`，`count` 的初值来自字段声明。模块的 `@pool` 据此推导出 TS 的 `reset(type)`，更新类型并把 `count` 恢复为声明值。TS 名称查询文件已完整对齐引擎。[模块声明](../spec/render/layout-graph.spec)

`DescriptorBlockIndex` 从 `RenderCommon` 引入 `UpdateFrequency` 和 `ParameterType`，从 `Gfx` 引入 `ShaderStageFlagBit`。生成模块之间的枚举引用使用提供方配置的头文件和 TS 输出路径。声明上的 `@comparison("<")` 让 C++ 后端按字段声明顺序生成 `operator<`。该键依次比较 `updateFrequency`、`parameterType`、`descriptorType`、`visibility`。`DescriptorGroupBlockIndex` 在这四项之后依次比较 `accessType`、`viewDimension`、`sampleType`、`format`。两个索引的 TS 构造函数以相同顺序写入字段，保留引擎用 `JSON.stringify(index)` 查表时的键字符串。[C++ 键测试](../tests/consumer/records/RecordConsumer.cpp) · [TS 键测试](../tests/consumer/records/consumer.cjs)

记录的比较、哈希和接口方法的原生可见性在 `.spec` 中声明。`@comparison` 可组合 `"=="`、`"<"`、`">"`，其中 `"=="` 同时生成 `!=`，`">"` 复用反向的 `<`。`@hash` 独立请求哈希。同一模块的绑定块声明 TS 名称、枚举形式、对象池、编辑器注册和 JSB 导出。工作区 JSON 选择 C++ 和 TS 产物及其输出路径。

C++ 索引图的每个 `component` 独占一个 vector，按顶点编号访问。组件列扩容会使该列已有的引用、指针和迭代器失效，调用方可以保留顶点编号并重新取组件。多态载荷按类型分列，记录类型的载荷使用 PMR deque。组件或载荷包含 `@moveOnly` 记录时，生成的图只支持移动。普通移动构造使用 `noexcept = default`，移动后源图只用于析构。带 allocator 的复制、移动和赋值使用目标资源。`LayoutGraph` 在图外声明 `Name_` 和 `Descriptors_`。调用 `add_vertex(RenderStage_{}, name, parent)` 时，每个新顶点都获得独立的空 `DescriptorDB`，其中两个 map 使用图的内存资源。`getDescriptors(v)` 与 `get(Descriptors_{}, v)` 返回同一对象的引用，并提供 const 重载。修改图时若抛出 `std::bad_alloc`，调用方直接销毁该图。[图声明](../spec/render/layout-graph.spec) · [生成实现](../src/codegen/GraphEmitter.cpp) · [消费测试](../tests/consumer/records/LayoutGraphConsumer.cpp)

`add_vertex` 将每组构造参数直接转发给对应组件列的 `emplace_back`。调用方应保证参数在整个调用期间有效。从同一张图取值且插入可能触发扩容时，先复制所需的值，再传入构造参数。

`LayoutGraph` 声明 `hierarchy_graph { storage: shared_edges; }`，父子关系存储在图的邻接表中。`add_vertex(RenderStage_{}, name)` 创建根节点，`add_vertex(RenderStage_{}, name, parent)` 创建子节点。`get_parent(v)` 和 `num_children(v)` 的时间复杂度为 O(1)，根节点的父节点为 `null_vertex()`。`child_links(v)` 按插入顺序遍历直接子边，`child(link)` 取得子节点。无效父节点会触发 `std::out_of_range`。[层级声明](../spec/render/layout-graph.spec) · [C++17 消费测试](../tests/consumer/records/LayoutGraphConsumer.cpp)

`named_graph { component: Name_; }` 将本图的字符串组件选作名称，`parent_graph;` 启用 C++ 成员函数 `locate_child(parent, name)`。传入普通父节点时查询其直接子节点，传入 `null_vertex()` 时按顶点插入顺序查找根。重名时返回第一个匹配项，未命中返回 `null_vertex()`。查询逐项比较组件中的当前值，通过 `getName(v)` 或 `get(Name_{}, v)` 改名后立即生效。[模块声明](../spec/render/layout-graph.spec) · [查找测试](../tests/consumer/records/LayoutGraphConsumer.cpp)

`addressable_graph;` 在 `parent_graph;` 上启用 `locate(refPath, start)`、`exists(refPath, start)` 和 `path(v)`。查找从指定顶点开始，起点默认为虚根 `null_vertex()`，前导 `/` 始终从虚根开始。`.` 保留当前位置，`..` 向上一级，连续和末尾的分隔符会被跳过。空字符串、缺失的路径段、越过虚根均返回 `null_vertex()`。`path(v)` 用图的内存资源返回绝对路径，传入虚根得到空字符串，其他越界编号会触发 `std::out_of_range`。路径按当前的节点名称计算，祖先改名对后代立即生效。[引擎路径规则](../../cocos4/cocos/rendering/custom/graph.ts) · [C++17 路径测试](../tests/consumer/records/LayoutGraphConsumer.cpp)

TS 用 `locate(absPath)` 从虚根查找，用 `locateRelative(path, start = this.N)` 指定起点，用 `path(v)` 生成绝对路径。起点可以是图中的顶点或虚根 `g.N`。路径段取自 `named_graph` 指定的名称组件。同一父节点下有重复名称时，查询返回最先插入的匹配项。查找失败返回 `g.N`，`path(g.N)` 返回空字符串。消费测试在 TS 中重建 C++ 用例的顶点和父子关系，逐项比较绝对路径和查询结果。[TS 寻址测试](../tests/consumer/records/layout-graph-consumer.ts.in)

在 `layout-graph.spec` 中，`@serialize graph LayoutGraph` 和相应的 `@serialize struct` 声明共同定义图归档。四个头部数值依次为顶点数、边数、阶段数和相位数。接下来按顶点编号写类别、父编号、名称、描述符库和载荷；根的父编号为 `0xFFFFFFFF`。描述符映射的键在 TS 内存中使用 JSON 字符串，在归档中按索引结构的字段顺序写入。`layout-graph.ts` 从同目录的 `./serialization` 导入 `UniformBlock` 和 `DescriptorSetLayoutInfo` 的读写函数。`InputArchive` 和 `OutputArchive` 的导入路径单独取自 `ArchiveInterface` 的工作区配置。读入整张图并核对类别和边的计数后，两端才替换目标图。[布局声明](../spec/render/layout-graph.spec) · [外部 Gfx 声明](../spec/extern/gfx.spec)

互操作测试使用引擎提交 `81f9296d` 的 TS 写入器生成的 628 字节固定样本，包含空图和一片有两个根的森林。C++ 和 TS 均读取该样本，并分别写出与其逐字节一致的数据。用例覆盖同名相位、两种描述符映射、含成员的 uniform block 和着色器集合。字符串沿用引擎 `BinaryArchive` 的单字节编码。[旧格式样本](../tests/consumer/records/fixtures/layout-graph.bin) · [跨版本测试](../tests/consumer/records/layout-archive.cjs)

`DescriptorBlockFlattened` 按声明顺序写入四个数组，再写 `capacity` 和 `count`。每个数组先写长度，再依次写元素。TS 加载时保留原数组引用，并按本次读到的长度覆盖或截短数组。互操作测试用旧引擎的 TS 写入器核对字节，并让 C++ 和 TS 依次读入空数组、两个元素、一个元素和空数组。[布局声明](../spec/render/layout-graph.spec) · [跨版本测试](../tests/consumer/records/layout-archive.cjs)

`UniformData` 在 C++ 中用 `using UniformID = uint32_t` 表示编号，在 TS 中沿用底层类型的 `number` 绑定。默认编号是 `0xFFFFFFFF`，归档依次写入编号、类型、偏移和大小。`UniformBlockData` 先写 `bufferSize`，再写 `uniforms`。C++ 加载到调用方提供内存资源的 PMR vector，TS 重用目标对象的 `uniforms` 数组。[布局声明](../spec/render/layout-graph.spec) · [原生互读测试](../tests/consumer/records/LayoutArchiveConsumer.cpp)

`NameLocalID => number(value)` 将 C++ 结构体的唯一数值字段绑定为 TS 的 `number`。用作字段、Map 键或 Map 值时均可直接传数值。默认值继承自 `value`。`DescriptorData` 在类绑定中用 `field descriptorID default 0;` 保留 TS 构造默认值，C++ 默认构造仍使用 `0xFFFFFFFF`。两端按编号、类型、数量的顺序归档。[布局声明](../spec/render/layout-graph.spec) · [跨版本测试](../tests/consumer/records/layout-archive.cjs)

`DescriptorBlockData` 在 C++ 中提供三个必填字段的末置分配器重载。七个字段的构造由 `defaults` 声明生成，使用前置分配器。该记录先写入类型、可见性、偏移、容量、访问方式、视图维度、采样类型和格式，再写 `descriptors` 数组。TS 的 `reset()` 更新构造参数、将 `offset` 归零，并原地清空数组。[布局声明](../spec/render/layout-graph.spec) · [互读测试](../tests/consumer/records/layout-archive.cjs)

`DescriptorSetLayoutData` 在 C++ 中只支持移动，构造时显式接收 PMR 分配器。TS 构造函数可以接收调用方的数组和两个 Map。`reset(slot, capacity)` 保留这三个容器的引用，清空内容，并将 `uniformBlockCapacity` 和 `samplerTextureCapacity` 归零。[布局声明](../spec/render/layout-graph.spec) · [容器声明](../spec/extern/maps.spec)

归档依次写入四个标量、描述符块数组、uniform block 映射和 binding 映射。空映射与单键映射可以逐字节核对，多键映射按键值核对。向已有对象加载时，数组按归档长度调整，两个 Map 都保留归档中缺失的键。遇到重复键，TS 用归档值覆盖，C++ 保留已有值。[原生互读测试](../tests/consumer/records/LayoutArchiveConsumer.cpp) · [TS 互读测试](../tests/consumer/records/layout-archive.cjs)

`DescriptorSetData` 到 `RenderPhaseData` 的九个记录都由同一份布局声明生成。C++ 记录只支持移动，并将传入的分配器传给嵌套记录和 PMR 容器。TS 重置只读的本模块记录时，调用其已有的 `reset`，按子记录的绑定顺序传入字段默认值。`DescriptorSetData` 据此调用 `descriptorSetLayoutData.reset(0xFFFFFFFF, 0)`。外部 GFX 对象 `descriptorSetLayoutInfo` 的成员函数 `reset()` 原地清空绑定数组。[布局声明](../spec/render/layout-graph.spec) · [GFX 绑定](../spec/extern/gfx.spec)

`Layout` 在共享 `.spec` 中声明 `static type: LayoutType = LayoutType::VULKAN;` 和 `static isWebGPU: bool = false;`，由 C++ 和 TS 各自生成类级状态。静态字段不计入构造参数、对象池重置、归档、比较和哈希。TS 的 `layout-graph.ts` 导出 `Layout` 和 `PipelineLayoutData`。`PipelineLayoutData.getSets()` 和 `getSet(frequency)` 在 `(COCOS_RUNTIME || HTML5) && Layout.isWebGPU` 时查询 `descriptorGroups`，其余情况查询 `descriptorSets`。两个方法通过同一份 `.spec` 中的 `methods` 块生成。[声明与方法实现](../spec/render/layout-graph.spec)

互读样本覆盖效果、技术、着色器、描述符集合以及渲染阶段和相位。每个 Map 使用一个键，便于逐字节核对 C++、TS 和旧引擎 TS 的输出。GPU 句柄声明为 `@transient`，原生测试使用引擎空后端创建有效句柄，并验证加载后的对象身份。[原生测试](../tests/consumer/records/PipelineArchiveConsumer.cpp) · [TS 测试](../tests/consumer/records/pipeline-archive.cjs)

`LayoutGraphData` 在同一个 `.spec` 中声明三个顶点组件、两种载荷和六个图级字段。归档先写顶点，再按声明顺序写 `valueNames`、`attributeIndex`、`constantIndex`、`shaderLayoutIndex` 和 `effects`。`@transient constantMacros` 保留目标图的运行时值。加载完整归档后才替换目标图，TS 保留只读数组和 Map 的对象身份。互读样本包含空图和带描述符、着色器及效果数据的父子图。测试编译旧引擎 C++ 读写器，逐字节核对新旧 C++ 和 TS 的输出。[图声明](../spec/render/layout-graph.spec) · [旧引擎读写器测试](../tests/consumer/records/LegacyArchiveConsumer.cpp)

编辑器转换测试把同一份 `LayoutGraph` 归档分别读入旧引擎和新生成的 TS 类型，调用引擎的 `buildLayoutGraphData()`，逐字节比较两份输出。C++ 再加载转换结果，核对顶点与父节点编号、着色器索引、描述符排序和偏移、预留容量，以及普通集合和描述符组中共用的 uniform 名称编号。[转换样本](../tests/consumer/records/layout-conversion.ts.in) · [原生读回](../tests/consumer/records/PipelineArchiveConsumer.cpp)

同一份转换结果也用于核对 TS 调用方。`printLayoutGraphData()` 与构建器的 `print()` 通过 `getName(v)` 读取顶点名，测试逐字比较两种图的打印文本。`createGfxDescriptorSetsAndPipelines()` 用 `v()` 遍历顶点，再用 `getLayout(v)` 获取布局。测试分别在普通集合和 WebGPU 描述符组上检查绑定编号、类型、数量、可见性和图像属性，并用测试设备核对布局句柄如何传给描述符集合。[调用方对照测试](../tests/consumer/records/layout-bindings.cjs)

原生 `printLayoutGraphData()` 只输出根顶点及其描述符集合。暂存版使用图的成员接口和命名空间标签。两个 C++17 消费程序分别链接引擎原有的打印函数和暂存实现，读取同一份归档并逐字节比较输出。带数组描述符、空预留块和子节点的样本还对照固定文本，检查标签、可见性、缩进与换行。[暂存实现](../runtime/cpp/LayoutGraphPrint.cpp) · [预期文本](../tests/consumer/records/fixtures/layout-print.txt)

原生 `initializeDescriptorSetLayoutInfo()` 向现有绑定列表追加条目。每个块从自己的 `offset` 开始，按描述符的 `count` 推进绑定编号，类型转换沿用引擎的 `getGfxDescriptorType()`。`getOrCreatePerPassDescriptorSet()` 用 `getLayout(vertex)` 查找普通集合的 `PER_PASS` 项，按需创建并缓存布局和集合。新旧消费程序读取同一份转换归档，对照绑定字段，再通过 `gfx::Device` 的创建入口检查缓存命中时的调用次数与对象身份。[暂存实现](../runtime/cpp/LayoutGraphDescriptorSet.cpp) · [原生对照测试](../tests/consumer/records/LayoutDescriptorConsumer.cpp)

`LayoutGraphObjectPool` 的 24 个工厂由模块的 `@pool` 生成。`import RenderCommon;` 会推导出构造参数和只读属性 `renderCommon: RenderCommonObjectPool`。`reset()` 只重置本地子池的取用位置。下次取出时，普通记录调用自己的 `reset(...)`，两个描述符索引用 `assign;` 按构造参数逐字段赋值，可修改的图调用 `clear()`，外部实现的空记录 `Layout` 直接复用。消费测试用同一套用例对照旧引擎和新生成的池。[布局绑定](../spec/render/layout-graph.spec) · [对象池测试](../tests/consumer/records/layout-pool.cjs)

PMR 字符串和 vector 直接用分配器构造。图插入用 `std::apply` 将参数元组转发给组件列和载荷列的 `emplace_back`，元组为空时直接无参插入。MSVC Debug 的字符串、vector 和 unordered_map 在某些 `noexcept` 构造中会分配迭代器代理，该次分配失败会终止进程。布局图消费测试在独立进程中逐笔注入分配失败。异常传播时，测试让图随栈展开而析构，再检查 PMR 资源上的分配是否全部释放。[分配失败测试](../tests/consumer/records/LayoutGraphConsumer.cpp)

`RenderCommon` 的十二个标签在 C++ 命名空间中定义。记录后端按声明顺序输出 `RenderCommonFwd.h`，并按记录声明上的 `@hash` 生成 `ccstd::hash<ResolvePair>`。哈希计算与相等比较都遍历记录的字段序列。测试核对两个共享标签的前置声明和名称查询、对 `ArchiveFwd.h` 的导入，以及这两个头文件其余内容与历史基线的一致性，C++17 消费测试使用不同分配器构造等值的键并查询同一容器条目。[声明](../spec/render/render-common.spec)

记录模块可以用 `cpp.names` 为 C++ 名称查询头选择默认名或指定相对于模块输出目录的文件名。`RenderCommonNames.h` 按声明顺序为普通枚举、标签和结构体生成 `cc::render::getName` 重载。相同枚举值返回首次声明的名称，未知值返回空字符串。标签返回去掉 `Tag` 或 `_` 后缀的名称，例如 `RasterPassTag` 对应 `"RasterPass"`。[工作区](../config/workspace.json) · [C++17 消费测试](../tests/consumer/records/RecordConsumer.cpp)

`LayoutGraphNames.h` 包含布局类型头，以及直接导入模块所生成的名称查询头。C++17 调用方只需包含它，就能查询布局枚举、`RenderStage_`、`RenderPhase_`、两种布局图以及来自 `RenderCommon` 的 `UpdateFrequency`。导入头的路径随依赖模块的 `cpp.names` 配置变化。[单头消费测试](../tests/consumer/records/LayoutNamesConsumer.cpp) · [导入路径测试](../tests/ModuleImports.cmake)

记录用 `@serialize` 参与归档，用 `@transient` 标记加载时应保留原值的字段。记录模块直接导入 `ArchiveInterface`，用 `cpp.serialization` 指定 C++ 头路径。C++ 的 `save/load` 和 TS 的 `save类型名/load类型名` 共用 TypeGraph 中的字段顺序。`RenderCommon` 包含五组归档函数，其中 `LightInfo` 只读写 `level` 和 `culledByLight`。消费测试用引擎的 `BinaryArchive` 在两种语言间双向读写五种记录并逐字节比较。[声明](../spec/render/render-common.spec) · [互读测试](../tests/RecordInterop.cmake)

JSB 绑定的 `record` 单独选择 `LightInfo`、`ResolvePair`、`CopyPair`、`UploadPair` 和 `MovePair`。`RenderCommonJsb.h/.cpp` 按声明顺序转换所选记录的所有字段，包括 `LightInfo.light` 和 `probe`。JS 写入 C++ 时，缺失、`null` 或 `undefined` 的属性保留 C++ 原值。独立的引擎消费测试还验证 `UploadPair.source` 从 `Uint8Array` 子视图读入字节并转回 JS。[模块绑定](../spec/render/render-common.spec) · [JSB 消费测试](../tests/consumer/jsb/JsbConsumer.cpp)

生成默认暂存文件后，可在原生引擎的构建环境中运行两个 JSB 消费测试。一个验证记录与 JS 值的转换，另一个用暂存的 `render.i` 和 C++ 头文件生成并编译 `jsb_render_auto.cpp`。后者通过 JS 读写 `RenderNode.name`，调用 `ComputePassBuilder.addQueue` 和 `ComputeQueueBuilder.addDispatch` 的各个重载，核对原生方法收到的实参及默认值。测试还分别在 JS 持有和释放队列后触发 GC，检查每个原生队列的析构次数。两个程序都编译并链接暂存的 `RenderCommonTypes.cpp` 和 `RenderCommonJsb.cpp`。`ENGINE` 和 `STAGING` 分别指向引擎和暂存根目录。

绑定测试中的 `render.Factory.createPipeline()` 返回测试用的 C++ `Pipeline`。JS 分别向 `addCopyPass`、`addResolvePass`、`addMovePass` 和 `addUploadPass` 传入空数组和两条记录。原生方法核对调用次数、记录顺序、字段默认值和 PMR 资源，并检查上传字节是否与 `Uint8Array` 子视图一致。`NativeRenderGraph.cpp` 和 `FrameGraphDispatcher.cpp` 都直接向 PMR vector 的 `emplace_back` 传字段。Windows Release 已分别用旧版和新生成的 `RenderCommon` 构建引擎并链接原生 `NativeFactory`，验证 `ResolvePair` 和 `CopyPair` 的字段及字符串使用的目标内存资源。[数组绑定测试](../tests/consumer/jsb/PassConsumer.cpp)

```sh
cmake -S tests/consumer/jsb -B build/jsb -DENGINE=<engine-root> -DSTAGING=<staging-root> -DCMAKE_BUILD_TYPE=Release
cmake --build build/jsb --config Release --target JsbConsumer RenderNodeConsumer
ctest --test-dir build/jsb -C Release --output-on-failure
```

引擎的 JSB 数组转换用 `vector::resize` 增加元素。`ResolvePair`、`CopyPair`、`UploadPair` 和 `MovePair` 在 `.spec` 中声明 `@cppOnly constructor() defaults;`，无参构造时使用 `boost::container::pmr::get_default_resource()`。带字段的构造声明继续生成分配器前置的 C++ 重载，以及带默认字段参数的 TS 构造。

2026-11-22 在 `cocos4`（`9e96ddbc1e`）的隔离副本中完成了 `RenderCommon` 整模块验证。副本仅替换本模块的 11 个产物，逐文件核对了与生成器（`0b4fe4a`）输出的 SHA-256。Windows Release 从源码构建引擎，归档用例链接原生 `NativeFactory`。数组用例通过引擎生成的 `jsb_render_auto.cpp` 调用四个 pass 入口，由测试用 `Pipeline` 核对实参。此构建沿用已有 SWIG 测试环境，其中跳过了缺少 `RenderSceneInterfaceTypes.h` 的 `render-scene.i`。

引擎的 `tsc --noEmit` 覆盖了 `web-pipeline.ts`、`render-graph.ts`、`layout-graph.ts` 及本模块的两个 TS 文件。Jest 使用引擎的 `RecyclePool` 验证六类记录重用同一对象并恢复默认字段。C++ 用 `BinaryOutputArchive` 写入 `LightInfo`、`ResolvePair`、`CopyPair`、`MovePair` 和 `PipelineStatistics`，TS 核对字段、修改值并回写，C++ 再核对新值和归档末尾。

旧版 52 个文件作为历史基线。按这 11 个模块的新版计划，退役四个图头后生成 **48 个文件**，其中原生 C++ 文件为 37 个。C++ 默认目录是 `native/cocos/renderer/pipeline/custom/`，引擎 TS 默认目录是 `cocos/rendering/custom/`。模块名与文件前缀分别配置，尤其注意 `PrivateInterface → Private` 和 `PrivateTypes → Native`。

| 阶段 | 模块 | 旧文件数 | 目标文件数 | 优先验证的产物或行为 |
| --- | --- | ---: | ---: | --- |
| 1 | ArchiveInterface | 4 | 4 | `ArchiveFwd.h`、`ArchiveTypes.h`、`ArchiveTypes.cpp`、`archive.ts`。验证虚方法、原生 `scratch()` 与 TS 的 `b/n/s`。 |
| 2 | RenderSettings | 1 | 1 | 编辑器的 `editor/assets/default_renderpipeline/builtin-pipeline-types.ts`。用旧设置调用 `fillRequiredBloom`，验证 `material` 迁移到 `kawaseFilterMaterial`。 |
| 2 | RenderCommon | 10 | 10 | `RenderCommon*`、`types.ts`、`types-names.ts`。覆盖枚举数值、构造默认值、对象池复用、归档筛选和 `RenderCommonJsb` 转换。 |
| 3 | LayoutGraph | 9 | 8 | 八个生成产物和七个配套运行时文件已接入 cocos4，`--module LayoutGraph --output ../cocos4 --check` 核对全部 15 个文件。原生调用方使用成员接口，布局编辑器使用迭代 DFS，`LayoutGraphGraphs.h` 已退役。 |
| 3 | RenderGraph | 5 | 4 | `RenderGraphFwd.h`、`RenderGraphTypes.h/.cpp`、`render-graph.ts`。分别验证 `ResourceGraph` 的别名引用关系、`RenderGraph` 的独立引用关系，以及 TS 可执行的组件访问和对象池。 |
| 4 | RenderInterface | 5 | 5 | `RenderInterface*`、`pipeline.ts`、`native/tools/swig-config/render.i`。覆盖重载、getter、条件宏、返回对象的 GC 规则和公开注释。 |
| 4 | PrivateInterface | 4 | 4 | `PrivateFwd.h`、`PrivateTypes.h/.cpp`、`private.ts`。使用 `RenderInterface` 和外部 `ProgramLib` 核对签名。 |
| 4 | Customization | 3 | 3 | `CustomFwd.h`、`CustomTypes.h/.cpp`。用引擎自定义渲染命令的派生类验证虚接口。 |
| 4 | FGDispatcher | 3 | 2 | `FGDispatcherTypes.h/.cpp`。用手写 `FrameGraphDispatcher.cpp` 验证索引与遍历，显式启用 pass 重排并对照顶点间的可达关系。 |
| 4 | PrivateTypes | 4 | 4 | `NativeFwd.h`、`NativeTypes.h/.cpp`、`web-types.ts`。`NativePipelineTypes.h` 按值使用 `LayoutGraphData`，直接包含 `LayoutGraphTypes.h`。 |
| 5 | NativePipeline | 4 | 3 | `NativePipelineFwd.h`、`NativePipelineTypes.h/.cpp`。编译并链接手写 `NativePipeline.cpp` 与执行器，运行一次最小原生管线。 |

当前旧生成器保留模块注册以解析类型依赖，只向 cocos4 写入 `RenderGraph`、`FGDispatcher` 和 `NativePipeline` 的 12 个文件。更新原生 CMake 清单时只补缺少的条目。其余八个模块由新生成器输出。`CocosCommon` 提供外部类型登记，`RenderInterfaceDocument` 提供注释，`RenderSceneInterface` 仅参与登记。`RenderCompiler`、`ExecutorExample`、`RenderExample` 的构建和输出受关闭的分支控制。[模块入口](../../cocos-engine-generator/Cocos/CocosBuilder/CocosBuilder.cpp) · [旧外部类型登记](../../cocos-engine-generator/Cocos/CocosBuilder/CocosCommon.cpp)

## 当前图接口覆盖

生成的 C++ 和 TS 图按引擎 `graph.ts` 的 concept 顺序组织公开操作，只输出有成员的分组。`source`、`target` 属于 `IncidenceGraph`，在 `.spec` 中声明 `edge_list_graph;` 才会生成 C++ 的 `edges()`、`num_edges()` 和 TS 的 `ne()`。省略它仍可按顶点遍历出边。`LayoutGraph` 和 `LayoutGraphData` 在归档时按顶点累加出度，将边数写入原有的归档头。普通边和独立父子引用的修改接口分别归入 `MutableGraph` 和 `MutableReferenceGraph`。类外定义沿用头文件的分组顺序。C++ 多态图按类型分列保存载荷，通过顶点的 `Kind` 和列内下标定位对象。

TS 索引图默认按列存储（SoA）。每个组件生成独立的私有数组，名称取自 `.spec` 的 `member`，例如 `member: names;` 生成 `_names`。组件读写、边的查询与修改、路径操作都由调用者保证顶点编号有效。父子关系和入出边按图声明分别存储。多态图用两个与顶点数等长的数组保存标签和载荷。同一个顶点编号 `v` 直接索引两列。`object(v)` 返回该顶点的载荷，`value(id, v)` 用调用者提供的标签确定返回类型，`visitVertex` 按存储的标签分派。归档读取器在添加顶点前校验标签和父编号。

对照旧清单，`LayoutGraph` 的九个文件对应八个生成产物。原 `LayoutGraphGraphs.h` 的图操作迁入类型成员。C++ 可遍历入边、出边、邻接顶点和全图边，TS 提供 `ie/id/d/adj/ne`、多态类别名称和 `setUpdate`。顶点和组件存储由图持有，调用方用 `object(v)` 或 `value(tag, v)` 读取载荷。旧 TS 写入器访问内部数组 `x[v].j`，跨版本测试分别运行对应写入器，并用旧读取器读回新图。[生成接口测试](../tests/consumer/records/LayoutGraphConsumer.cpp) · [归档对照](../tests/consumer/records/layout-archive.cjs)

`cocos.records.interop` 从引擎 Git 仓库导出 `config/baseline.json` 的 `engineCommit`，在测试临时目录编译旧版 C++ 和 TS 的布局图与归档实现。运行测试的引擎检出需要包含该提交。新版使用当前生成的图和序列化代码。测试让旧版 C++ 读入新版 TS 写出的 `LayoutGraph`，再写给新版 C++，并与已登记的二进制样本逐字节核对。`LayoutGraphData` 也覆盖新旧 C++ 和 TS 的交叉读写。[基线提交](../config/baseline.json) · [互读测试](../tests/RecordInterop.cmake)

两张布局图的归档为每个顶点保存一个父编号。C++ `add_edge(u, v)` 和 TS `addEdge(u, v)`、`addReference(u, v)` 可将已有根节点挂到编号更小的父节点。第二个父节点、重复边、自环和前向父引用都会被拒绝。C++ 返回的成功标志为 `false`，TS 返回 `null`。C++ 也会拒绝无效编号。新挂接的节点按编号插入兄弟序列，保证同名节点的查找结果在归档往返后稳定。C++ `remove_edge(e)` 可将整棵子树拆为独立的树。

`ResourceGraph` 支持按编号遍历顶点、访问 Name、Desc、Traits、States、Sampler 五个组件。C++ 的 `Kind` 和 TS 的 `ResourceGraphValue` 沿用旧版的 0–8 顺序，依次为 Managed、ManagedBuffer、ManagedTexture、PersistentBuffer、PersistentTexture、Framebuffer、Swapchain、FormatView、SubresourceView。Framebuffer 在 C++ 中持有 `IntrusivePtr<gfx::Framebuffer>`，TS 直接保存传入的 GFX 对象。四类缓冲与纹理记录的 `fenceValue` 在 C++ 中保留完整的 `uint64_t`，TS 接受 0 到 `Number.MAX_SAFE_INTEGER` 的整数。TS 通过 `RenderGraphObjectPool` 复用图和记录。[资源图声明](../spec/render/render-graph.spec)

`name_index;` 为 `named_graph` 选定的名称组件建立全图唯一索引。`ResourceGraph` 在两端都提供 `contains(name)`、`find(name)`、`vertex(name)`。名称缺失时，`contains` 返回 `false`，`find` 返回无效顶点编号。TS 的 `vertex` 由调用者保证名称已存在，C++ 的 `vertex` 在名称缺失时抛出异常。空字符串也是合法键。添加顶点或通过 `setName(v, name)` 改名遇到重名时，操作失败，原顶点和索引保持一致。C++ 使用支持 `std::string_view` 查询的 PMR 有序映射，TS 使用随图清空的 `Map`。

`ResourceGraph` 用 `hierarchy_graph { storage: shared_edges; }` 让普通边和父子引用共用一份关系。每个顶点最多有一个父节点，纹理可以有多个子视图，视图也可以派生子视图。创建时可指定父节点，已有根节点也可挂到编号比它大的节点下。挂接会拒绝重复边、第二个父节点和环。已有父子关系和名称索引在失败后保持一致。

图和普通记录使用 C++ 成员函数语法。用 `vertex_descriptor = uint32_t;` 声明顶点描述符，在图内写 `bool isTextureView(vertex_descriptor resID) const noexcept;`，在图外写 `ResourceGraph::vertex_descriptor`。`ResourceGraph` 的八个原生资源管理方法由生成头声明，由引擎提供定义。C++17 消费测试核对全部签名，并用引擎的查询逻辑区分普通纹理、格式视图和子资源视图。[图声明](../spec/render/render-graph.spec) · [引擎实现](../../cocos4/native/cocos/renderer/pipeline/custom/NativeResourceGraph.cpp) · [消费测试](../tests/consumer/records/ResourceGraphConsumer.cpp)

`ResourceGraph.nextFenceValue` 和 `version` 初值为 0。C++ 使用 `uint64_t`，TS 沿用 `fenceValue` 的安全整数范围，调用 `clear()` 或从池中重新取得图时两个字段都归零。`PersistentRenderPassAndFramebuffer` 持有可空的渲染通道和帧缓冲句柄。C++ 的 `clearColors` 使用构造时指定的内存资源。TS 复用该记录时原地清空颜色数组，将深度和模板清除值归零，并使用新传入的句柄。[渲染图声明](../spec/render/render-graph.spec)

`ClearValue`、`RasterView` 和 `ComputeView` 已接入 `RenderGraph`。记录按声明顺序生成 C++ 比较与哈希，字段上的 `@notElement` 从两者中排除该字段。`RasterView.slotID` 和 `ComputeView.clearValue` 使用此标注。外部类型 `gfx::Color` 上的 `@constRef` 让原生构造函数按 `const gfx::Color&` 接收颜色值。[外部 GFX 类型](../spec/extern/gfx.spec)

`RasterView` 和 `ComputeView` 各保留一条带 `defaults` 的共享构造。原生重载用 `@cppOnly` 标记，带第二个槽名或显式 `plane` 的重载要求传齐字段和分配器。两个视图的池化工厂复用嵌套对象，分别调用 `Color.reset()` 和 `ClearValue.reset(0, 0, 0, 0)`。`defaults` 优先使用字段初值。按值存储的外部类型 `T` 省略初值时，生成的 C++ 默认实参为 `T{}`，由编译器检查该表达式。例如 `RasterView` 和 `ClearView` 的 `clearColor` 均使用 `gfx::Color{}`。[渲染图声明与绑定](../spec/render/render-graph.spec) · [外部 GFX 类型](../spec/extern/gfx.spec)

`Subpass` 用三个容器保存光栅视图、计算视图和解析对。TS 对象池在复用时原地清空这三个容器。`SubpassGraph` 用 `incidence_graph` 存储出边，用 `bidirectional_graph` 存储入边，两端都保留插入顺序，允许多个前驱、平行边和自环。按描述符删除一条匹配边，按端点删除该方向的全部匹配边。C++ 的图比较和哈希按顶点顺序使用名称与 `Subpass` 两列，连边不同的图可以命中同一个缓存键。[渲染图规格](../spec/render/render-graph.spec) · [原生用例](../tests/consumer/records/SubpassGraphConsumer.cpp) · [TS 用例](../tests/consumer/records/subpass-graph-consumer.ts)

`ResourceGraph.renderPasses` 在 C++ 中用 `RasterPass` 作键。比较与哈希依次使用 `rasterViews`、`computeViews`、`textures`、`subpassGraph`、`width`、`height`、`count` 和 `quality`。插入缓存时，键中的子通道图和值中的清除颜色都使用资源图的分配器。TS 绑定通过 `mapKey string` 接受调用方提供的字符串，`ResourceGraph.clear()` 原地清空缓存。池中复用的 `RasterPass` 保留四张 Map、子通道图和视口的对象引用，恢复默认值。[规格](../spec/render/render-graph.spec) · [C++ 缓存用例](../tests/consumer/records/RasterPassConsumer.cpp) · [TS 复用用例](../tests/consumer/records/resource-graph-consumer.ts)

`RasterSubpass` 和 `ComputeSubpass` 均可用 `T{alloc}` 在指定资源上初始化，指定字段时使用 `T{std::allocator_arg, alloc, ...}`。`ComputePass`、`ResolvePass`、`CopyPass`、`MovePass` 和 `RaytracePass` 通过 `T(alloc)` 接收指定资源。`CopyPass` 通过 `@moveOnly` 保留上传数据的移动语义。TS 对象池复用已有的 Map、数组和视口。两个 Subpass 的工厂接收新的构造参数，省略参数时使用默认值。[C++ 跨分配器用例](../tests/consumer/records/PassConsumer.cpp) · [TS 对象池用例](../tests/consumer/records/pass-consumer.ts)

`ClearView`、`RenderQueue`、`SceneData`、`Dispatch`、`Blit` 和 `RenderData` 已接入渲染图规格，剔除标志 `CullingFlags` 和绘制类型 `BlitType` 随模块生成。`RenderQueue.viewport` 在 C++ 中按值存储，TS 通过 `field viewport default null` 声明为 `Viewport | null`。`SceneData` 复用 `LightInfo` 时，按声明模块中的参数顺序和默认值调用其重置方法。`RenderData` 通过 `@moveOnly` 声明移动语义，常量字节和四张资源表使用目标分配器。[渲染图规格](../spec/render/render-graph.spec) · [C++ 资源所有权测试](../tests/consumer/records/RenderDataConsumer.cpp) · [TS 对象池测试](../tests/consumer/records/render-node-consumer.ts)

`RenderGraph` 同时声明双向执行边和 `hierarchy_graph { storage: separate_edges; }`。创建顶点时传入父节点只会添加一条父子引用。每个顶点可以有多个父节点，`get_parent`／`getParent` 返回仍在图中的第一个父节点。两种关系都保留平行边和插入顺序。按描述符删除一条，按端点删除对应关系的全部匹配项。四个组件和 14 种载荷保留引擎定义的顺序。`RasterPassTag as RasterPass` 复用既有原生标签，并指定两端公开的枚举名。顶点可以重名，调用方单独维护 `index`。[渲染图规格](../spec/render/render-graph.spec) · [C++ 跨分配器与边操作](../tests/consumer/records/RenderGraphConsumer.cpp) · [TS 边操作与对象池](../tests/consumer/records/render-graph-consumer.ts)

## 第一次合并及后续验收

1. **把本次基线做成可以重跑的测试。** 在 `generator` 记录旧生成器和引擎的基线提交、52 条引擎相对路径及 SHA-256，单列 CMake 源列表检查。Windows 对照任务固定检出和换行策略，在临时目录复建旧工具。日常测试读取固定引擎版本的文件，旧工具用于复核来源。
2. **交付一个能端到端工作的 Archive 模块。** CI 先单独检出 `generator`，用仓库中的 CMake 配置构建 `cocos-codegen`。第一批实现只需支持这两个接口所用的声明、外部类型、语言绑定、文件输出和只读检查。生成表中的 4 个路径，与 Windows 基线逐字节对照。把这四个文件放进隔离的引擎副本，分别用真实的 `BinaryArchive.h` 和 `binary-archive.ts` 读写同一组布尔值、数值和 ASCII 字符串，互读对方写出的数据。[原生读写](../../cocos4/native/cocos/renderer/pipeline/custom/BinaryArchive.h) · [Web 读写](../../cocos4/cocos/rendering/custom/binary-archive.ts)
3. **验证 Cocos 图算法与运行时。** 在生成器阶段用空图、孤立顶点、菱形依赖、有环图和长链验证 DFS 的覆盖范围、依赖顺序、环诊断和显式栈。C++17 消费用例只包含对应的 `*Types.h`，构造顶点、调用成员访问组件并遍历出边，链接该模块的 `*Types.cpp` 后运行。TS 侧验证 `locateChild`、`getParent`。用例覆盖边与父子关系共享和独立存储，以及过滤后访问的顶点和顺序。CI 检查生成器和原生用例的实际头文件依赖，含预编译头。检测到 `boost/graph/` 时检查失败，并报告引入路径。[旧 LayoutGraph 描述](../../cocos-engine-generator/Cocos/CocosBuilder/LayoutGraph.cpp) · [旧 RenderGraph 描述](../../cocos-engine-generator/Cocos/CocosBuilder/RenderGraph.cpp)
4. **每批都放回引擎验证。** 在隔离的 `cocos4` 副本中按清单放入本批产物。图模块同批更新手写调用方、原生测试以及生成代码的包含和调用，例如重新生成 `LayoutGraphSerialization.cpp`。移除该批退役的图头和对应 CMake 条目，在全新构建目录编译并链接。Web 侧执行引擎的 `tsc --noEmit` 并运行图和对象池用例，编辑器资产由其消费端单独验证。绑定阶段执行引擎的 SWIG 步骤，编译生成的包装，并从 JS 调用一次渲染接口。布局归档用已有数据及跨 C++/TS 的样本验证字段顺序和枚举值。[原生测试调用方](../../cocos4/native/cocos/renderer/pipeline/custom/test/test.h) · [布局序列化](../../cocos4/native/cocos/renderer/pipeline/custom/LayoutGraphSerialization.cpp) · [Web 消费方](../../cocos4/cocos/rendering/custom/web-pipeline.ts) · [原生布局读取](../../cocos4/native/cocos/renderer/pipeline/custom/NativeFactory.cpp) · [引擎构建](../../cocos4/native/CMakeLists.txt)
5. **按模块切换生成入口。** 新工具默认写入 `generator/build/staging/cocos4/`。建议从 `generator` 运行 `cocos-codegen generate --workspace config/workspace.json --check --diff` 比较暂存目录。引擎对照任务按批核对目标产物和已退役路径。仅替换生成器内部算法时，继续要求产物字节一致。切换引擎图接口时，逐文件审查成员化与适配代码带来的差异，以消费者编译、遍历事件和调度结果验收，再更新对应基线。每次接管记录模块、输入版本和产物清单。回退时恢复该批产物、旧头、CMake 登记及配套调用方。

引擎文件归属按具体路径登记。同步时只接受本批标为 `generated` 的路径。`NativePipeline.cpp`、手写 Web 实现和编辑器 `.meta` 由引擎维护。`native/CMakeLists.txt` 的受管范围是 renderer 源列表。旧基线登记了 41 个原生生成文件，四个图头按批移出后核对 37 个原生生成文件的登记。

迁移完成须同时通过生成器自举和引擎验收。从干净检出构建的候选工具须能复现全部自身模型，并通过 **48 个引擎目标文件对照、4 个旧图头退役、重复生成零改写、Web 与原生消费测试**。Windows 负责历史基线与新版目标清单的对照，Linux 和 macOS 可使用同一份声明验证新工具的构建与生成。
