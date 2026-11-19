cmake_minimum_required(VERSION 3.24)
project(RenderCommonConsumer LANGUAGES CXX)
find_package(boost_container CONFIG REQUIRED)
set(native "${ENGINE}/native")
add_executable(RecordConsumer RecordConsumer.cpp ForwardConsumer.cpp LayoutGraphConsumer.cpp GraphDFSConsumer.cpp "${native}/cocos/base/RefCounted.cpp"
    "${STAGING}/native/ShiftProbe.cpp"
    "${STAGING}/native/ShiftSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/ArchiveTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/LayoutGraphTypes.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderCommonSerialization.cpp"
    "${STAGING}/native/cocos/renderer/pipeline/custom/RenderCommonTypes.cpp")
target_link_libraries(RecordConsumer PRIVATE Boost::container)
get_target_property(containerType Boost::container TYPE)
if(WIN32 AND containerType STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET RecordConsumer POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "$<TARGET_FILE:Boost::container>" "$<TARGET_FILE_DIR:RecordConsumer>")
endif()
set_target_properties(RecordConsumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(RecordConsumer PRIVATE "${STAGING}/native" "${native}" "${native}/cocos"
    "${native}/cocos/renderer" "${native}/external/sources")
target_compile_definitions(RecordConsumer PRIVATE BOOST_ALL_NO_LIB=1 CC_STATIC=1
    CC_PLATFORM_WINDOWS=2 CC_PLATFORM_MACOS=4 CC_PLATFORM_LINUX=6
    "$<$<PLATFORM_ID:Windows>:CC_PLATFORM=CC_PLATFORM_WINDOWS>"
    "$<$<PLATFORM_ID:Darwin>:CC_PLATFORM=CC_PLATFORM_MACOS>"
    "$<$<PLATFORM_ID:Linux>:CC_PLATFORM=CC_PLATFORM_LINUX>")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/consumer-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:RecordConsumer>")
