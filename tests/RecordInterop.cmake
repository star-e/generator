include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
set(stage "${work}/engine")
# Compile a uint32_t high bit alongside the production enums in both consumer languages.
file(COPY "${SOURCE}/spec" "${SOURCE}/config" "${SOURCE}/runtime" DESTINATION "${work}")
file(WRITE "${work}/spec/text.spec" [[extern module Text;
cpp { header "SampleText.h"; }
namespace sample { @string class Text; @string struct Label; struct ThrowOnMove; template<T> struct Box; }
binding typescript {
    Label => Caption from "cocos/rendering/custom/caption";
}
]])
file(WRITE "${work}/spec/shift.spec" "module ShiftProbe; import LayoutGraph; import RenderCommon; import Strings; import Containers; import Sets; import Gfx; import Text; import ArchiveInterface;
namespace cc {
    flags ShiftProbe : uint32_t { HIGH = 1 << 31 }
    interface ConversionProbe {
        explicit operator uint32_t() const noexcept { return 17; }
        virtual explicit operator bool() const = 0;
        virtual bool enabled(bool value = true) const = 0;
        static bool defaultsMatch(ShiftProbe flag = ShiftProbe::HIGH, float factor = 0.5,
            const uint32_t* ptr = nullptr, uint64_t mask = 18446744073709551615) {
            return flag == ShiftProbe::HIGH && factor == 0.5F && ptr == nullptr && mask == UINT64_MAX;
        }
    }
    struct ImplicitCtor { count: uint32_t = 7; }
    struct EmptyCtor { count: uint32_t = 7; constructor(); }
    struct RequiredCtor { count: uint32_t = 7; enabled: bool = true; constructor(count, enabled); }
    enum class DefaultKind { ZERO, ONE }
    struct DefaultCtor { count: uint32_t = 7; enabled: bool; kind: DefaultKind; constructor(count, enabled, kind) defaults; }
    struct WrappedCtor { value: DefaultCtor; constructor(value) defaults; }
    @cppOnly struct ExternalDefaultProbe { box: sample::Box<uint32_t>; constructor(box) defaults; }
    struct OverloadedCtor { value: RequiredCtor; count: uint32_t = 7; @cppOnly constructor(value, count); constructor(count, value); }
    struct VectorProbe { bytes: ccstd::vector<uint8_t>; words: ccstd::vector<uint32_t>; constructor(bytes, words) defaults; }
    struct AllocatorCtorProbe {
        count: uint32_t = 0;
        countIn: ccstd::pmr::string;
        @cppOnly constructor(count);
        constructor(count, countIn);
    }
    @cppOnly struct NestedAllocatorCtorProbe {
        required: render::DescriptorDB;
        count: uint32_t = 7;
        constructor(required, count);
    }
    @cppOnly @moveOnly struct ThrowingMoveProbe {
        member: sample::ThrowOnMove;
        text: ccstd::pmr::string;
    }
    struct NestedProbe {
        count: uint32_t;
        rows: ccstd::vector<ccstd::vector<render::Descriptor>>;
        byteArrays: ccstd::vector<ccstd::vector<uint8_t>>;
        tables: ccstd::vector<ccstd::map<render::LayoutType, ccstd::vector<gfx::UniformBlock>>>;
        passes: ccstd::vector<PmrTransparentSet<render::RenderPassType>>;
        keyed: ccstd::vector<ccstd::map<render::DescriptorGroupBlockIndex, render::DescriptorBlockIndex>>;
        index: render::DescriptorBlockIndex;
        pair: render::ResolvePair;
        @cppOnly constructor() defaults;
        constructor(index, pair, count) defaults;
    }
    @serialize struct TextProbe { text: sample::Text; label: sample::Label; constructor(text, label) defaults; }
    @serialize struct ArrayArchiveProbe { readonly rows: ccstd::vector<ccstd::vector<bool>>; }
    struct UniformIDProbe { id: render::UniformID = 0xFFFFFFFF; local: LocalUniformID = 0; }
    struct LocalIDProbe { id: render::NameLocalID; values: ccstd::map<render::NameLocalID, render::NameLocalID>; }
    using LocalUniformID = UniformWord;
    using UniformWord = render::UniformID;
    @moveOnly struct MoveProbe { value: uint32_t = 0; }
    interface DispatchProbe {
        @cppOnly virtual uint32_t apply(MoveProbe packet, const int* marker = nullptr, uint32_t scale = 3) const = 0;
        @cppOnly virtual uint32_t apply(MoveProbe packet, const bool* marker, uint32_t scale) const = 0;
        @cppOnly virtual bool matches(const ccstd::string& name = \"default\", const ccstd::string& label = {},
            gfx::ShaderStageFlagBit stages = gfx::ShaderStageFlagBit::VERTEX | gfx::ShaderStageFlagBit::FRAGMENT) const = 0;
    }
    @comparison(\">\", \"==\") struct ComparisonProbe { major: uint32_t = 0; @notElement scratch: uint32_t = 0; minor: uint32_t = 0; }
    graph TinyNames {
        vertex_descriptor Index = uint8_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        incidence_graph { out_edges: vector; }
        component cc::render::Name_: ccstd::pmr::string { member: labels; }
    }
    graph FlatComponents {
        vertex_descriptor Index = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        component cc::render::Descriptors_: render::DescriptorDB { member: data; }
        component cc::render::Name_: ccstd::pmr::string { member: labels; }
        component Count_: uint32_t { member: counts; }
        version: uint64_t = 1;
        named_graph { component: cc::render::Name_; }
    }
    @serialize graph IndexedNames {
        vertex_descriptor Index = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Count_: uint32_t { member: counts; }
        component cc::render::Name_: ccstd::pmr::string { member: labels; }
        name_index;
        named_graph { component: cc::render::Name_; }
        polymorphic_graph { Count_: uint32_t { member: values; } }
    }
    graph HierarchyOnly {
        vertex_descriptor Index = uint32_t;
        vertex_storage: indexed;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Count_: uint32_t { member: counts; }
    }
    using ToggleFlag = bool;
    graph Toggles {
        vertex_descriptor Index = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        polymorphic_graph {
            Count_: bool { member: handles_; }
            render::ComputeTag: ToggleFlag { member: flags; }
            Layout_: render::LayoutGraph { member: layouts; }
        }
    }
    @tag struct Count_ {}
    @tag struct Layout_ {}
}
binding typescript {
    ::ccstd::vector<::uint32_t> => Uint32Array;
    poolModule \"cocos/core/memop\";
    VectorProbe => class { pool vp; };
}")
set(workspace "${work}/config/workspace.json")
file(READ "${workspace}" config)
string(JSON count LENGTH "${config}" modules)
string(JSON config SET "${config}" modules ${count} [[{
    "backend": "records", "spec": "../spec/shift.spec",
    "nativeRoot": "native", "header": "ShiftProbe.h", "forwardHeader": "ShiftProbeFwd.h", "serializationHeader": "ShiftSerialization.h",
    "typescript": "cocos/rendering/custom/shift-probe.ts"
}]])
math(EXPR count "${count} + 1")
string(JSON config SET "${config}" modules ${count} [[{
    "backend": "model", "spec": "../spec/text.spec", "typescript": "cocos/rendering/custom/text"
}]])
file(WRITE "${work}/spec/indexed-keys.spec" [[module IndexedKeys;
import RenderCommon;
import Strings;
import ArchiveInterface;
namespace cc {
    @tag struct SignedKey_ {}
    @serialize graph IndexedKeys {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component render::Name_: ccstd::pmr::string { member: labels; }
        component SignedKey_: int32_t { member: keys; }
        named_graph { component: render::Name_; }
        name_index;
        component_index { component: SignedKey_; }
        polymorphic_graph { SignedKey_: uint32_t { member: values; } }
    }
}
]])
math(EXPR count "${count} + 1")
string(JSON config SET "${config}" modules ${count} [[{
    "backend": "records", "spec": "../spec/indexed-keys.spec", "nativeRoot": "native",
    "header": "IndexedKeys.h", "serializationHeader": "IndexedKeysSerialization.h"
}]])
file(WRITE "${workspace}" "${config}")
run("${TOOL}" generate --workspace "${workspace}" --output "${stage}")
file(WRITE "${stage}/native/SampleText.h" [[#pragma once
#include <string>
namespace sample {
using Text = std::string;
using Label = std::string;
struct MoveFailure {};
struct ThrowOnMove {
    ThrowOnMove() = default;
    ThrowOnMove(ThrowOnMove&&) { throw MoveFailure{}; }
    ThrowOnMove& operator=(ThrowOnMove&&) = default;
};
template<class T> struct Box {
    explicit Box() = default;
    T value{42};
};
}
]])
file(WRITE "${stage}/cocos/rendering/custom/caption.ts" "export type Caption = string;\n")
file(READ "${stage}/cocos/rendering/custom/shift-probe.ts" script)
foreach(fragment IN ITEMS "export class ImplicitCtor {\n    count = 7;" "export class EmptyCtor {\n    constructor () {")
    string(FIND "${script}" "${fragment}" pos)
    if(pos EQUAL -1)
        message(FATAL_ERROR "Incorrect TypeScript constructor declaration: ${fragment}")
    endif()
endforeach()
if(script MATCHES "operator *[(]")
    message(FATAL_ERROR "C++ conversion operator leaked into the TypeScript interface")
endif()
string(FIND "${script}" [[import type { Caption } from './caption';]] captionImport)
if(captionImport EQUAL -1 OR NOT script MATCHES "label: Caption")
    message(FATAL_ERROR "Explicit TS binding on an owning string was lost")
endif()
foreach(expected IN ITEMS "enabled (value?: boolean): boolean;" "rows: Descriptor[][] = []" "byteArrays: Uint8Array[] = []"
                          "values: Map<number, number>"
                          "tables: Map<LayoutType, UniformBlock[]>[] = []"
                          "passes: Set<RenderPassType>[] = []"
                          "keyed: Map<string, DescriptorBlockIndex>[] = []"
                          "index: DescriptorBlockIndex = new DescriptorBlockIndex()"
                          [[import { DescriptorBlockIndex } from './layout-graph';]]
                          [[import type { Descriptor, DescriptorDB, LayoutGraph, LayoutType, RenderPassType } from './layout-graph';]]
                          [[import type { UniformBlock } from '../../gfx';]])
    string(FIND "${script}" "${expected}" pos)
    if(pos EQUAL -1)
        message(FATAL_ERROR "Missing nested container binding: ${expected}")
    endif()
endforeach()
function(source_between source start stop result)
    string(FIND "${${source}}" "${start}" first)
    if(first LESS 0)
        message(FATAL_ERROR "Missing engine source starting at ${start}")
    endif()
    string(SUBSTRING "${${source}}" ${first} -1 tail)
    string(FIND "${tail}" "${stop}" length)
    if(length LESS_EQUAL 0)
        message(FATAL_ERROR "Missing engine source between ${start} and ${stop}")
    endif()
    string(SUBSTRING "${tail}" 0 ${length} body)
    set(${result} "${body}" PARENT_SCOPE)
endfunction()
# Run the engine's sort routines against the generated queue types.
file(READ "${ENGINE}/native/cocos/renderer/pipeline/custom/NativeRenderQueue.cpp" nativeQueues)
source_between(nativeQueues "void RenderDrawQueue::sortOpaqueOrCutout()" "void RenderDrawQueue::recordCommandBuffer(" drawSort)
source_between(nativeQueues "void RenderInstancingQueue::sort()" "void RenderInstancingQueue::uploadBuffers(" instanceSort)
source_between(nativeQueues "void NativeRenderQueue::sort()" "void NativeRenderQueue::recordCommands(" queueSort)
file(WRITE "${stage}/NativeQueueSort.cpp" [[
#include "cocos/renderer/pipeline/custom/NativePipelineTypes.h"
#include <algorithm>
#include <tuple>
namespace cc::render {
]] "${drawSort}\n${instanceSort}\n${queueSort}\n}\n")
# Compile the engine's texture-view query with the generated descriptor and visitor member.
file(READ "${ENGINE}/native/cocos/renderer/pipeline/custom/NativeResourceGraph.cpp" nativeResourceGraph)
source_between(nativeResourceGraph "bool ResourceGraph::isTextureView(" "gfx::Buffer* ResourceGraph::getBuffer(" textureViewQuery)
string(REPLACE "vertex_descriptor" "Vertex" textureViewQuery "${textureViewQuery}")
string(REGEX REPLACE "visitObject[(][ \t\r\n]*resID, [*]this," "visit_object(resID," textureViewQuery "${textureViewQuery}")
file(WRITE "${stage}/ResourceGraphTextureView.cpp" [[
#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include <tuple>
namespace cc::render {
]] "${textureViewQuery}\n}\n")
# Exercise the dispatcher configuration and accessors directly from the handwritten engine source.
file(READ "${ENGINE}/native/cocos/renderer/pipeline/custom/FrameGraphDispatcher.cpp" dispatcherSource)
source_between(dispatcherSource "void FrameGraphDispatcher::enablePassReorder(" "[[nodiscard]] ccstd::pmr::string concatResName(" dispatcherQueries)
string(REPLACE "::vertex_descriptor" "::Vertex" dispatcherQueries "${dispatcherQueries}")
string(REPLACE "resourceAccessGraph.passIndex.at(u)" "resourceAccessGraph.vertex(u)" dispatcherQueries "${dispatcherQueries}")
foreach(tag IN ITEMS Barrier PassNode RenderPassInfo)
    string(REPLACE "get(ResourceAccessGraph::${tag}Tag{}, resourceAccessGraph, ragVertID)"
        "resourceAccessGraph.get${tag}(ragVertID)" dispatcherQueries "${dispatcherQueries}")
endforeach()
string(REPLACE "holds<RasterPassTag>(u, renderGraph)" "renderGraph.holds<RasterPassTag>(u)" dispatcherQueries "${dispatcherQueries}")
file(WRITE "${stage}/DispatcherQueries.cpp" [[
#include "cocos/renderer/pipeline/custom/FGDispatcherTypes.h"
#include "cocos/renderer/pipeline/custom/details/GslUtils.h"
#include "cocos/math/Vec2.h"
namespace cc::render {
]] "${dispatcherQueries}\n}\n")
# Compile the engine's original printer and visibility formatter against its own graph types.
file(READ "${ENGINE}/native/cocos/renderer/pipeline/custom/LayoutGraphUtils.cpp" nativeLayoutUtils)
source_between(nativeLayoutUtils "ccstd::string getName(gfx::ShaderStageFlagBit" "gfx::TextureType getTextureType(" legacyPrint)
file(WRITE "${stage}/LegacyLayoutGraphPrint.cpp" [[
#include "cocos/renderer/pipeline/custom/LayoutGraphGraphs.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphNames.h"
#include "cocos/renderer/pipeline/custom/details/DebugUtils.h"
#include "cocos/renderer/pipeline/custom/details/Range.h"
namespace cc::render {
namespace {
]] "${legacyPrint}\n} // namespace cc::render\n")
# The descriptor type mapping is still provided by the engine for both consumers.
source_between(nativeLayoutUtils "gfx::DescriptorType getGfxDescriptorType(" "DescriptorTypeOrder getDescriptorTypeOrder(" descriptorTypeMapping)
file(WRITE "${stage}/DescriptorTypeMapping.cpp" [[
#include "cocos/base/Log.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
namespace cc::render {
]] "${descriptorTypeMapping}\n}\n")
source_between(nativeLayoutUtils "void initializeDescriptorSetLayoutInfo(" "\nnamespace {" legacyBindings)
source_between(nativeLayoutUtils "gfx::DescriptorSet* getOrCreatePerPassDescriptorSet(" "void generateConstantMacros(" legacyPerPass)
file(WRITE "${stage}/LegacyLayoutDescriptorSet.cpp" [[
#include "gfx-base/GFXDevice.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphGraphs.h"
namespace cc::render {
gfx::DescriptorType getGfxDescriptorType(DescriptorTypeOrder type);
]] "${legacyBindings}\n${legacyPerPass}\n}\n")
set(build "${work}/build")
build_consumer("${SOURCE}/tests/consumer/records" "${build}" "${stage}")
file(READ "${build}/consumer-${CONFIG}.txt" consumer)
run("${consumer}" pmr-move-termination)
set(pathCases "${work}/layout-paths.bin")
run("${consumer}" path-cases "${pathCases}")
# Each injected failure runs independently, including the MSVC debug STL's noexcept hash proxy allocation.
foreach(count IN ITEMS 2 16)
    set(failAfter 0)
    set(proxyFailure FALSE)
    while(TRUE)
        execute_process(COMMAND "${consumer}" stage-allocation ${count} ${failAfter} RESULT_VARIABLE rc)
        if(rc STREQUAL "0")
            break()
        elseif(rc STREQUAL "43" AND NOT proxyFailure)
            set(proxyFailure TRUE)
            message(STATUS "Stage at vertex ${count}: MSVC debug hash proxy terminated on allocation ${failAfter}")
        elseif(NOT rc STREQUAL "42")
            message(FATAL_ERROR "Stage insertion at vertex ${count}, allocation ${failAfter}: result ${rc}")
        endif()
        math(EXPR failAfter "${failAfter} + 1")
    endwhile()
endforeach()
set(dfsCases "${work}/dfs-cases.bin")
run("${consumer}" dfs-cases "${dfsCases}")

# Exercise the engine's actual pool implementation with the generated pool and record.
foreach(path IN ITEMS cocos/core/memop/recycle-pool.ts cocos/core/memop/scalable-container.ts cocos/core/utils/array.ts)
    configure_file("${ENGINE}/${path}" "${stage}/${path}" COPYONLY)
endforeach()
file(WRITE "${stage}/cocos/core/memop/index.ts" "export { RecyclePool } from './recycle-pool';\n")
file(MAKE_DIRECTORY "${stage}/cocos/core/platform" "${stage}/cocos/render-scene/scene")
file(WRITE "${stage}/cocos/core/platform/debug.ts" "export function logID(id: number): void { throw Error(String(id)); }\n")
set(scene "${stage}/cocos/render-scene/scene")
set(window "${stage}/cocos/render-scene/core/render-window.d.ts")
file(MAKE_DIRECTORY "${stage}/cocos/render-scene/core" "${stage}/cocos/asset/assets")
set(material "${stage}/cocos/asset/assets/index.d.ts")
set(renderScene "${stage}/cocos/render-scene/core/render-scene.d.ts")
set(engineAPI "${ENGINE}/bin/.declarations/cc.d.ts")
if(EXISTS "${engineAPI}")
    file(WRITE "${window}" "import { renderer } from 'cc'; export import RenderWindow = renderer.RenderWindow;\n")
    list(APPEND declarations "${engineAPI}")
    file(WRITE "${scene}/index.d.ts" "import { renderer } from 'cc'; export import Light = renderer.scene.Light; export import Model = renderer.scene.Model;\n")
    file(WRITE "${scene}/camera.d.ts" "import { renderer } from 'cc'; export import Camera = renderer.scene.Camera;\n")
    file(WRITE "${renderScene}" "import { renderer } from 'cc'; export import RenderScene = renderer.RenderScene;\n")
    file(WRITE "${material}" "export { Material } from 'cc';\n")
    file(WRITE "${scene}/reflection-probe.d.ts" "import { renderer } from 'cc'; export import ReflectionProbe = renderer.scene.ReflectionProbe;\n")
else()
    file(WRITE "${scene}/index.d.ts" "export declare class Light { private light: void; } export declare class Model { private model: void; }\n")
    file(WRITE "${scene}/camera.d.ts" "export declare class Camera { private camera: void; }\n")
    file(WRITE "${renderScene}" "export declare class RenderScene { private scene: void; }\n")
    file(WRITE "${material}" "export declare class Material { private material: void; }\n")
    file(WRITE "${window}" "export declare class RenderWindow { private window: void; }\n")
    file(WRITE "${scene}/reflection-probe.d.ts" "export declare class ReflectionProbe { private probe: void; }\n")
endif()
set(mathApi "${stage}/cocos/core/math/index.d.ts")
file(MAKE_DIRECTORY "${stage}/cocos/core/math")
if(EXISTS "${engineAPI}")
    file(WRITE "${mathApi}" "export { Mat4, Quat, Vec2, Vec4 } from 'cc';\n")
else()
    foreach(type IN ITEMS Mat4 Quat Vec2 Vec4)
        file(APPEND "${mathApi}" "export declare class ${type} { private brand${type}: void; }\n")
    endforeach()
endif()
foreach(type IN ITEMS DirectionalLight SphereLight SpotLight PointLight RangedDirectionalLight)
    string(REGEX REPLACE "([a-z])([A-Z])" "\\1-\\2" filename "${type}")
    string(TOLOWER "${filename}" filename)
    if(EXISTS "${engineAPI}")
        file(WRITE "${scene}/${filename}.d.ts" "import { renderer } from 'cc'; export import ${type} = renderer.scene.${type};\n")
    else()
        file(WRITE "${scene}/${filename}.d.ts" "import { Light } from './index'; export declare class ${type} extends Light { private brand${type}: void; }\n")
    endif()
endforeach()
find_program(node node REQUIRED)
# The engine allows implicit any in MutableGraph.addVertex(...args). Compile its runtime
# with that option, then check consumers against its declarations with full strictness.
set(tsc "${ENGINE}/node_modules/typescript/bin/tsc")
run("${node}" "${tsc}" --strict --noImplicitAny false --skipLibCheck --target es6 --module commonjs --declaration
    --declarationDir "${stage}/cocos/rendering/custom" --outDir "${work}/js/rendering/custom"
    "${ENGINE}/cocos/rendering/custom/graph.ts")
# Isolate the engine's exported enums without loading its graphics device at runtime.
file(READ "${ENGINE}/cocos/gfx/base/define.ts" gfx)
file(MAKE_DIRECTORY "${stage}/cocos/gfx")
foreach(name IN ITEMS ResolveMode Type ShaderStageFlagBit MemoryAccessBit ViewDimension SampleType Format DescriptorType
        AccessFlagBit SampleCount TextureType TextureFlagBit Filter Address ComparisonFunc LoadOp StoreOp ClearFlagBit)
    string(REGEX MATCH "export enum ${name} \\{[^}]+\\}" definition "${gfx}")
    if(NOT definition)
        message(FATAL_ERROR "Missing ${name} in engine gfx definitions")
    endif()
    string(APPEND gfxEnums "${definition}\n")
endforeach()
# Include the engine's value classes used by DescriptorBlockFlattened.
string(REGEX MATCH "interface ICopyable [^\n]+\n\nconst deepCopy[^\n]+(\n[ \t][^\n]*)*\n[}];" copyHelper "${gfx}")
if(NOT copyHelper)
    message(FATAL_ERROR "Missing UniformBlock copy helper in engine gfx definitions")
endif()
set(gfxClasses "type Constructor<T> = new () => T;\n${copyHelper}\n")
foreach(name IN ITEMS Uniform UniformBlock DescriptorSetLayoutBinding DescriptorSetLayoutInfo DescriptorSetInfo SamplerInfo Color Viewport)
    string(REGEX MATCH "export class ${name} [{]\n([ \t][^\n]*\n|\n)*[}]" definition "${gfx}")
    if(NOT definition)
        message(FATAL_ERROR "Missing ${name} class in engine gfx definitions")
    endif()
    string(APPEND gfxClasses "${definition}\n")
endforeach()
file(WRITE "${stage}/cocos/gfx/index.ts" "${gfxEnums}${gfxClasses}"
    "export type ShaderStageFlags = ShaderStageFlagBit; export type MemoryAccess = MemoryAccessBit;\n"
    "export interface Attribute { name: string; } export interface ShaderInfo { name: string; }\n"
    "export interface Shader { readonly name: string; } export interface Device { readonly id: number; } export interface PipelineState {}\n"
    "export interface Sampler {}\n"
    "export interface BufferInfo {} export interface TextureInfo {} export interface CommandBuffer {}\n"
    "export declare class Buffer { private buffer: void; }\n"
    "export declare class Texture { private texture: void; }\n"
    "export declare class Framebuffer { private framebuffer: void; }\n"
    "export declare class RenderPass { private renderPass: void; }\n"
    "export declare class Swapchain { private swapchain: void; }\n"
    "export declare class DescriptorSet { private set: void; }\n"
    "export declare class DescriptorSetLayout { private layout: void; }\n"
    "export declare class PipelineLayout { private pipeline: void; }\n")
set(constants "${stage}/cocos/internal-constants.d.ts")
file(WRITE "${constants}" "declare module 'internal:constants' { export const COCOS_RUNTIME: boolean; export const HTML5: boolean; }\n")
list(APPEND declarations "${constants}")
file(READ "${ENGINE}/cocos/rendering/custom/layout-graph.ts" layout)
# Compile the old graph and its archive functions separately from the generated implementation.
function(ts_definition source symbol result)
    string(REGEX MATCH "${symbol} [^\n]+[{]\n([ \t][^\n]*\n|\n)*[}]" body "${${source}}")
    if(NOT body)
        message(FATAL_ERROR "Missing engine TypeScript definition ${symbol}")
    endif()
    set(${result} "${body}" PARENT_SCOPE)
endfunction()
set(custom "${stage}/cocos/rendering/custom")
file(WRITE "${stage}/cocos/render-scene/core/program-lib.d.ts" "export interface IProgramInfo { name: string; }\n")
file(APPEND "${material}" "export interface EffectAsset { readonly name: string; }\n")
file(WRITE "${stage}/cocos/render-scene/core/pass-utils.d.ts" "export type MacroRecord = Record<string, string | number | boolean>;\n")
configure_file("${ENGINE}/cocos/rendering/custom/private.ts" "${custom}/legacy-private.ts" COPYONLY)
configure_file("${ENGINE}/cocos/rendering/custom/pipeline.ts" "${custom}/legacy-pipeline.ts" COPYONLY)
file(READ "${custom}/legacy-pipeline.ts" legacyPipeline)
file(READ "${custom}/pipeline.ts" generatedPipeline)
string(REGEX MATCHALL "export (enum|interface|class) [A-Za-z0-9_]+" legacyExports "${legacyPipeline}")
string(REGEX MATCHALL "export (enum|interface|class) [A-Za-z0-9_]+" generatedExports "${generatedPipeline}")
if(NOT legacyExports STREQUAL generatedExports)
    message(FATAL_ERROR "RenderInterface exports differ from the engine: ${generatedExports}")
endif()
set(pipelineContract "${custom}/pipeline-contract.ts")
file(WRITE "${pipelineContract}" "import type * as G from './pipeline';\nimport type * as L from './legacy-pipeline';\n"
    "type Compatible<T extends U, U> = T;\ntype API<T> = [keyof T, T];\n")
foreach(declaration IN LISTS legacyExports)
    if(declaration MATCHES "export interface (.+)")
        set(name "${CMAKE_MATCH_1}")
        file(APPEND "${pipelineContract}"
            "type G${name} = Compatible<API<G.${name}>, API<L.${name}>>;\n"
            "type L${name} = Compatible<API<L.${name}>, API<G.${name}>>;\n")
    endif()
endforeach()
file(WRITE "${stage}/cocos/rendering/geometry-renderer.d.ts" "export interface GeometryRenderer {}\n")
file(WRITE "${stage}/cocos/rendering/global-descriptor-set-manager.d.ts" "export interface GlobalDSManager {}\n")
file(WRITE "${stage}/cocos/rendering/pipeline-scene-data.d.ts" "export interface PipelineSceneData {}\n")
configure_file("${ENGINE}/cocos/rendering/custom/web-types.ts" "${custom}/legacy-web-types.ts" COPYONLY)
configure_file("${SOURCE}/tests/consumer/records/program-consumer.ts" "${custom}/program-consumer.ts" COPYONLY)
file(READ "${ENGINE}/cocos/rendering/custom/serialization.ts" codec)
foreach(type IN ITEMS Uniform UniformBlock DescriptorSetLayoutBinding DescriptorSetLayoutInfo)
    foreach(op IN ITEMS save load)
        string(REGEX MATCH "export function ${op}${type} [(][^\n]+[{]\n([ \t][^\n]*\n|\n)*[}]" definition "${codec}")
        if(NOT definition)
            message(FATAL_ERROR "Missing ${op}${type} in engine serialization.ts")
        endif()
        string(APPEND gfxCodec "${definition}\n")
    endforeach()
endforeach()
file(WRITE "${custom}/serialization.ts" "import { Uniform, UniformBlock, DescriptorSetLayoutBinding, DescriptorSetLayoutInfo } from '../../gfx';\n"
    "import type { InputArchive, OutputArchive } from './archive';\n${gfxCodec}")
source_between(layout "function resetDescriptorSetLayoutInfo " "export function saveDescriptor (" legacyGraph)
string(FIND "${layout}" "export function saveDescriptor (" codecStart)
if(codecStart EQUAL -1)
    message(FATAL_ERROR "Missing engine layout graph codecs")
endif()
string(SUBSTRING "${layout}" ${codecStart} -1 legacyArchive)
string(REGEX MATCH "import [{][^\n]* from './graph';" graphImports "${layout}")
file(WRITE "${custom}/legacy-layout.ts" "${graphImports}\nimport { ParameterType, UpdateFrequency, RenderCommonObjectPool } from './types';\n"
    "import { RecyclePool } from '../../core/memop';\n"
    "import { COCOS_RUNTIME, HTML5 } from 'internal:constants';\n"
    "import { DescriptorSet, DescriptorSetLayout, DescriptorSetLayoutInfo, PipelineLayout, Format, MemoryAccessBit, SampleType, ShaderStageFlagBit, Type, UniformBlock, ViewDimension } from '../../gfx';\n"
    "import type { InputArchive, OutputArchive } from './archive';\n"
    "import { saveUniformBlock, loadUniformBlock, saveDescriptorSetLayoutInfo, loadDescriptorSetLayoutInfo } from './serialization';\n${legacyGraph}\n${legacyArchive}")
foreach(name IN ITEMS DescriptorBlockIndex DescriptorGroupBlockIndex)
    string(REGEX MATCH "export class ${name} [^\n]+(\n[ \t][^\n]*)*\n}" definition "${layout}")
    if(NOT definition)
        message(FATAL_ERROR "Missing ${name} class in engine layout-graph.ts")
    endif()
    string(APPEND engineIndices "${definition}\n")
endforeach()
set(baseline "${stage}/cocos/rendering/custom/engine-index.ts")
file(WRITE "${baseline}" "import { ParameterType, UpdateFrequency } from './types';\n"
    "import { DescriptorTypeOrder } from './layout-graph';\n"
    "import { Format, MemoryAccessBit, SampleType, ShaderStageFlagBit, ViewDimension } from '../../gfx';\n${engineIndices}")
set(names "cocos/rendering/custom/types-names.ts")
run("${CMAKE_COMMAND}" -E compare_files "${stage}/${names}" "${ENGINE}/${names}")
set(layoutNames "cocos/rendering/custom/layout-graph-names.ts")
run("${CMAKE_COMMAND}" -E compare_files "${stage}/${layoutNames}" "${ENGINE}/${layoutNames}")
file(READ "${ENGINE}/${layoutNames}" legacyNames)
string(REPLACE "from './layout-graph'" "from './legacy-layout'" legacyNames "${legacyNames}")
file(WRITE "${custom}/legacy-layout-names.ts" "${legacyNames}")
set(enumConsumer "${stage}/cocos/rendering/custom/enum-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/enum-consumer.ts" "${enumConsumer}" COPYONLY)

# Compile the editor's lookup, traversal and descriptor propagation against the generated graph.
file(READ "${ENGINE}/cocos/rendering/custom/layout-graph-editor.ts" editor)
file(READ "${ENGINE}/cocos/rendering/custom/layout-graph-utils.ts" layoutUtils)
source_between(layoutUtils "export function sortDescriptorBlocks<" "function createDescriptorInfo (" layoutConversionUtils)
ts_definition(layoutUtils "export function getGfxDescriptorType" gfxDescriptorType)
source_between(layoutUtils "function createDescriptorInfo (" "function getDescriptorBlockData (" gfxDescriptorSets)
source_between(editor "function getGfxTypeName (" "function convertDescriptorBlock (" layoutPrintHelpers)
ts_definition(editor "class VectorGraphColorMap" colorMap)
source_between(editor "function convertDescriptorBlock (" "// cache of descriptor blocks" flattenBlock)
source_between(editor "function buildLayoutGraphDataImpl (" "export function printLayoutGraphData (" layoutConversion)
ts_definition(editor "export function printLayoutGraphData" printLayoutData)
foreach(layoutModule IN ITEMS layout-graph legacy-layout)
    set(conversionFile "${custom}/${layoutModule}-conversion.ts")
    configure_file("${SOURCE}/tests/consumer/records/layout-conversion.ts.in" "${conversionFile}" @ONLY)
    list(APPEND conversionFiles "${conversionFile}")
endforeach()
file(READ "${ENGINE}/cocos/rendering/custom/web-program-library.ts" webPrograms)
source_between(webPrograms "    constructor (lg: LayoutGraphData) {" "    init (deviceIn: Device): void {" programLibraryConstructor)
ts_definition(editor "class LayoutGraphPrintVisitor" layoutGraphPrintVisitor)
string(REGEX MATCH "    public print [(][)]: string [{]\n(        [^\n]*\n|\n)*    [}]" editorPrint "${editor}")
if(NOT editorPrint)
    message(FATAL_ERROR "Missing layout editor print method")
endif()
foreach(method IN ITEMS getPassID getSubpassID getPhaseID getDescriptorBlock getDescriptorGroupBlock
                        buildDescriptorBlocks buildDescriptorGroupBlocks)
    string(REGEX MATCH "    private ${method} [(][^)]*[)]: [^\n]+[{]\n(        [^\n]*\n|\n)*    [}]" definition "${editor}")
    if(NOT definition)
        message(FATAL_ERROR "Missing layout editor method ${method}")
    endif()
    string(APPEND editorMethods "${definition}\n")
endforeach()
string(REGEX MATCH "        for [(]const passID of lg[.]v[(][)][)] [{]\n(            [^\n]*\n|\n)*        [}]" updatePasses "${editor}")
if(NOT updatePasses)
    message(FATAL_ERROR "Missing layout editor pass traversal")
endif()
set(graphConsumer "${stage}/cocos/rendering/custom/layout-graph-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/layout-graph-consumer.ts.in" "${graphConsumer}" @ONLY)
set(dfsConsumer "${stage}/cocos/rendering/custom/graph-dfs-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/graph-dfs-consumer.ts" "${dfsConsumer}" COPYONLY)
set(resourceConsumer "${custom}/resource-graph-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/resource-graph-consumer.ts" "${resourceConsumer}" COPYONLY)
set(subpassConsumer "${custom}/subpass-graph-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/subpass-graph-consumer.ts" "${subpassConsumer}" COPYONLY)
set(passConsumer "${custom}/pass-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/pass-consumer.ts" "${passConsumer}" COPYONLY)
set(nodeConsumer "${custom}/render-node-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/render-node-consumer.ts" "${nodeConsumer}" COPYONLY)
set(renderGraphConsumer "${custom}/render-graph-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/render-graph-consumer.ts" "${renderGraphConsumer}" COPYONLY)

# Compile both versions against the same engine imports to exercise the bound defaults at runtime.
set(commonSpec "${work}/spec/render/render-common.spec")
file(READ "${commonSpec}" common)
string(REPLACE "numRenderPasses: uint32_t = 0" "numRenderPasses: uint32_t = 7" common "${common}")
file(WRITE "${commonSpec}" "${common}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/variant")
set(variant "${stage}/cocos/rendering/custom/types-variant.ts")
configure_file("${work}/variant/cocos/rendering/custom/types.ts" "${variant}" COPYONLY)
set(binaryArchive "cocos/rendering/custom/binary-archive.ts")
configure_file("${ENGINE}/${binaryArchive}" "${stage}/${binaryArchive}" COPYONLY)
run("${node}" "${tsc}" --strict --skipLibCheck --target es6 --module commonjs
    --outDir "${work}/js" --rootDir "${stage}/cocos" ${declarations} "${stage}/${names}" "${stage}/${layoutNames}"
    "${custom}/program-consumer.ts" "${pipelineContract}" "${enumConsumer}" "${graphConsumer}" "${resourceConsumer}" "${subpassConsumer}" "${passConsumer}" "${nodeConsumer}" "${renderGraphConsumer}" "${dfsConsumer}" "${baseline}" "${variant}" "${stage}/${binaryArchive}" ${conversionFiles})
run("${node}" "${work}/js/rendering/custom/program-consumer.js")
run("${node}" "${work}/js/rendering/custom/resource-graph-consumer.js")
run("${node}" "${work}/js/rendering/custom/subpass-graph-consumer.js")
run("${node}" "${work}/js/rendering/custom/pass-consumer.js")
run("${node}" "${work}/js/rendering/custom/render-node-consumer.js")
run("${node}" "${work}/js/rendering/custom/render-graph-consumer.js")
run("${consumer}" write "${work}/native.bin")
run("${node}" "${SOURCE}/tests/consumer/records/consumer.cjs" "${work}/js/rendering/custom" "${stage}" "${ENGINE}" "${work}/native.bin" "${work}/web.bin" "${pathCases}" "${dfsCases}")
run("${node}" "${SOURCE}/tests/consumer/records/layout-pool.cjs" "${work}/js/rendering/custom")
run("${consumer}" read "${work}/web.bin")
run("${CMAKE_COMMAND}" -E compare_files "${work}/native.bin" "${work}/web.bin")
run("${consumer}" layout-write "${work}/layout-native.bin")
run("${consumer}" blocks-write "${work}/blocks-native.bin")
set(legacyFixture "${SOURCE}/tests/consumer/records/fixtures/layout-graph.bin")
run("${node}" "${SOURCE}/tests/consumer/records/layout-archive.cjs" "${work}/js/rendering/custom"
    "${work}/layout-native.bin" "${work}/layout-web.bin" "${work}/layout-legacy.bin" "${work}/layout-invalid.bin" "${legacyFixture}"
    "${work}/blocks-native.bin" "${work}/blocks-web.bin" "${work}/blocks-legacy.bin")
run("${consumer}" blocks-read "${work}/blocks-web.bin")
run("${consumer}" blocks-read "${work}/blocks-legacy.bin")
run("${consumer}" pipeline-write "${work}/pipeline-native.bin")
run("${node}" "${SOURCE}/tests/consumer/records/pipeline-archive.cjs" "${work}/js/rendering/custom"
    "${work}/pipeline-native.bin" "${work}/pipeline-web.bin" "${work}/pipeline-legacy.bin" "${work}/converted-layout.bin" "${work}/print-layout.bin")
run("${consumer}" converted-layout-read "${work}/converted-layout.bin")
run("${node}" "${SOURCE}/tests/consumer/records/layout-bindings.cjs" "${work}/js/rendering/custom" "${work}/converted-layout.bin")
run("${consumer}" pipeline-read "${work}/pipeline-web.bin")
run("${consumer}" pipeline-read "${work}/pipeline-legacy.bin")
file(READ "${build}/legacy-${CONFIG}.txt" legacyConsumer)
run("${legacyConsumer}" layout-descriptors "${work}/converted-layout.bin" "${work}/native-descriptors-legacy.txt")
run("${consumer}" layout-descriptors "${work}/converted-layout.bin" "${work}/native-descriptors-current.txt")
run("${CMAKE_COMMAND}" -E compare_files "${work}/native-descriptors-legacy.txt" "${work}/native-descriptors-current.txt")
foreach(sample IN ITEMS print-layout converted-layout)
    run("${legacyConsumer}" layout-print "${work}/${sample}.bin" "${work}/${sample}-legacy.txt")
    run("${consumer}" layout-print "${work}/${sample}.bin" "${work}/${sample}-current.txt")
    run("${CMAKE_COMMAND}" -E compare_files "${work}/${sample}-legacy.txt" "${work}/${sample}-current.txt")
endforeach()
configure_file("${SOURCE}/tests/consumer/records/fixtures/layout-print.txt" "${work}/print-expected.txt" @ONLY NEWLINE_STYLE UNIX)
run("${CMAKE_COMMAND}" -E compare_files "${work}/print-expected.txt" "${work}/print-layout-current.txt")
run("${legacyConsumer}" "${work}/pipeline-native.bin" "${work}/pipeline-legacy-native.bin")
run("${CMAKE_COMMAND}" -E compare_files "${work}/pipeline-web.bin" "${work}/pipeline-legacy-native.bin")
run("${consumer}" pipeline-read "${work}/pipeline-legacy-native.bin")
run("${consumer}" layout-read "${work}/layout-web.bin")
run("${consumer}" layout-read "${work}/layout-legacy.bin")
run("${CMAKE_COMMAND}" -E compare_files "${legacyFixture}" "${work}/layout-legacy.bin")
run("${consumer}" layout-read "${legacyFixture}")
run("${consumer}" layout-invalid "${work}/layout-invalid.bin")
finish_workspace("${work}")
