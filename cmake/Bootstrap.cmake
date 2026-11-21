cmake_minimum_required(VERSION 3.24)

cmake_path(GET BOOTSTRAP PARENT_PATH bootstrap_dir)
file(MAKE_DIRECTORY "${bootstrap_dir}")
file(LOCK "${bootstrap_dir}/update.lock" GUARD PROCESS)

if(DEFINED TOOL)
    # Verify the exact executable that will be published. Keep A available on failure.
    cmake_path(GET BOOTSTRAP FILENAME name)
    set(staged_tool "${bootstrap_dir}/pending/${name}")
    file(MAKE_DIRECTORY "${bootstrap_dir}/pending")
    file(COPY_FILE "${TOOL}" "${staged_tool}")
    set(TOOL "${staged_tool}")
    include("${SOURCE}/tests/SelfHost.cmake")
    file(RENAME "${staged_tool}" "${BOOTSTRAP}")
    message(STATUS "Saved bootstrap generator: ${BOOTSTRAP}")
else()
    if(NOT EXISTS "${BOOTSTRAP}")
        message(FATAL_ERROR "No saved generator at ${BOOTSTRAP}. Run the begin-codegen-update target first.")
    endif()
    execute_process(COMMAND "${BOOTSTRAP}" generate --workspace "${SOURCE}/config/self-host.json"
        --output "${SOURCE}/src" COMMAND_ERROR_IS_FATAL ANY)
endif()
