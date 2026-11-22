cmake_minimum_required(VERSION 3.24)
project(CocosCodegen LANGUAGES CXX)

find_package(nlohmann_json 3.12.0 EXACT CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

set(COCOS_CODEGEN_SELF_HOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH
    "Root containing the generated self-hosted model files")
include(cmake/SelfHostManifest.cmake)
set(self_host_manifest "${COCOS_CODEGEN_SELF_HOST_ROOT}/codegen-generated-manifest.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${self_host_manifest}")
read_self_host_paths("${self_host_manifest}" self_host_sources)
list(FILTER self_host_sources INCLUDE REGEX "[.](cpp|h)$")
list(TRANSFORM self_host_sources PREPEND "${COCOS_CODEGEN_SELF_HOST_ROOT}/")

add_library(CocosCodegenModel STATIC
    ${self_host_sources}
    src/model/TypeGraph.cpp
    src/model/TypeGraphBuilder.cpp
    src/parser/SpecParser.cpp)
target_compile_features(CocosCodegenModel PUBLIC cxx_std_20)
target_include_directories(CocosCodegenModel PUBLIC "${COCOS_CODEGEN_SELF_HOST_ROOT}" src)
set_target_properties(CocosCodegenModel PROPERTIES CXX_EXTENSIONS OFF)

add_executable(CocosCodegen
    src/cli/Main.cpp
    src/codegen/ArchiveEmitter.cpp
    src/codegen/GraphEmitter.cpp
    src/codegen/TypeScriptGraphEmitter.cpp
    src/codegen/InterfaceEmitter.cpp
    src/codegen/JsbEmitter.cpp
    src/codegen/RecordEmitter.cpp
    src/codegen/SettingsEmitter.cpp
    src/codegen/SwigEmitter.cpp
    src/workspace/Workspace.cpp)
target_compile_features(CocosCodegen PRIVATE cxx_std_20)
target_link_libraries(CocosCodegen PRIVATE CocosCodegenModel nlohmann_json::nlohmann_json)
set_target_properties(CocosCodegen PROPERTIES OUTPUT_NAME cocos-codegen CXX_EXTENSIONS OFF)

set(COCOS_CODEGEN_ENGINE_ROOT "" CACHE PATH "Engine checkout for baseline and consumer tests")
# Keep a multi-directory search path in a single -D argument when expanding the options.
string(REPLACE ";" "\\;" self_host_prefix "${CMAKE_PREFIX_PATH}")
set(self_host_options
    "-DSOURCE=${PROJECT_SOURCE_DIR}" "-DGENERATOR=${CMAKE_GENERATOR}"
    "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}" "-DTOOLSET=${CMAKE_GENERATOR_TOOLSET}"
    "-DINSTANCE=${CMAKE_GENERATOR_INSTANCE}" "-DTOOLCHAIN=${CMAKE_TOOLCHAIN_FILE}"
    "-DCOMPILER=${CMAKE_CXX_COMPILER}" "-DPREFIX=${self_host_prefix}"
    "-DJSON_DIR=${nlohmann_json_DIR}" "-DJSON_SOURCE=${nlohmann_json_SOURCE_DIR}"
    "-DENGINE=${COCOS_CODEGEN_ENGINE_ROOT}" "-DCONFIG=$<CONFIG>")
set(bootstrap "${PROJECT_SOURCE_DIR}/build/bootstrap/cocos-codegen${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_target(begin-codegen-update
    COMMAND "${CMAKE_COMMAND}" "-DBOOTSTRAP=${bootstrap}" "-DTOOL=$<TARGET_FILE:CocosCodegen>"
        ${self_host_options} -P "${PROJECT_SOURCE_DIR}/cmake/Bootstrap.cmake"
    DEPENDS CocosCodegen
    COMMENT "Verify and save the generator for this update"
    USES_TERMINAL VERBATIM)
add_custom_target(generate-selfhost
    COMMAND "${CMAKE_COMMAND}" "-DBOOTSTRAP=${bootstrap}" "-DSOURCE=${PROJECT_SOURCE_DIR}"
        -P "${PROJECT_SOURCE_DIR}/cmake/Bootstrap.cmake"
    COMMENT "Regenerate the self-hosted sources with the saved generator"
    USES_TERMINAL VERBATIM)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
