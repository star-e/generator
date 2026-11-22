include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(WRITE "${work}/provider.spec" "module Provider; namespace cc { struct Item {} }")
file(WRITE "${work}/client.spec" "module View; import Provider; namespace cc { struct Data { item: Item; } }")
file(WRITE "${work}/model.spec" "module Model;")
file(WRITE "${work}/preamble.txt" "")
file(WRITE "${work}/runtime.ts" "// runtime\n")
set(config [=[{
    "outputRoot": "staging", "lineEnding": "lf", "preamble": "preamble.txt", "manifest": {},
    "modules": [
        {"spec":"client.spec", "typescript":"api/view.ts", "runtimeFiles":{"api/support.ts":"runtime.ts"}},
        {"spec":"provider.spec", "typescript":"other/provider.ts"},
        {"spec":"model.spec"}
    ]
}]=])
set(workspace "${work}/workspace.json")
file(WRITE "${workspace}" "${config}")
set(generate "${TOOL}" generate --workspace "${workspace}")
run(${generate})

# The engine contains just the selected module's payloads alongside handwritten code.
set(engine "${work}/engine")
file(COPY "${work}/staging/api" DESTINATION "${engine}")
file(WRITE "${engine}/handwritten.ts" "// engine\n")
set(check ${generate} --output "${engine}" --check)
execute_process(COMMAND ${check} --module View OUTPUT_VARIABLE summary
    OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT summary STREQUAL "View: 2 files match")
    message(FATAL_ERROR "Unexpected module check summary: ${summary}")
endif()
file(WRITE "${engine}/codegen-generated-manifest.json" "{}")
run(${check} --module View --diff)
run(${check} --module Model)
expect_failure(1 "unexpected: handwritten[.]ts" ${check})

file(APPEND "${engine}/api/support.ts" "// edited\n")
file(SHA256 "${engine}/api/support.ts" edited)
file(REMOVE "${engine}/api/view.ts")
execute_process(COMMAND ${check} --module View --diff ERROR_VARIABLE diagnostics RESULT_VARIABLE rc)
if(NOT rc STREQUAL "1" OR NOT diagnostics MATCHES "changed: api/support[.]ts"
        OR NOT diagnostics MATCHES "first mismatch at byte" OR NOT diagnostics MATCHES "missing: api/view[.]ts"
        OR diagnostics MATCHES "other/|codegen-generated-manifest|unexpected:")
    message(FATAL_ERROR "Incorrect module diagnostics (${rc}): ${diagnostics}")
endif()
file(SHA256 "${engine}/api/support.ts" after)
if(NOT after STREQUAL edited OR EXISTS "${engine}/api/view.ts")
    message(FATAL_ERROR "Module check modified its outputs")
endif()

set(empty "${work}/absent")
expect_failure(2 "--module requires --check" ${generate} --output "${empty}" --module View)
expect_failure(2 "unknown module: view" ${generate} --output "${empty}" --check --module view)
expect_failure(2 "--module expects one module name" ${check} --module)
expect_failure(2 "--module expects one module name" ${check} --module View --module Provider)
expect_failure(1 "missing: api/view[.]ts" ${generate} --output "${empty}" --check --module View)
if(EXISTS "${empty}")
    message(FATAL_ERROR "Module check created the output directory")
endif()

string(JSON config REMOVE "${config}" manifest)
file(WRITE "${workspace}" "${config}")
run(${generate} --check --module View)
string(JSON config SET "${config}" modules 1 runtimeFiles [[{"other/unused.ts":"unavailable.ts"}]])
file(WRITE "${workspace}" "${config}")
run(${generate} --check --module View)
string(JSON config SET "${config}" modules 1 runtimeFiles [[{"api/support.ts":"runtime.ts"}]])
file(WRITE "${workspace}" "${config}")
expect_failure(2 "duplicate output api/support[.]ts" ${generate} --check --module View)
finish_workspace("${work}")
