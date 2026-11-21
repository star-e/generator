add_executable(CocosTypeGraphTest TypeGraphTest.cpp)
target_link_libraries(CocosTypeGraphTest PRIVATE CocosCodegenModel)
add_test(NAME cocos.typegraph COMMAND CocosTypeGraphTest
    "${PROJECT_SOURCE_DIR}/spec")

add_test(NAME cocos.spec.documentation COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" -P "${CMAKE_CURRENT_SOURCE_DIR}/DocumentationTest.cmake")

add_test(NAME cocos.modules.imports COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/ModuleImports.cmake")

add_test(NAME cocos.workspace.outputs COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/WorkspaceOutputs.cmake")

add_test(NAME cocos.archive.generate COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/ArchiveTest.cmake")

add_test(NAME cocos.swig.generate COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" -P "${CMAKE_CURRENT_SOURCE_DIR}/SwigTest.cmake")

add_test(NAME cocos.settings.generate COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/SettingsTest.cmake")

add_test(NAME cocos.records.generate COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/RecordTest.cmake")

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/codegen-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:CocosCodegen>")
add_test(NAME cocos.selfhost COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:CocosCodegen>" ${self_host_options} -P "${CMAKE_CURRENT_SOURCE_DIR}/SelfHost.cmake")

if(COCOS_CODEGEN_ENGINE_ROOT)
    add_test(NAME cocos.swig.interop COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
        "-DENGINE=${COCOS_CODEGEN_ENGINE_ROOT}" -P "${CMAKE_CURRENT_SOURCE_DIR}/SwigInterop.cmake")
    add_test(NAME cocos.baseline COMMAND "${CMAKE_COMMAND}"
        "-DENGINE=${COCOS_CODEGEN_ENGINE_ROOT}" "-DSOURCE=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/Baseline.cmake")
    add_test(NAME cocos.settings.editor COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
        "-DENGINE=${COCOS_CODEGEN_ENGINE_ROOT}" -P "${CMAKE_CURRENT_SOURCE_DIR}/SettingsInterop.cmake")
    add_test(NAME cocos.records.interop COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
        "-DENGINE=${COCOS_CODEGEN_ENGINE_ROOT}" "-DGENERATOR=${CMAKE_GENERATOR}"
        "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}" "-DTOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DPREFIX=${CMAKE_PREFIX_PATH}" "-DTOOLCHAIN=${CMAKE_TOOLCHAIN_FILE}"
        "-DCONFIG=$<CONFIG>" -P "${CMAKE_CURRENT_SOURCE_DIR}/RecordInterop.cmake")
    add_test(NAME cocos.archive.interop COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:CocosCodegen>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
        "-DENGINE=${COCOS_CODEGEN_ENGINE_ROOT}" "-DGENERATOR=${CMAKE_GENERATOR}"
        "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}" "-DTOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DPREFIX=${CMAKE_PREFIX_PATH}" "-DTOOLCHAIN=${CMAKE_TOOLCHAIN_FILE}"
        "-DCONFIG=$<CONFIG>" -P "${CMAKE_CURRENT_SOURCE_DIR}/Interop.cmake")
endif()
