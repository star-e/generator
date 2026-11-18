include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
set(stage "${work}/engine")
# Compile a uint32_t high bit alongside the production enums in both consumer languages.
file(COPY "${SOURCE}/spec" "${SOURCE}/config" DESTINATION "${work}")
file(WRITE "${work}/spec/shift.spec" "module ShiftProbe; namespace cc {
    flags ShiftProbe : uint32_t { HIGH = 1 << 31 }
    @moveOnly struct MoveProbe { value: uint32_t = 0; }
}")
set(workspace "${work}/config/workspace.json")
file(READ "${workspace}" config)
string(JSON count LENGTH "${config}" modules)
string(JSON config SET "${config}" modules ${count} [[{
    "backend": "records", "spec": "../spec/shift.spec",
    "header": "native/ShiftProbe.h", "typescript": "cocos/rendering/custom/shift-probe.ts"
}]])
file(WRITE "${workspace}" "${config}")
run("${TOOL}" generate --workspace "${workspace}" --output "${stage}")
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
# Isolate the engine's exported enum without loading its graphics device at runtime.
file(READ "${ENGINE}/cocos/gfx/base/define.ts" gfx)
string(REGEX MATCH "export enum ResolveMode \\{[^}]+\\}" mode "${gfx}")
if(NOT mode)
    message(FATAL_ERROR "Missing ResolveMode in engine gfx definitions")
endif()
file(MAKE_DIRECTORY "${stage}/cocos/gfx")
file(WRITE "${stage}/cocos/gfx/index.ts" "${mode}\n")
set(names "cocos/rendering/custom/types-names.ts")
run("${CMAKE_COMMAND}" -E compare_files "${stage}/${names}" "${ENGINE}/${names}")
set(enumConsumer "${stage}/cocos/rendering/custom/enum-consumer.ts")
configure_file("${SOURCE}/tests/consumer/records/enum-consumer.ts" "${enumConsumer}" COPYONLY)

# Compile both versions against the same engine imports to exercise the projected defaults at runtime.
set(commonSpec "${work}/spec/render/render-common.spec")
file(READ "${commonSpec}" common)
string(REPLACE "numRenderPasses: uint32_t = 0" "numRenderPasses: uint32_t = 7" common "${common}")
file(WRITE "${commonSpec}" "${common}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/variant")
set(variant "${stage}/cocos/rendering/custom/types-variant.ts")
configure_file("${work}/variant/cocos/rendering/custom/types.ts" "${variant}" COPYONLY)
run("${node}" "${ENGINE}/node_modules/typescript/bin/tsc" --strict --skipLibCheck --target es6 --module commonjs
    --outDir "${work}/js" --rootDir "${stage}/cocos" ${declarations} "${stage}/${names}" "${enumConsumer}" "${variant}")
run("${node}" "${SOURCE}/tests/consumer/records/consumer.cjs" "${work}/js/rendering/custom" "${stage}" "${ENGINE}")
finish_workspace("${work}")
