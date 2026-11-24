cmake_minimum_required(VERSION 3.24)
project(RenderCommonConsumer LANGUAGES CXX)
find_package(boost_container CONFIG REQUIRED)
set(native "${ENGINE}/native")
add_library(DescriptorTestGfx STATIC
    "${native}/cocos/base/RefCounted.cpp"
    "${native}/cocos/base/Log.cpp"
    "${native}/cocos/renderer/gfx-base/GFXBuffer.cpp"
    "${native}/cocos/renderer/gfx-base/GFXDef.cpp"
    "${native}/cocos/renderer/gfx-base/GFXDevice.cpp"
    "${native}/cocos/renderer/gfx-base/GFXTexture.cpp"
    "${native}/cocos/renderer/gfx-base/states/GFXBufferBarrier.cpp"
    "${native}/cocos/renderer/gfx-base/states/GFXGeneralBarrier.cpp"
    "${native}/cocos/renderer/gfx-base/states/GFXSampler.cpp"
    "${native}/cocos/renderer/gfx-base/states/GFXTextureBarrier.cpp"
    "${native}/cocos/renderer/gfx-base/GFXObject.cpp"
    "${native}/cocos/renderer/gfx-base/GFXDescriptorSet.cpp"
    "${native}/cocos/renderer/gfx-base/GFXDescriptorSetLayout.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyDescriptorSet.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyDescriptorSetLayout.cpp")
add_executable(RecordConsumer RecordConsumer.cpp ForwardConsumer.cpp LayoutGraphConsumer.cpp LayoutNamesConsumer.cpp GraphDFSConsumer.cpp LayoutArchiveConsumer.cpp PipelineArchiveConsumer.cpp
    "${native}/cocos/renderer/gfx-base/GFXFramebuffer.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyFramebuffer.cpp"
    "${native}/cocos/renderer/gfx-base/GFXRenderPass.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyRenderPass.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyBuffer.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyTexture.cpp"
    "${native}/cocos/renderer/gfx-base/GFXPipelineLayout.cpp"
    "${native}/cocos/renderer/gfx-empty/EmptyPipelineLayout.cpp"
    "${STAGING}/native/ShiftProbe.cpp"
    "${STAGING}/native/ShiftSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/ArchiveTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderInterfaceTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphTypes.cpp"
    "${STAGING}/native/IndexedKeys.cpp"
    "${STAGING}/native/IndexedKeysSerialization.cpp"
    "${STAGING}/DispatcherQueries.cpp" "${STAGING}/CurrentLayoutUtils.cpp"
    DispatcherGraphConsumer.cpp
    MigrationConsumer.cpp
    RenderQueueConsumer.cpp
    "${STAGING}/NativeQueueSort.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/NativeTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/FGDispatcherTypes.cpp"
    "${STAGING}/NativeData.cpp"
    ResourceGraphConsumer.cpp SubpassGraphConsumer.cpp RasterPassConsumer.cpp PassConsumer.cpp RenderDataConsumer.cpp RenderGraphConsumer.cpp
    "${STAGING}/native/cocos/renderer/pipeline/custom/NativeResourceGraph.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/NativePools.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderGraphTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderCommonSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderCommonTypes.cpp")
set(legacyNative "${LEGACY_ENGINE}/native")
set(custom "${legacyNative}/cocos/renderer/pipeline/custom")
add_executable(LegacyArchiveConsumer LegacyArchiveConsumer.cpp "${STAGING}/LegacyLayoutUtils.cpp"
    "${custom}/ArchiveTypes.cpp" "${custom}/RenderCommonTypes.cpp" "${custom}/LayoutGraphTypes.cpp"
    "${custom}/LayoutGraphSerialization.cpp")
target_compile_definitions(LegacyArchiveConsumer PRIVATE CC_TEST_LEGACY_LAYOUT=1)
target_include_directories(LegacyArchiveConsumer PRIVATE "${legacyNative}" "${legacyNative}/cocos" "${legacyNative}/cocos/renderer")
# NativeData.cpp inherits the adjacent generated header's include directory.
set_property(SOURCE "${STAGING}/NativeData.cpp" PROPERTY INCLUDE_DIRECTORIES "${STAGING}/native/cocos/renderer/pipeline/custom")
set_property(SOURCE "${STAGING}/native/cocos/renderer/pipeline/custom/NativePools.cpp" PROPERTY INCLUDE_DIRECTORIES "${native}/cocos/renderer/pipeline/custom")
add_library(NativeLibraryConstructor STATIC "${STAGING}/NativeLibraryConstructor.cpp")
target_link_libraries(RecordConsumer PRIVATE NativeLibraryConstructor)
# Material and Model transitively include the engine's native typed array headers.
foreach(target IN ITEMS RecordConsumer NativeLibraryConstructor)
    target_compile_definitions(${target} PRIVATE "$<$<PLATFORM_ID:Windows>:_USE_MATH_DEFINES>")
    target_include_directories(${target} PRIVATE "${STAGING}/native"
        "$<$<PLATFORM_ID:Windows>:${native}/external/win64/include/v8>"
        "$<$<PLATFORM_ID:Darwin>:${native}/external/mac/include/v8>"
        "$<$<PLATFORM_ID:Linux>:${native}/external/linux/include/v8>")
endforeach()
get_target_property(containerType Boost::container TYPE)
if(WIN32 AND containerType STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET RecordConsumer POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "$<TARGET_FILE:Boost::container>" "$<TARGET_FILE_DIR:RecordConsumer>")
endif()
foreach(target IN ITEMS RecordConsumer LegacyArchiveConsumer)
    target_sources(${target} PRIVATE LayoutPrintConsumer.cpp LayoutDescriptorConsumer.cpp)
    target_link_libraries(${target} PRIVATE DescriptorTestGfx)
endforeach()
foreach(target IN ITEMS RecordConsumer LegacyArchiveConsumer DescriptorTestGfx NativeLibraryConstructor)
    target_link_libraries(${target} PRIVATE Boost::container)
    set_target_properties(${target} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    target_include_directories(${target} PRIVATE "${native}" "${native}/cocos"
        "${native}/cocos/renderer" "${native}/external/sources")
    target_compile_definitions(${target} PRIVATE BOOST_ALL_NO_LIB=1 CC_STATIC=1
        CC_PLATFORM_WINDOWS=2 CC_PLATFORM_MACOS=4 CC_PLATFORM_LINUX=6
        "$<$<PLATFORM_ID:Windows>:CC_PLATFORM=CC_PLATFORM_WINDOWS>"
        "$<$<PLATFORM_ID:Darwin>:CC_PLATFORM=CC_PLATFORM_MACOS>"
        "$<$<PLATFORM_ID:Linux>:CC_PLATFORM=CC_PLATFORM_LINUX>")
endforeach()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/consumer-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:RecordConsumer>")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/legacy-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:LegacyArchiveConsumer>")
