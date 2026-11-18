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

add_library(CocosCodegenModel STATIC
    src/generated/TypeGraphTypes.cpp
    src/model/TypeGraph.cpp
    src/model/TypeGraphBuilder.cpp
    src/parser/SpecParser.cpp)
target_compile_features(CocosCodegenModel PUBLIC cxx_std_20)
target_include_directories(CocosCodegenModel PUBLIC src)
set_target_properties(CocosCodegenModel PROPERTIES CXX_EXTENSIONS OFF)

add_executable(CocosCodegen
    src/cli/Main.cpp
    src/codegen/InterfaceEmitter.cpp
    src/workspace/Workspace.cpp)
target_compile_features(CocosCodegen PRIVATE cxx_std_20)
target_link_libraries(CocosCodegen PRIVATE CocosCodegenModel nlohmann_json::nlohmann_json)
set_target_properties(CocosCodegen PROPERTIES OUTPUT_NAME cocos-codegen CXX_EXTENSIONS OFF)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
