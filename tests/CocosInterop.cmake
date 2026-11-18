include("${CMAKE_CURRENT_LIST_DIR}/CocosTestSupport.cmake")
test_workspace(work)
set(stage "${work}/engine")
run("${TOOL}" generate --workspace "${SOURCE}/config/cocos-workspace.json" --output "${stage}")
foreach(path IN ITEMS native/cocos/renderer/pipeline/custom/BinaryArchive.h
                      native/cocos/base/std/container/string.h cocos/rendering/custom/binary-archive.ts)
    configure_file("${ENGINE}/${path}" "${stage}/${path}" COPYONLY)
endforeach()

set(build "${work}/build")
set(args -G "${GENERATOR}")
if(PLATFORM)
    list(APPEND args -A "${PLATFORM}")
endif()
if(TOOLSET)
    list(APPEND args -T "${TOOLSET}")
endif()
if(TOOLCHAIN)
    list(APPEND args "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}")
endif()
if(CONFIG)
    set(configArg --config "${CONFIG}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SOURCE}/tests/consumer" -B "${build}"
    ${args} "-DSTAGING=${stage}" "-DCMAKE_PREFIX_PATH=${PREFIX}" "-DCMAKE_BUILD_TYPE=${CONFIG}"
    COMMAND_ERROR_IS_FATAL ANY)
run("${CMAKE_COMMAND}" --build "${build}" ${configArg})
file(READ "${build}/consumer-${CONFIG}.txt" consumer)

find_program(node node REQUIRED)
set(ts "${stage}/cocos/rendering/custom")
run("${node}" "${ENGINE}/node_modules/typescript/bin/tsc" --target es6 --module commonjs
    --strict --outDir "${work}/js" "${ts}/archive.ts" "${ts}/binary-archive.ts")
run("${consumer}" write "${work}/native.bin")
run("${node}" "${SOURCE}/tests/consumer/cocos-archive-consumer.cjs"
    "${work}/js/binary-archive.js" "${work}/native.bin" "${work}/web.bin")
run("${consumer}" read "${work}/web.bin")
run("${CMAKE_COMMAND}" -E compare_files "${work}/native.bin" "${work}/web.bin")
finish_workspace("${work}")
