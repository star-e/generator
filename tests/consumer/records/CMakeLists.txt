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
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphTypes.cpp"
    ResourceGraphConsumer.cpp SubpassGraphConsumer.cpp RasterPassConsumer.cpp PassConsumer.cpp RenderDataConsumer.cpp
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderGraphTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphPrint.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphDescriptorSet.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderCommonSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderCommonTypes.cpp")
set(custom "${native}/cocos/renderer/pipeline/custom")
add_executable(LegacyArchiveConsumer LegacyArchiveConsumer.cpp "${STAGING}/LegacyLayoutDescriptorSet.cpp"
    "${STAGING}/LegacyLayoutGraphPrint.cpp"
    "${custom}/ArchiveTypes.cpp" "${custom}/RenderCommonTypes.cpp" "${custom}/LayoutGraphTypes.cpp"
    "${custom}/LayoutGraphSerialization.cpp")
target_compile_definitions(LegacyArchiveConsumer PRIVATE CC_TEST_LEGACY_LAYOUT=1)
# Material and Model transitively include the engine's native typed array headers.
target_include_directories(RecordConsumer PRIVATE "${STAGING}/native"
    "$<$<PLATFORM_ID:Windows>:${native}/external/win64/include/v8>"
    "$<$<PLATFORM_ID:Darwin>:${native}/external/mac/include/v8>"
    "$<$<PLATFORM_ID:Linux>:${native}/external/linux/include/v8>")
get_target_property(containerType Boost::container TYPE)
if(WIN32 AND containerType STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET RecordConsumer POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "$<TARGET_FILE:Boost::container>" "$<TARGET_FILE_DIR:RecordConsumer>")
endif()
# Both readers use the same printer declaration but resolve their graph and archive types separately.
set_property(SOURCE LayoutPrintConsumer.cpp PROPERTY INCLUDE_DIRECTORIES "${STAGING}/native/cocos/renderer/pipeline/custom")
foreach(target IN ITEMS RecordConsumer LegacyArchiveConsumer)
    target_sources(${target} PRIVATE LayoutPrintConsumer.cpp LayoutDescriptorConsumer.cpp "${STAGING}/DescriptorTypeMapping.cpp")
    target_link_libraries(${target} PRIVATE DescriptorTestGfx)
endforeach()
foreach(target IN ITEMS RecordConsumer LegacyArchiveConsumer DescriptorTestGfx)
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
