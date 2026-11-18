include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(COPY "${SOURCE}/spec" "${SOURCE}/config" DESTINATION "${work}")
set(workspace "${work}/config/workspace.json")
set(generate "${TOOL}" generate --workspace "${workspace}" --output "${work}/stage")
run(${generate})

# Consumer-first and provider-first workspaces must plan the same files.
file(READ "${workspace}" config)
string(JSON count LENGTH "${config}" modules)
math(EXPR last "${count} - 1")
set(reordered "[]")
foreach(i RANGE 0 ${last})
    math(EXPR j "${last} - ${i}")
    string(JSON module GET "${config}" modules ${j})
    string(JSON reordered SET "${reordered}" ${i} "${module}")
endforeach()
string(JSON config SET "${config}" modules "${reordered}")
file(WRITE "${workspace}" "${config}")
run(${generate} --check --diff)

set(common "${work}/spec/render/render-common.spec")
file(READ "${common}" original)
file(STRINGS "${common}" commonLines)
list(FIND commonLines "        nullable light: IntrusivePtr<scene::Light>;" lightIndex)
list(FIND commonLines "        nullable probe: scene::ReflectionProbe* = nullptr;" probeIndex)
math(EXPR lightLine "${lightIndex} + 1")
math(EXPR probeLine "${probeIndex} + 1")
function(change_imports before after)
    string(REPLACE "${before}" "${after}" changed "${original}")
    file(WRITE "${common}" "${changed}")
endfunction()

change_imports("import Gfx;" "")
expect_failure(2 "render-common.spec:[0-9]+:[0-9]+: cc::gfx::ResolveMode requires import Gfx in module RenderCommon" ${generate})
change_imports("import ArchiveInterface;" "")
expect_failure(2 "serialization requires import ArchiveInterface" ${generate})
change_imports("import ReflectionProbe;" "")
expect_failure(2 "render-common.spec:${probeLine}:25: cc::scene::ReflectionProbe requires import ReflectionProbe in module RenderCommon" ${generate})
change_imports("import Light;" "")
expect_failure(2 "render-common.spec:${lightLine}:38: cc::scene::Light requires import Light in module RenderCommon" ${generate})

# A provider's own imports do not make a third module's names visible to its consumer.
set(probe "${work}/spec/extern/reflection-probe.spec")
file(READ "${probe}" provider)
string(REPLACE "namespace" "import Light;\nnamespace" indirect "${provider}")
file(WRITE "${probe}" "${indirect}")
expect_failure(2 "render-common.spec:${lightLine}:38: cc::scene::Light requires import Light" ${generate})

change_imports("import Light;" "import Missing;")
expect_failure(2 "render-common.spec:2:8: unknown imported module Missing" ${generate})
change_imports("import ReflectionProbe;" "import Light;")
expect_failure(2 "render-common.spec:3:8: duplicate import Light" ${generate})
file(WRITE "${common}" "${original}")
string(REPLACE "namespace" "import RenderCommon;\nnamespace" cyclic "${provider}")
file(WRITE "${probe}" "${cyclic}")
expect_failure(2 "module import cycle: ReflectionProbe -> RenderCommon -> ReflectionProbe" ${generate})
file(WRITE "${probe}" "${provider}")

set(graphSpec "${work}/spec/graph.spec")
string(JSON config SET "${config}" modules ${count} [[{"backend":"model","spec":"../spec/graph.spec"}]])
file(WRITE "${workspace}" "${config}")
set(graph "module GraphConsumer;
import RenderCommon;
namespace cc::render {
    graph Passes {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        component RasterPassTag: uint32_t { member: passes; }
    }
}")
file(WRITE "${graphSpec}" "${graph}")
run(${generate} --check --diff)
function(bad_graph before after error)
    string(REPLACE "${before}" "${after}" broken "${graph}")
    file(WRITE "${graphSpec}" "${broken}")
    expect_failure(2 "graph.spec:[0-9]+:[0-9]+: ${error}" ${generate})
endfunction()
bad_graph("import RenderCommon;" "" "cc::render::RasterPassTag requires import RenderCommon")
bad_graph("RasterPassTag:" "MissingTag:" "unknown type MissingTag")
bad_graph("RasterPassTag:" "LightInfo:" "component requires an independently declared @tag")
bad_graph("component RasterPassTag:" "@tag struct LocalTag {} component LocalTag:" "expected graph descriptor, storage or component reference")
bad_graph("component RasterPassTag:" "component RasterPassTag: uint32_t { member: first; } component RasterPassTag:" "invalid or duplicate component accessor")

finish_workspace("${work}")
