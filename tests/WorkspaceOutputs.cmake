include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(COPY "${SOURCE}/spec" "${SOURCE}/config" DESTINATION "${work}")
set(workspace "${work}/config/workspace.json")
file(READ "${workspace}" config)
string(JSON length LENGTH "${config}" modules)
math(EXPR last "${length} - 1")
foreach(i RANGE 0 ${last})
    string(JSON spec GET "${config}" modules ${i} spec)
    if(spec MATCHES "/render-settings[.]spec$")
        string(JSON config SET "${config}" modules ${i} cpp "{}")
    endif()
    # Compare generated files only. Explicit runtime copies have independent destinations.
    string(JSON runtime ERROR_VARIABLE missing TYPE "${config}" modules ${i} runtimeFiles)
    if(NOT missing)
        string(JSON config REMOVE "${config}" modules ${i} runtimeFiles)
    endif()
endforeach()
file(WRITE "${workspace}" "${config}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/both")
file(GLOB_RECURSE all RELATIVE "${work}/both" "${work}/both/*")
foreach(path IN ITEMS native/cocos/renderer/pipeline/custom/RenderSettingsTypes.h
                      native/cocos/renderer/pipeline/custom/RenderSettingsTypes.cpp
                      native/cocos/renderer/pipeline/custom/RenderSettingsFwd.h
                      editor/assets/default_renderpipeline/builtin-pipeline-types.ts
                      native/cocos/renderer/pipeline/custom/RenderCommonJsb.cpp
                      cocos/rendering/custom/layout-graph-names.ts)
    if(NOT path IN_LIST all)
        message(FATAL_ERROR "Missing selected output ${path}")
    endif()
endforeach()
foreach(language IN ITEMS cpp typescript)
    if(language STREQUAL "cpp")
        set(remove typescript)
        set(extension "[.](cpp|h)$")
    else()
        set(remove cpp)
        set(extension "[.]ts$")
    endif()
    set(selected "${config}")
    foreach(i RANGE 0 ${last})
        string(JSON option ERROR_VARIABLE missing TYPE "${selected}" modules ${i} ${remove})
        if(NOT missing)
            string(JSON selected REMOVE "${selected}" modules ${i} ${remove})
        endif()
    endforeach()
    file(WRITE "${workspace}" "${selected}")
    run("${TOOL}" generate --workspace "${workspace}" --output "${work}/${language}")
    file(GLOB_RECURSE actual RELATIVE "${work}/${language}" "${work}/${language}/*")
    set(expected ${all})
    list(FILTER expected INCLUDE REGEX "${extension}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${language} file set differs: ${actual}")
    endif()
    foreach(path IN LISTS expected)
        run("${CMAKE_COMMAND}" -E compare_files "${work}/both/${path}" "${work}/${language}/${path}")
    endforeach()
endforeach()

# A consumer must be able to include or import the provider in its selected language.
file(WRITE "${work}/spec/provider.spec" "module Provider; namespace cc { struct Item {} }")
file(WRITE "${work}/spec/use.spec" "module Use; import Provider; namespace cc { struct UseItem { item: Item; } }")
string(JSON config SET "${config}" modules [=[
[
    {"spec":"../spec/provider.spec","typescript":"provider.ts"},
    {"spec":"../spec/use.spec","cpp":{},"typescript":"use.ts"}
]
]=])
set(generate "${TOOL}" generate --workspace "${workspace}" --output "${work}/mixed")
file(WRITE "${workspace}" "${config}")
expect_failure(2 "missing C[+][+] header for imported type cc::Item" ${generate})
string(JSON config SET "${config}" modules 0 cpp "{}")
string(JSON missing REMOVE "${config}" modules 0 typescript)
file(WRITE "${workspace}" "${missing}")
expect_failure(2 "missing external record mapping" ${generate})
string(JSON invalid SET "${config}" modules 1 cpp jsb true)
file(WRITE "${workspace}" "${invalid}")
expect_failure(2 "cpp.jsb requires a .spec binding jsb block" ${generate})
string(JSON invalid SET "${config}" modules 1 cpp foward true)
file(WRITE "${workspace}" "${invalid}")
expect_failure(2 "unknown option foward" ${generate})
file(WRITE "${workspace}" "${config}")
run(${generate})
run(${generate} --check --diff)
# Both historical spellings of the removed switch use the same source-file plan.
string(JSON legacy SET "${config}" modules 0 cpp emitEmptySource false)
string(JSON legacy SET "${legacy}" modules 1 [=[
    {"backend":"records","spec":"../spec/use.spec","nativeRoot":"native",
     "header":"cocos/renderer/pipeline/custom/UseTypes.h","forwardHeader":"cocos/renderer/pipeline/custom/UseFwd.h",
     "typescript":"use.ts","emitEmptySource":false}
]=])
file(WRITE "${workspace}" "${legacy}")
run(${generate} --check --diff)
finish_workspace("${work}")
