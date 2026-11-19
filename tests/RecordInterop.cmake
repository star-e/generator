include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
set(stage "${work}/engine")
# Compile a uint32_t high bit alongside the production enums in both consumer languages.
file(COPY "${SOURCE}/spec" "${SOURCE}/config" DESTINATION "${work}")
file(WRITE "${work}/spec/text.spec" [[extern module Text;
namespace sample { @string class Text; @string struct Label; }
binding typescript { Label => Caption from "cocos/rendering/custom/caption"; }
]])
file(WRITE "${work}/spec/shift.spec" "module ShiftProbe; import LayoutGraph; import RenderCommon; import Strings; import Containers; import Sets; import GfxUniformBlock; import Text; import ArchiveInterface;
namespace cc {
    flags ShiftProbe : uint32_t { HIGH = 1 << 31 }
    struct VectorProbe { bytes: ccstd::vector<uint8_t>; words: ccstd::vector<uint32_t>; constructor(bytes, words); }
    struct NestedProbe {
        rows: ccstd::vector<ccstd::vector<render::Descriptor>>;
        byteArrays: ccstd::vector<ccstd::vector<uint8_t>>;
        tables: ccstd::vector<ccstd::map<render::LayoutType, ccstd::vector<gfx::UniformBlock>>>;
        passes: ccstd::vector<PmrTransparentSet<render::RenderPassType>>;
        keyed: ccstd::vector<ccstd::map<render::DescriptorGroupBlockIndex, render::DescriptorBlockIndex>>;
        index: render::DescriptorBlockIndex;
        pair: render::ResolvePair;
        constructor(index);
    }
    @serialize struct TextProbe { text: sample::Text; label: sample::Label; constructor(text, label); }
    @moveOnly struct MoveProbe { value: uint32_t = 0; }
    @comparison(\">\", \"==\") struct ComparisonProbe { major: uint32_t = 0; minor: uint32_t = 0; }
    graph TinyNames {
        vertex_descriptor Index = uint8_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
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
        named_graph { component: cc::render::Name_; }
    }
    graph HierarchyOnly {
        vertex_descriptor Index = uint32_t;
        vertex_storage: indexed;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Count_: uint32_t { member: counts; }
    }
    graph Toggles {
        vertex_descriptor Index = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        polymorphic_graph {
            Count_: bool { member: handles_; }
            render::ComputeTag: bool { member: flags; }
            Layout_: render::LayoutGraph { member: layouts; }
        }
    }
    @tag struct Count_ {}
    @tag struct Layout_ {}
}
binding typescript { ::ccstd::vector<::uint32_t> => Uint32Array; }")
set(workspace "${work}/config/workspace.json")
file(READ "${workspace}" config)
string(JSON count LENGTH "${config}" modules)
string(JSON config SET "${config}" modules ${count} [[{
    "backend": "records", "spec": "../spec/shift.spec",
    "nativeRoot": "native", "header": "ShiftProbe.h", "serializationHeader": "ShiftSerialization.h",
    "typescript": "cocos/rendering/custom/shift-probe.ts"
}]])
math(EXPR count "${count} + 1")
string(JSON config SET "${config}" modules ${count} [[{
    "backend": "model", "spec": "../spec/text.spec",
    "types": { "Text": { "header": "SampleText.h" }, "Label": { "header": "SampleText.h" } }
}]])
file(WRITE "${workspace}" "${config}")
run("${TOOL}" generate --workspace "${workspace}" --output "${stage}")
file(WRITE "${stage}/native/SampleText.h" "#pragma once\n#include <string>\nnamespace sample { using Text = std::string; using Label = std::string; }\n")
file(WRITE "${stage}/cocos/rendering/custom/caption.ts" "export type Caption = string;\n")
file(READ "${stage}/cocos/rendering/custom/shift-probe.ts" script)
string(FIND "${script}" [[import type { Caption } from './caption';]] captionImport)
if(captionImport EQUAL -1 OR NOT script MATCHES "label: Caption")
    message(FATAL_ERROR "Explicit TS binding on an owning string was lost")
endif()
foreach(expected IN ITEMS "rows: Descriptor[][] = []" "byteArrays: Uint8Array[] = []"
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
set(build "${work}/build")
build_consumer("${SOURCE}/tests/consumer/records" "${build}" "${stage}")
file(READ "${build}/consumer-${CONFIG}.txt" consumer)
set(pathCases "${work}/layout-paths.bin")
run("${consumer}" path-cases "${pathCases}")

# Exercise the engine's actual pool implementation with the generated pool and record.
foreach(path IN ITEMS cocos/core/memop/recycle-pool.ts cocos/core/memop/scalable-container.ts cocos/core/utils/array.ts)
    configure_file("${ENGINE}/${path}" "${stage}/${path}" COPYONLY)
endforeach()
file(WRITE "${stage}/cocos/core/memop/index.ts" "export { RecyclePool } from './recycle-pool';\n")
file(MAKE_DIRECTORY "${stage}/cocos/core/platform" "${stage}/cocos/render-scene/scene")
file(WRITE "${stage}/cocos/core/platform/debug.ts" "export function logID(id: number): void { throw Error(String(id)); }\n")
set(scene "${stage}/cocos/render-scene/scene")
set(engineAPI "${ENGINE}/bin/.declarations/cc.d.ts")
if(EXISTS "${engineAPI}")
    list(APPEND declarations "${engineAPI}")
    file(WRITE "${scene}/index.d.ts" "import { renderer } from 'cc'; export import Light = renderer.scene.Light;\n")
    file(WRITE "${scene}/reflection-probe.d.ts" "import { renderer } from 'cc'; export import ReflectionProbe = renderer.scene.ReflectionProbe;\n")
else()
    file(WRITE "${scene}/index.d.ts" "export declare class Light { private light: void; }\n")
    file(WRITE "${scene}/reflection-probe.d.ts" "export declare class ReflectionProbe { private probe: void; }\n")
endif()
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
foreach(name IN ITEMS ResolveMode Type ShaderStageFlagBit MemoryAccessBit ViewDimension SampleType Format)
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
foreach(name IN ITEMS Uniform UniformBlock)
    string(REGEX MATCH "export class ${name} [{]\n([ \t][^\n]*\n|\n)*[}]" definition "${gfx}")
    if(NOT definition)
        message(FATAL_ERROR "Missing ${name} class in engine gfx definitions")
    endif()
    string(APPEND gfxClasses "${definition}\n")
endforeach()
file(WRITE "${stage}/cocos/gfx/index.ts" "${gfxEnums}${gfxClasses}")
file(READ "${ENGINE}/cocos/rendering/custom/layout-graph.ts" layout)
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
set(enumConsumer "${stage}/cocos/rendering/custom/enum-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/enum-consumer.ts" "${enumConsumer}" COPYONLY)

# Compile the editor's lookup, traversal and descriptor propagation against the generated graph.
file(READ "${ENGINE}/cocos/rendering/custom/layout-graph-editor.ts" editor)
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

# Compile both versions against the same engine imports to exercise the bound defaults at runtime.
set(commonSpec "${work}/spec/render/render-common.spec")
file(READ "${commonSpec}" common)
string(REPLACE "numRenderPasses: uint32_t = 0" "numRenderPasses: uint32_t = 7" common "${common}")
# Also compile a default-reset pool for a record with explicit constructors.
string(REPLACE "pool li;" "pool li; reset defaults;" common "${common}")
file(WRITE "${commonSpec}" "${common}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/variant")
set(variant "${stage}/cocos/rendering/custom/types-variant.ts")
configure_file("${work}/variant/cocos/rendering/custom/types.ts" "${variant}" COPYONLY)
set(binaryArchive "cocos/rendering/custom/binary-archive.ts")
configure_file("${ENGINE}/${binaryArchive}" "${stage}/${binaryArchive}" COPYONLY)
run("${node}" "${tsc}" --strict --skipLibCheck --target es6 --module commonjs
    --outDir "${work}/js" --rootDir "${stage}/cocos" ${declarations} "${stage}/${names}" "${stage}/${layoutNames}"
    "${enumConsumer}" "${graphConsumer}" "${baseline}" "${variant}" "${stage}/${binaryArchive}")
run("${consumer}" write "${work}/native.bin")
run("${node}" "${SOURCE}/tests/consumer/records/consumer.cjs" "${work}/js/rendering/custom" "${stage}" "${ENGINE}" "${work}/native.bin" "${work}/web.bin" "${pathCases}")
run("${consumer}" read "${work}/web.bin")
run("${CMAKE_COMMAND}" -E compare_files "${work}/native.bin" "${work}/web.bin")
finish_workspace("${work}")
