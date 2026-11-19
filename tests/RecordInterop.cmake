include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
set(stage "${work}/engine")
# Compile a uint32_t high bit alongside the production enums in both consumer languages.
file(COPY "${SOURCE}/spec" "${SOURCE}/config" DESTINATION "${work}")
file(WRITE "${work}/spec/text.spec" [[extern module Text;
namespace sample { @string class Text; @string struct Label; }
binding typescript { Label => Caption from "cocos/rendering/custom/caption"; }
]])
file(WRITE "${work}/spec/shift.spec" "module ShiftProbe; import LayoutGraph; import Strings; import Text; import ArchiveInterface;
namespace cc {
    flags ShiftProbe : uint32_t { HIGH = 1 << 31 }
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
}")
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
set(build "${work}/build")
build_consumer("${SOURCE}/tests/consumer/records" "${build}" "${stage}")
file(READ "${build}/consumer-${CONFIG}.txt" consumer)
run("${consumer}")

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
file(WRITE "${stage}/cocos/gfx/index.ts" "${gfxEnums}")
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
run("${node}" "${ENGINE}/node_modules/typescript/bin/tsc" --strict --skipLibCheck --target es6 --module commonjs
    --outDir "${work}/js" --rootDir "${stage}/cocos" ${declarations} "${stage}/${names}" "${stage}/${layoutNames}"
    "${enumConsumer}" "${baseline}" "${variant}" "${stage}/${binaryArchive}")
run("${consumer}" write "${work}/native.bin")
run("${node}" "${SOURCE}/tests/consumer/records/consumer.cjs" "${work}/js/rendering/custom" "${stage}" "${ENGINE}" "${work}/native.bin" "${work}/web.bin")
run("${consumer}" read "${work}/web.bin")
run("${CMAKE_COMMAND}" -E compare_files "${work}/native.bin" "${work}/web.bin")
finish_workspace("${work}")
