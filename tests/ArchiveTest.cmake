include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(COPY "${SOURCE}/spec" "${SOURCE}/config" "${SOURCE}/runtime" DESTINATION "${work}")
set(output "${work}/staging")
set(generate "${TOOL}" generate --workspace "${work}/config/workspace.json" --output "${output}")

expect_failure(1 "missing:" ${generate} --check)
if(EXISTS "${output}")
    message(FATAL_ERROR "--check created the output directory")
endif()
run(${generate})
file(READ "${SOURCE}/config/baseline.json" baseline)
file(GLOB_RECURSE files RELATIVE "${output}" "${output}/*")
set(baselineFiles
    cocos/rendering/custom/archive.ts
    cocos/rendering/custom/layout-graph-names.ts
    cocos/rendering/custom/types-names.ts
    editor/assets/default_renderpipeline/builtin-pipeline-types.ts
    native/cocos/renderer/pipeline/custom/CustomTypes.cpp
    native/cocos/renderer/pipeline/custom/PrivateTypes.cpp
    native/cocos/renderer/pipeline/custom/RenderInterfaceTypes.cpp
    native/tools/swig-config/render.i
    native/cocos/renderer/pipeline/custom/ArchiveFwd.h
    native/cocos/renderer/pipeline/custom/ArchiveTypes.cpp
    native/cocos/renderer/pipeline/custom/ArchiveTypes.h
    native/cocos/renderer/pipeline/custom/RenderCommonFwd.h
    native/cocos/renderer/pipeline/custom/RenderCommonJsb.cpp
    native/cocos/renderer/pipeline/custom/RenderCommonJsb.h
    native/cocos/renderer/pipeline/custom/RenderCommonNames.h
    native/cocos/renderer/pipeline/custom/RenderCommonSerialization.cpp
    native/cocos/renderer/pipeline/custom/RenderCommonSerialization.h)
set(expectedFiles ${baselineFiles} cocos/rendering/custom/types.ts cocos/rendering/custom/layout-graph.ts
    cocos/rendering/custom/render-graph.ts
    cocos/rendering/custom/private.ts
    cocos/rendering/custom/pipeline.ts
    cocos/rendering/custom/web-types.ts
    native/cocos/renderer/pipeline/custom/Array4.h
    native/cocos/renderer/pipeline/custom/CustomFwd.h
    native/cocos/renderer/pipeline/custom/CustomTypes.h
    native/cocos/renderer/pipeline/custom/FGDispatcherFwd.h
    native/cocos/renderer/pipeline/custom/FGDispatcherTypes.cpp
    native/cocos/renderer/pipeline/custom/FGDispatcherTypes.h
    native/cocos/renderer/pipeline/custom/NativeFwd.h
    native/cocos/renderer/pipeline/custom/NativePipelineTypes.cpp
    native/cocos/renderer/pipeline/custom/NativePipelineTypes.h
    native/cocos/renderer/pipeline/custom/NativePipelineFwd.h
    native/cocos/renderer/pipeline/custom/NativeTypes.cpp
    native/cocos/renderer/pipeline/custom/NativeTypes.h
    native/cocos/renderer/pipeline/custom/PrivateFwd.h
    native/cocos/renderer/pipeline/custom/PrivateTypes.h
    native/cocos/renderer/pipeline/custom/RenderInterfaceFwd.h
    native/cocos/renderer/pipeline/custom/RenderInterfaceTypes.h
    native/cocos/renderer/pipeline/custom/RenderGraphFwd.h
    native/cocos/renderer/pipeline/custom/RenderGraphTypes.h native/cocos/renderer/pipeline/custom/RenderGraphTypes.cpp
    native/cocos/base/GraphDFS.h
    native/cocos/base/GraphDescriptor.h
    native/cocos/base/GraphIterator.h
    native/cocos/base/GraphView.h
    native/cocos/base/GraphEdges.h
    native/cocos/renderer/pipeline/custom/LayoutGraphFwd.h
    native/cocos/renderer/pipeline/custom/LayoutGraphNames.h
    native/cocos/renderer/pipeline/custom/LayoutGraphSerialization.h native/cocos/renderer/pipeline/custom/LayoutGraphSerialization.cpp
    native/cocos/renderer/pipeline/custom/LayoutGraphTypes.h native/cocos/renderer/pipeline/custom/LayoutGraphTypes.cpp
    native/cocos/renderer/pipeline/custom/RenderCommonTypes.h native/cocos/renderer/pipeline/custom/RenderCommonTypes.cpp)
