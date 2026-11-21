cmake_minimum_required(VERSION 3.24)
project(RenderCommonJsbConsumer LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the engine's SWIG headers, binding runtime and platform SDK with their normal build flags.
include("${ENGINE}/native/CMakeLists.txt")
set(stagedNative "${STAGING}/native")
set(records "${stagedNative}/cocos/renderer/pipeline/custom")
include("${CMAKE_CURRENT_LIST_DIR}/../../SwigTool.cmake")
render_swig_command(swig "${ENGINE}/native" "${stagedNative}")
set(config "${stagedNative}/tools/swig-config/render.i")
set(bindings "${CMAKE_CURRENT_BINARY_DIR}/staged-bindings")
set(wrapper "${bindings}/bindings/auto/jsb_render_auto")
add_custom_command(OUTPUT "${wrapper}.cpp" "${wrapper}.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${bindings}/bindings/auto"
    COMMAND ${swig} -o "${wrapper}.cpp" "${config}"
    DEPENDS "${config}" "${records}/RenderInterfaceTypes.h"
    VERBATIM)
# Both consumers link to the engine's native record implementations. The record
# interop target separately builds RenderCommonTypes.cpp from staging.
add_executable(JsbConsumer JsbConsumer.cpp "${records}/RenderCommonJsb.cpp")
add_executable(RenderNodeConsumer RenderNodeConsumer.cpp "${wrapper}.cpp" "${wrapper}.h" "${records}/RenderCommonJsb.cpp")
foreach(consumer IN ITEMS JsbConsumer RenderNodeConsumer)
    target_include_directories(${consumer} BEFORE PRIVATE "${stagedNative}" "${stagedNative}/cocos")
    target_link_libraries(${consumer} PRIVATE cocos_engine)
    add_dependencies(${consumer} genbindings)
endforeach()
target_include_directories(RenderNodeConsumer BEFORE PRIVATE "${bindings}")

if(WIN32)
    set(dlls ${WINDOWS_DLLS})
    foreach(dll IN LISTS V8_DLLS)
        get_filename_component(name "${dll}" NAME)
        list(APPEND dlls "${V8_DIR}/$<IF:$<CONFIG:Debug>,Debug,Release>/${name}")
    endforeach()
    add_custom_target(JsbRuntimeDlls
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:JsbConsumer>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${dlls} "$<TARGET_FILE_DIR:JsbConsumer>" VERBATIM)
    add_dependencies(JsbConsumer JsbRuntimeDlls)
    add_dependencies(RenderNodeConsumer JsbRuntimeDlls)
endif()
enable_testing()
add_test(NAME cocos.jsb.interop COMMAND JsbConsumer)
add_test(NAME cocos.jsb.render-node COMMAND RenderNodeConsumer)
