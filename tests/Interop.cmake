include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
set(stage "${work}/engine")
run("${TOOL}" generate --workspace "${SOURCE}/config/workspace.json" --output "${stage}")
foreach(path IN ITEMS native/cocos/renderer/pipeline/custom/BinaryArchive.h
                      native/cocos/base/std/container/string.h cocos/rendering/custom/binary-archive.ts)
    configure_file("${ENGINE}/${path}" "${stage}/${path}" COPYONLY)
endforeach()

set(build "${work}/build")
build_consumer("${SOURCE}/tests/consumer" "${build}" "${stage}")
file(READ "${build}/consumer-${CONFIG}.txt" consumer)

find_program(node node REQUIRED)
set(ts "${stage}/cocos/rendering/custom")
run("${node}" "${ENGINE}/node_modules/typescript/bin/tsc" --target es6 --module commonjs
    --strict --outDir "${work}/js" "${ts}/archive.ts" "${ts}/binary-archive.ts")
run("${consumer}" write "${work}/native.bin")
run("${node}" "${SOURCE}/tests/consumer/archive-consumer.cjs"
    "${work}/js/binary-archive.js" "${work}/native.bin" "${work}/web.bin")
run("${consumer}" read "${work}/web.bin")
run("${CMAKE_COMMAND}" -E compare_files "${work}/native.bin" "${work}/web.bin")
finish_workspace("${work}")