set(manifestName "codegen-generated-manifest.json")
file(READ "${output}/${manifestName}" manifest)
string(JSON version GET "${manifest}" schemaVersion)
string(JSON count LENGTH "${manifest}" files)
list(LENGTH expectedFiles expectedCount)
if(NOT version STREQUAL "1" OR NOT count EQUAL expectedCount)
    message(FATAL_ERROR "Unexpected engine manifest version ${version} or file count ${count}")
endif()
foreach(path IN LISTS expectedFiles)
    string(JSON source GET "${manifest}" files "${path}" source)
    string(JSON module GET "${manifest}" files "${path}" module)
    if(NOT source MATCHES "^spec/.*[.]spec$")
        message(FATAL_ERROR "${path}: invalid manifest source '${source}'")
    endif()
    file(STRINGS "${work}/${source}" declaration REGEX "(^| )module ${module};" LIMIT_COUNT 1)
    if(NOT declaration)
        message(FATAL_ERROR "${path}: module ${module} not declared in ${source}")
    endif()
endforeach()
file(READ "${work}/config/workspace.json" config)
string(JSON moduleCount LENGTH "${config}" modules)
math(EXPR lastModule "${moduleCount} - 1")
foreach(i RANGE 0 ${lastModule})
    string(JSON copies ERROR_VARIABLE missing GET "${config}" modules ${i} runtimeFiles)
    if(missing)
        continue()
    endif()
    string(JSON spec GET "${config}" modules ${i} spec)
    string(JSON n LENGTH "${copies}")
    set(j 0)
    while(j LESS n)
        string(JSON path MEMBER "${copies}" ${j})
        string(JSON input GET "${copies}" "${path}")
        string(JSON copiedFrom GET "${manifest}" files "${path}" copiedFrom)
        string(JSON source GET "${manifest}" files "${path}" source)
        if(NOT "../${copiedFrom}" STREQUAL input OR NOT "../${source}" STREQUAL spec)
            message(FATAL_ERROR "${path}: incorrect runtime provenance ${source}, ${copiedFrom}")
        endif()
        math(EXPR j "${j} + 1")
    endwhile()
endforeach()
list(APPEND expectedFiles "${manifestName}")
list(SORT expectedFiles)
if(NOT files STREQUAL expectedFiles)
    message(FATAL_ERROR "Unexpected generated paths: ${files}")
endif()
foreach(path IN LISTS baselineFiles)
    string(JSON expected GET "${baseline}" files "${path}")
    file(READ "${output}/${path}" historical)
    if(path STREQUAL "native/tools/swig-config/render.i")
        # The binding now includes the definition used by its native type conversions directly.
        string(REPLACE "#include \"renderer/pipeline/PipelineSceneData.h\"\n" "" historical "${historical}")
    endif()
    if(path MATCHES "/ArchiveTypes[.]h$")
        # The old header included memory_resource even though scratch() only returns a pointer.
        string(REPLACE "\nnamespace boost::container::pmr {\nclass memory_resource;\n}\n" "" historical "${historical}")
        string(REPLACE "#include <string_view>" "#include <boost/container/pmr/memory_resource.hpp>\n#include <string_view>" historical "${historical}")
    endif()
    # The historical RenderCommon headers predate the shared tags and the ArchiveInterface import.
    if(path MATCHES "/RenderCommon(Fwd|Names)[.]h$")
        if(path MATCHES "Fwd[.]h$")
            set(sharedTags "struct Name_;\nstruct Layout_;\n")
        else()
            string(CONCAT sharedTags
                "inline const char* getName(const Name_& /*v*/) noexcept { return \"Name\"; }\n"
                "inline const char* getName(const Layout_& /*v*/) noexcept { return \"Layout\"; }\n")
        endif()
        string(FIND "${historical}" "${sharedTags}" offset)
        if(offset EQUAL -1)
            message(FATAL_ERROR "Missing shared graph tags in ${path}")
        endif()
        string(REPLACE "${sharedTags}" "" historical "${historical}")
        if(path MATCHES "Fwd[.]h$")
            string(REPLACE "#include \"cocos/renderer/pipeline/custom/ArchiveFwd.h\"\n" "" historical "${historical}")
        endif()
    endif()
    # The historical hashes were recorded from Windows text output.
    string(REPLACE "\n" "\r\n" historical "${historical}")
    string(SHA256 actual "${historical}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Baseline mismatch: ${path} (${actual})")
    endif()
endforeach()
foreach(path IN LISTS files)
    check_line_endings("${output}/${path}" native)
    file(TIMESTAMP "${output}/${path}" stamp "%s.%f" UTC)
    list(APPEND before "${stamp}")
endforeach()
run(${generate})
run(${generate} --check --diff)
foreach(path IN LISTS files)
    file(TIMESTAMP "${output}/${path}" stamp "%s.%f" UTC)
    list(APPEND after "${stamp}")
endforeach()
if(NOT before STREQUAL after)
    message(FATAL_ERROR "Repeated generation changed timestamps")
endif()
file(REMOVE "${output}/${manifestName}")
expect_failure(1 "missing: codegen-generated-manifest[.]json" ${generate} --check)
if(EXISTS "${output}/${manifestName}")
    message(FATAL_ERROR "--check recreated the missing manifest")
endif()
file(WRITE "${output}/${manifestName}" "{}")
expect_failure(1 "changed: codegen-generated-manifest[.]json" ${generate} --check --diff)
run(${generate})
file(READ "${output}/${manifestName}" restored)
if(NOT restored STREQUAL manifest)
    message(FATAL_ERROR "Generation did not restore the original manifest")
endif()

file(APPEND "${work}/runtime/cpp/GraphDFS.h" "// runtime source edit\n")
expect_failure(1 "changed: native/cocos/base/GraphDFS[.]h" ${generate} --check --diff)
run(${generate})
run(${generate} --check --diff)

set(ts "${output}/cocos/rendering/custom/archive.ts")
file(APPEND "${ts}" "// edited\n")
file(SHA256 "${ts}" beforeCheck)
expect_failure(1 "first mismatch at byte" ${generate} --check --diff)
file(SHA256 "${ts}" afterCheck)
if(NOT beforeCheck STREQUAL afterCheck)
    message(FATAL_ERROR "--check modified the file")
endif()
run(${generate})
file(WRITE "${output}/obsolete.ts" "")
expect_failure(1 "unexpected: obsolete.ts" ${generate} --check)

set(spec "${work}/spec/render/archive.spec")
file(READ "${spec}" original)
set(workspace "${work}/config/workspace.json")
file(READ "${workspace}" config)
string(REPLACE "native/cocos/base/GraphDFS.h" "Codegen-Generated-Manifest.json" reserved "${config}")
file(WRITE "${workspace}" "${reserved}")
expect_failure(2 "reserved manifest output" ${generate})
string(REPLACE "native/cocos/base/GraphDFS.h" "cocos/rendering/custom/Layout-Graph.ts" collision "${config}")
file(WRITE "${workspace}" "${collision}")
expect_failure(2 "duplicate output cocos/rendering/custom/layout-graph[.]ts" ${generate})
string(JSON old SET "${config}" modules 0 projection [["unused.json"]])
file(WRITE "${workspace}" "${old}")
expect_failure(2 "move JSON projection rules into .spec binding blocks" ${generate})
file(WRITE "${workspace}" "${config}")
string(JSON invalid SET "${config}" modules 0 cpp "{}")
file(WRITE "${workspace}" "${invalid}")
expect_failure(2 "extern module cannot declare generated outputs" ${generate})
file(WRITE "${workspace}" "${config}")
string(REPLACE "method writeBool => b;" "method scratch => s;" conflict "${original}")
file(WRITE "${spec}" "${conflict}")
expect_failure(2 "archive[.]spec:[0-9]+:[0-9]+: cannot assign a TS name to @cppOnly method OutputArchive[.]scratch" ${generate})
string(REPLACE "method writeBool => b;" "method missing => b;" broken "${original}")
file(WRITE "${spec}" "${broken}")
expect_failure(2 "archive[.]spec:[0-9]+:[0-9]+: unknown method missing" ${generate})
string(REPLACE "method writeBool => b;" "method writeBool => n;" broken "${original}")
file(WRITE "${spec}" "${broken}")
expect_failure(2 "duplicate TS method OutputArchive.n" ${generate})
string(REPLACE "bool value" "double enabled" changed "${original}")
string(REPLACE "module ArchiveInterface;" "module ArchiveInterface; import Containers;" changed "${changed}")
set(extraMethods [=[interface InputArchive {
        void nativeOnly();
        bool readBool(bool fallback) { return fallback; }
        static void inspect() {
            const char close = '}'; // }
            const auto text = R"cpp("} /*)cpp"; /* } */
            if (close == text[1]) { (void)1'000; }
        }
        virtual ccstd::vector<uint8_t> copyBytes(ccstd::vector<uint8_t> value) = 0;]=])
string(REPLACE "interface InputArchive {" "${extraMethods}" changed "${changed}")
file(WRITE "${spec}" "${changed}")
run(${generate})
file(READ "${ts}" emitted)
if(NOT emitted MATCHES "b \\(enabled: number\\)")
    message(FATAL_ERROR "TS output did not follow the declaration")
endif()
string(FIND "${emitted}" "copyBytes (value: Uint8Array): Uint8Array;" at)
file(READ "${output}/native/cocos/renderer/pipeline/custom/ArchiveTypes.h" header)
if(at EQUAL -1 OR NOT header MATCHES "ccstd::vector<uint8_t> copyBytes[(]ccstd::vector<uint8_t> value[)]"
   OR NOT header MATCHES "#include <cstdint>" OR NOT header MATCHES "#include <cocos/base/std/container/vector.h>")
    message(FATAL_ERROR "Interface signature or native includes did not follow the concrete template instance")
endif()
string(FIND "${header}" [=[const auto text = R"cpp("} /*)cpp"; /* } */]=] quoted)
if(quoted EQUAL -1 OR NOT header MATCHES "void nativeOnly[(][)][;]" OR header MATCHES "virtual void nativeOnly"
   OR emitted MATCHES "nativeOnly|inspect|fallback")
    message(FATAL_ERROR "Native method body, declaration or visibility changed")
endif()
string(REPLACE "virtual bool readBool() = 0;" "virtual bool readBool();" broken "${changed}")
file(WRITE "${spec}" "${broken}")
expect_failure(2 "expected '='" ${generate})
string(REPLACE "virtual bool readBool() = 0;" "bool readBool() = 0;" broken "${changed}")
file(WRITE "${spec}" "${broken}")
expect_failure(2 "pure virtual method requires 'virtual'" ${generate})
string(REPLACE "double enabled" "Missing enabled" broken "${changed}")
file(WRITE "${spec}" "${broken}")
expect_failure(2 "archive.spec:5:[0-9]+: unknown type Missing" ${generate})
finish_workspace("${work}")
