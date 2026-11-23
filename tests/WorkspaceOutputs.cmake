include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(COPY "${SOURCE}/spec" "${SOURCE}/config" DESTINATION "${work}")
set(workspace "${work}/config/workspace.json")
file(READ "${workspace}" config)
string(JSON obsolete SET "${config}" types [["missing-types.json"]])
file(WRITE "${workspace}" "${obsolete}")
expect_failure(2 "workspace[.]types was removed[.] Declare external types in a [.]spec extern module" "${TOOL}" generate --workspace "${workspace}")
string(JSON obsolete SET "${config}" modules 0 types [[{"Material":{"header":"Material.h"}}]])
file(WRITE "${workspace}" "${obsolete}")
expect_failure(2 "module[.]types was removed[.] Declare external types in a [.]spec extern module" "${TOOL}" generate --workspace "${workspace}")
string(JSON length LENGTH "${config}" modules)
math(EXPR last "${length} - 1")
foreach(i RANGE 0 ${last})
    string(JSON spec GET "${config}" modules ${i} spec)
    file(STRINGS "${work}/config/${spec}" external REGEX "^extern module " LIMIT_COUNT 1)
    if(NOT external)
        list(APPEND outputModules ${i})
    endif()
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
function(check_graph_order header graph)
    file(READ "${work}/both/native/cocos/renderer/pipeline/custom/${header}" text)
    string(FIND "${text}" "struct ${graph} {" start)
    if(start EQUAL -1)
        message(FATAL_ERROR "Missing C++ graph ${graph}")
    endif()
    string(SUBSTRING "${text}" ${start} -1 body)
    string(FIND "${body}" "\n};" end)
    string(SUBSTRING "${body}" 0 ${end} body)
    set(previous -1)
    foreach(fragment IN LISTS ARGN)
        string(FIND "${body}" "${fragment}" at)
        if(at LESS_EQUAL previous)
            message(FATAL_ERROR "${graph}: missing or misplaced ${fragment}")
        endif()
        set(previous ${at})
    endforeach()
endfunction()
set(traversal "// Graph\n" "// IncidenceGraph\n" " source(edge_descriptor" "// BidirectionalGraph\n" " in_edges("
    "// AdjacencyGraph\n" " adjacent_vertices(" "// VertexListGraph\n" " vertices()")
foreach(layout IN ITEMS LayoutGraph LayoutGraphData)
    check_graph_order(LayoutGraphTypes.h ${layout} ${traversal} "// MutableGraph\n" " add_vertex(" " add_edge("
        "// ComponentGraph\n" " getName(" "// PolymorphicGraph\n" " kind(vertex_descriptor" "// ReferenceGraph\n" " child(ownership_descriptor"
        "// ParentGraph\n" " locate_child(" "// AddressableGraph\n" " locate(")
endforeach()
check_graph_order(RenderGraphTypes.h RenderGraph ${traversal} "// EdgeListGraph\n" " edges()" "// MutableGraph\n" " add_vertex(" " add_edge("
    "// ReferenceGraph\n" " child(ownership_descriptor" "// MutableReferenceGraph\n" " add_reference(")
foreach(header IN ITEMS LayoutGraphTypes.h RenderGraphTypes.h FGDispatcherTypes.h)
    file(READ "${work}/both/native/cocos/renderer/pipeline/custom/${header}" graphTypes)
    if(NOT graphTypes MATCHES "using vertex_iterator = graph::VertexIterator<" OR
       graphTypes MATCHES "(class|struct|using) (Vertex|Edge|Link|VertexIterator|ChildIterator|ChildLinkIterator|ParentLinkIterator|EdgeIterator|VertexRange|IteratorRange) [={]")
        message(FATAL_ERROR "${header} must use the shared graph iterators and ranges")
    endif()
endforeach()
file(GLOB_RECURSE all RELATIVE "${work}/both" "${work}/both/*")
set(layoutTs "cocos/rendering/custom/layout-graph.ts")
file(READ "${work}/both/${layoutTs}" compactLayout)
file(READ "${work}/both/native/cocos/renderer/pipeline/custom/LayoutGraphTypes.h" layoutHeader)
if(layoutHeader MATCHES "EdgeListGraph|using (edge_iterator|ownership_iterator)| num_edges[(]| edges[(]" OR
   compactLayout MATCHES "EdgeListGraph| ne [(]")
    message(FATAL_ERROR "Layout graphs must omit the whole-graph edge list in both languages")
endif()
string(JSON readableConfig REMOVE "${config}" typescriptDefaults)
file(WRITE "${workspace}" "${readableConfig}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/readable")
file(READ "${work}/readable/${layoutTs}" readableLayout)
if(NOT compactLayout MATCHES "private readonly r: OutE[^}]*private readonly c: OutE" OR NOT readableLayout MATCHES "private readonly roots: OutE[^}]*private readonly _children: OutE"
   OR NOT compactLayout MATCHES "private readonly _names: string" OR NOT readableLayout MATCHES "private readonly _names: string")
    message(FATAL_ERROR "Unexpected graph storage or component names in compact or default mode")
endif()
if(NOT compactLayout MATCHES "private readonly a: LayoutGraphObject\\[\\]" OR compactLayout MATCHES "private (readonly )?x: number\\[\\]"
   OR NOT readableLayout MATCHES "private readonly _objects: LayoutGraphObject\\[\\]" OR readableLayout MATCHES "private (readonly )?_indices:")
    message(FATAL_ERROR "Polymorphic payloads must be a single vertex-indexed column in both naming modes")
endif()
if(NOT compactLayout MATCHES "return this[.]a\\[v\\] as T;" OR NOT readableLayout MATCHES "return this[.]_objects\\[v\\] as T;")
    message(FATAL_ERROR "The typed payload accessor must read the payload column directly in both naming modes")
endif()
if(NOT compactLayout MATCHES "return this[.]t\\[v\\] === id;" OR NOT readableLayout MATCHES "return this[.]_kinds\\[v\\] === id;")
    message(FATAL_ERROR "Polymorphic tag checks must read the tag column directly in both naming modes")
endif()
if(NOT compactLayout MATCHES "switch [(]this[.]t\\[v\\][)]" OR NOT readableLayout MATCHES "switch [(]this[.]_kinds\\[v\\][)]")
    message(FATAL_ERROR "Graph visitors must dispatch on the tag column directly in both naming modes")
endif()
string(JSON readableConfig SET "${config}" typescriptDefaults compactGraphNames false)
file(WRITE "${workspace}" "${readableConfig}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/readable" --check)
file(WRITE "${workspace}" "${config}")
file(READ "${work}/both/cocos/rendering/custom/pipeline.ts" pipeline)
foreach(signature IN ITEMS
        [=[    addQueue (phaseName?: string, passName?: string): ComputeQueueBuilder;]=]
        [=[    addQueue (
        hint?: QueueHint,
        phaseName?: string,
        passName?: string): RenderQueueBuilder;]=]
        [=[    configCamera?(
        camera: Readonly<Camera>,
        pplConfigs: Readonly<Record<string, any>>,
        cameraConfigs: Record<string, any>): void;]=])
    string(FIND "${pipeline}" "${signature}" at)
    if(at EQUAL -1)
        message(FATAL_ERROR "Unexpected parameter layout for TypeScript signature: ${signature}")
    endif()
endforeach()
file(READ "${work}/both/native/cocos/renderer/pipeline/custom/RenderInterfaceTypes.h" renderHeader)
foreach(type IN ITEMS Setter SceneBuilder)
    if(renderHeader MATCHES "\n    ((virtual )?~?${type}\\(|${type} *& *operator=)")
        message(FATAL_ERROR "${type} must inherit the special member behavior of its generated interface base")
    endif()
endforeach()
file(READ "${work}/both/native/cocos/renderer/pipeline/custom/PrivateTypes.h" privateHeader)
if(NOT renderHeader MATCHES "RenderNode\\(\\) noexcept = default;" OR NOT privateHeader MATCHES "ProgramProxy\\(\\) = default;")
    message(FATAL_ERROR "The generated root must be noexcept and ProgramProxy must infer its constructor exception specification from RefCounted")
endif()
if(NOT renderHeader MATCHES "\n    virtual ~RenderNode\\(\\) noexcept = default;" OR
   NOT privateHeader MATCHES "\n    ~ProgramProxy\\(\\) noexcept override = default;")
    message(FATAL_ERROR "RenderNode must declare a virtual destructor and ProgramProxy must override the RefCounted destructor")
endif()
set(interfaceTypes RenderNode ProgramProxy)
set(interfaceHeaders renderHeader privateHeader)
foreach(type header IN ZIP_LISTS interfaceTypes interfaceHeaders)
    foreach(member IN ITEMS "${type}(${type} &&rhs) = delete;"
                            "${type}(${type} const &rhs) = delete;" "${type} &operator=(${type} &&rhs) = delete;"
                            "${type} &operator=(${type} const &rhs) = delete;")
        string(FIND "${${header}}" "${member}" index)
        if(index EQUAL -1)
            message(FATAL_ERROR "Missing explicit special member of ${type}: ${member}")
        endif()
    endforeach()
endforeach()
# Forward declarations precede the module body and intentionally share a namespace block.
string(FIND "${renderHeader}" "namespace render {" renderBodyStart)
string(SUBSTRING "${renderHeader}" ${renderBodyStart} -1 renderBody)
if(renderBody MATCHES "([;}]\n|\n\n\n)(/\\*\\*|class |struct |enum class )")
    message(FATAL_ERROR "Expected one blank line before each top-level RenderInterface declaration or its documentation")
endif()
if(NOT renderHeader MATCHES "constexpr bool ENABLE_SUBPASS = true;" OR pipeline MATCHES "ENABLE_SUBPASS")
    message(FATAL_ERROR "RenderInterface must publish the subpass switch in its native header")
endif()
foreach(fragment IN ITEMS "class Setter : public RenderNode"
                          "const ccstd::vector<gfx::CommandBuffer *> &getCommandBuffers()"
                          "static RenderingModule *init(gfx::Device *deviceIn,"
                          "SubpassCapabilities &operator&=(SubpassCapabilities &lhs,"
                          "void addUploadPass(ccstd::vector<UploadPair> &uploadPairs)"
                          [=[    void addMaterialTexture(const ccstd::string &resourceName) {
        addMaterialTexture(resourceName, gfx::ShaderStageFlagBit::VERTEX | gfx::ShaderStageFlagBit::FRAGMENT);
    }]=]
                          [=[    ComputeQueueBuilder *addQueue() {
        return addQueue("default", "");
    }]=])
    string(FIND "${renderHeader}" "${fragment}" index)
    if(index EQUAL -1)
        message(FATAL_ERROR "Expected C++ interface output: ${fragment}")
    endif()
endforeach()
string(REGEX MATCHALL "export (enum|interface|class) [A-Za-z0-9_]+" pipelineExports "${pipeline}")
list(LENGTH pipelineExports count)
if(NOT count EQUAL 22 OR pipeline MATCHES "(Factory|isOcclusionQueryEnabled|resetRenderQueue|setValue \\()")
    message(FATAL_ERROR "Unexpected RenderInterface TypeScript exports: ${pipelineExports}")
endif()
# Each declaration starts after one blank line, including its optional JSDoc.
string(REGEX REPLACE "[^\n]\n\n(/\\*\\*\n( [*][^\n]*\n)* [*]/\n)?export (enum|interface|class) " "" unspaced "${pipeline}")
if(unspaced MATCHES "export (enum|interface|class) [A-Za-z0-9_]+")
    message(FATAL_ERROR "Expected one blank line before the declaration and its JSDoc: ${CMAKE_MATCH_0}")
endif()
if(pipeline MATCHES "[*]/\n\n+export (enum|interface|class) [A-Za-z0-9_]+")
    message(FATAL_ERROR "JSDoc must be adjacent to its TypeScript declaration: ${CMAKE_MATCH_0}")
endif()
foreach(path IN ITEMS native/cocos/renderer/pipeline/custom/RenderSettingsTypes.h
                      native/cocos/renderer/pipeline/custom/RenderSettingsTypes.cpp
                      native/cocos/renderer/pipeline/custom/RenderSettingsFwd.h
                      editor/assets/default_renderpipeline/builtin-pipeline-types.ts
                      native/cocos/renderer/pipeline/custom/RenderCommonJsb.cpp
                      native/tools/swig-config/render.i
                      cocos/rendering/custom/layout-graph-names.ts)
    if(NOT path IN_LIST all)
        message(FATAL_ERROR "Missing selected output ${path}")
    endif()
endforeach()
foreach(language IN ITEMS cpp typescript)
    if(language STREQUAL "cpp")
        set(remove typescript)
        set(extension "[.](cpp|h|i)$")
    else()
        set(remove cpp)
        set(extension "[.]ts$")
    endif()
    set(selected "${config}")
    foreach(i IN LISTS outputModules)
        string(JSON option ERROR_VARIABLE missing TYPE "${selected}" modules ${i} ${remove})
        if(NOT missing)
            string(JSON selected REMOVE "${selected}" modules ${i} ${remove})
        endif()
    endforeach()
    file(WRITE "${workspace}" "${selected}")
    run("${TOOL}" generate --workspace "${workspace}" --output "${work}/${language}")
    run("${TOOL}" generate --workspace "${workspace}" --output "${work}/${language}" --check)
    file(GLOB_RECURSE actual RELATIVE "${work}/${language}" "${work}/${language}/*")
    list(REMOVE_ITEM actual "codegen-generated-manifest.json")
    set(expected ${all})
    list(FILTER expected INCLUDE REGEX "${extension}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${language} file set differs: ${actual}")
    endif()
    foreach(path IN LISTS expected)
        run("${CMAKE_COMMAND}" -E compare_files "${work}/both/${path}" "${work}/${language}/${path}")
    endforeach()
endforeach()

file(WRITE "${work}/spec/provider.spec" "module Provider; namespace cc { struct Item {} }")
# With no selected language the provider contributes declarations and plans no files.
string(JSON modelOnly SET "${config}" modules [=[[{"spec":"../spec/provider.spec"}]]=])
file(WRITE "${workspace}" "${modelOnly}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/empty-manifest")
file(READ "${work}/empty-manifest/codegen-generated-manifest.json" manifest)
string(JSON count LENGTH "${manifest}" files)
if(NOT count EQUAL 0)
    message(FATAL_ERROR "Expected an empty manifest for a model-only workspace")
endif()
string(JSON modelOnly REMOVE "${modelOnly}" manifest)
file(WRITE "${workspace}" "${modelOnly}")
run("${TOOL}" generate --workspace "${workspace}" --output "${work}/model-only" --check --diff)

# A consumer must be able to include or import the provider in its selected language.
file(WRITE "${work}/spec/web.spec" "module Web; namespace cc { struct WebItem {} }")
file(WRITE "${work}/spec/use.spec" "module Use; import Provider; import Web; namespace cc { struct UseItem { item: Item; } }")
string(JSON config SET "${config}" modules [=[
[
    {"spec":"../spec/provider.spec","typescript":"provider.ts"},
    {"spec":"../spec/use.spec","cpp":{},"typescript":"use.ts"},
    {"spec":"../spec/web.spec","typescript":"web.ts"}
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
file(STRINGS "${work}/mixed/native/cocos/renderer/pipeline/custom/UseFwd.h" includes REGEX "^#include")
if(NOT includes STREQUAL [[#include "cocos/renderer/pipeline/custom/ProviderFwd.h"]])
    message(FATAL_ERROR "Unexpected forward includes for mixed-language imports: ${includes}")
endif()
# Legacy switches still produce the source and forward header. Explicit filenames are preserved.
string(JSON legacy SET "${config}" modules 0 cpp emitEmptySource false)
string(JSON legacy SET "${legacy}" modules 0 cpp forward false)
string(JSON legacy SET "${legacy}" modules 1 [=[
    {"backend":"records","spec":"../spec/use.spec","nativeRoot":"native",
     "header":"cocos/renderer/pipeline/custom/UseTypes.h","forwardHeader":"cocos/renderer/pipeline/custom/UseFwd.h",
     "forwardIncludes":["cocos/renderer/pipeline/custom/ProviderFwd.h"],
     "typescript":"use.ts","emitEmptySource":false}
]=])
file(WRITE "${workspace}" "${legacy}")
run(${generate} --check --diff)

set(provider "${work}/spec/provider.spec")
set(declarations "module Provider; namespace cc { struct Item {} struct Detail {} }")
file(WRITE "${provider}" "${declarations} binding typescript { export { Missing }; }")
expect_failure(2 "expected a local TypeScript export declaration for Missing" ${generate})
file(WRITE "${provider}" "${declarations} binding typescript { export { Item, Item }; }")
expect_failure(2 "duplicate TypeScript export Item" ${generate})
file(WRITE "${provider}" "${declarations} binding typescript { export { Detail }; }")
expect_failure(2 "missing external record mapping" ${generate})
file(WRITE "${provider}" "${declarations} binding typescript { export { Item }; }")
run(${generate})
file(READ "${work}/mixed/provider.ts" script)
file(READ "${work}/mixed/native/cocos/renderer/pipeline/custom/ProviderTypes.h" header)
if(script MATCHES "Detail" OR NOT header MATCHES "struct Detail")
    message(FATAL_ERROR "TypeScript export selection changed C++ declarations or leaked a hidden type")
endif()
string(REPLACE "struct Detail" "@cppOnly struct Detail" nativeOnly "${declarations}")
file(WRITE "${provider}" "${nativeOnly}")
run(${generate} --check --diff)
file(APPEND "${provider}" " binding typescript { export { Detail }; }")
expect_failure(2 "cannot export @cppOnly type Detail" ${generate})
file(WRITE "${provider}" "${nativeOnly} binding typescript { Detail => class {}; }")
expect_failure(2 "cannot bind @cppOnly type" ${generate})

set(callbacks "module Provider; import Web; namespace cc { struct Item {} interface Hooks {} }
    binding typescript { Hooks => interface { callback update?(item: Readonly<WebItem>): WebItem | undefined; }; }")
file(WRITE "${provider}" "${callbacks}")
run(${generate})
file(READ "${work}/mixed/provider.ts" script)
if(NOT script MATCHES "import type [{] WebItem [}] from './web'" OR NOT script MATCHES "Readonly<WebItem>")
    message(FATAL_ERROR "A type used only by a callback must be imported")
endif()
string(REPLACE "Readonly<WebItem>" "Readonly<Missing>" invalid "${callbacks}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "unknown type Missing" ${generate})
string(REPLACE "interface Hooks {}" "interface Hooks { virtual void update() = 0; }" invalid "${callbacks}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "duplicate TypeScript member Hooks.update" ${generate})
string(REPLACE "item: Readonly<WebItem>" "first?: number, second: number" invalid "${callbacks}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "required callback parameter follows an optional parameter" ${generate})
set(property "getter getValue => value setter setValue;")
set(accessors "module Provider; namespace cc { struct Item {} interface Options {
    virtual int getValue() const = 0; virtual void setValue(int v) = 0;
} } binding typescript { Options => interface { ${property} }; }")
file(WRITE "${provider}" "${accessors}")
run(${generate})
file(READ "${work}/mixed/provider.ts" script)
if(NOT script MATCHES "    value: number;" OR script MATCHES "readonly value|getValue|setValue")
    message(FATAL_ERROR "The getter and setter must produce one writable property")
endif()
string(REPLACE "setValue(int v)" "setValue(bool v)" invalid "${accessors}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "setter setValue must return void and accept the property type" ${generate})
string(REPLACE "setter setValue;" "setter missing;" invalid "${accessors}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "unknown method missing" ${generate})
string(REPLACE "${property}" "${property} method setValue;" invalid "${accessors}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "duplicate method binding setValue" ${generate})

# Moving the property rule onto the declarations preserves both generated interfaces.
set(annotated "module Provider; namespace cc { struct Item {} interface Options {
    @getter virtual int getValue() const = 0; @setter virtual void setValue(int v) = 0;
} }")
file(WRITE "${provider}" "${annotated}")
run(${generate} --check --diff)
file(WRITE "${provider}" "${annotated} binding typescript { Options => interface { ${property} }; }")
expect_failure(2 "duplicate accessor binding for getValue" ${generate})
string(REPLACE "setValue(int v)" "setValue(bool v)" invalid "${annotated}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "setter setValue must return void and accept the property type" ${generate})
string(REPLACE "setValue(int v)" "setOther(int v)" invalid "${annotated}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "setter setOther requires a getter for other" ${generate})
string(REPLACE "getValue() const" "getValue()" invalid "${annotated}")
file(WRITE "${provider}" "${invalid}")
expect_failure(2 "getter requires a const nullary value method" ${generate})
file(WRITE "${provider}" "${annotated} binding typescript { Options => interface { method setValue nullable; }; }")
expect_failure(2 "setter cannot have nullable or optional method bindings" ${generate})

file(WRITE "${provider}" "module Provider; namespace cc { struct Item {} interface Options {
    @setter virtual void setValue(int v) = 0;
    @getter virtual int getURLValue() const = 0;
    @getter virtual Item* getItem() const = 0;
    virtual int getMethod() const = 0;
    @getter virtual int getValue() const = 0;
} } binding typescript { Options => interface { method getItem nullable; }; }")
run(${generate})
file(READ "${work}/mixed/provider.ts" script)
foreach(member IN ITEMS "    value: number;" "    readonly urlValue: number;" "    readonly item: Item | null;" "    getMethod (): number;")
    string(FIND "${script}" "${member}" index)
    if(index EQUAL -1)
        message(FATAL_ERROR "Missing TypeScript member ${member}")
    endif()
endforeach()
if(script MATCHES "readonly value|getURLValue|getValue|setValue")
    message(FATAL_ERROR "TypeScript exposed annotated methods or made a writable property readonly")
endif()

file(WRITE "${work}/spec/shadow.spec" [=[extern module Shadow;
cpp { header "Vendor.h"; }
namespace vendor { struct Device; }
namespace cc::vendor { struct Other; }
]=])
string(JSON config SET "${config}" modules 3 [[{"spec":"../spec/shadow.spec"}]])
file(WRITE "${workspace}" "${config}")
file(WRITE "${provider}" [=[module Provider;
import Shadow;
namespace cc {
    /** Native switch. */
    constexpr bool ENABLE_TEST = false;
    struct Item { @cppOnly link: const Item* = nullptr; @cppOnly static current: Item* = nullptr; Item& assign(const Item& other); Item* clone() const; }
    using Count = int;
    @cppOnly interface Adapter : ::vendor::Device {}
    interface Parent {
        @cppOnly virtual Parent* fork(Count* value) const = 0;
        @cppOnly virtual int inspect(const int& value, int mode = 1) = 0;
        @cppOnly virtual int select(int select = 2, int mode = 3) = 0;
        static int helper() { return 1; }
        static int Item() { return 0; }
    }
    interface Middle : Parent { @cppOnly virtual int count() = 0; }
    interface Child : Middle {
        @cppOnly virtual void borrow(int Count, ::cc::Count* value, ::cc::Item* item, ::vendor::Device* device) = 0;
        @cppOnly virtual int& revise(int& value, const int& other) = 0;
        @cppOnly virtual Child* fork(int* renamed) const noexcept = 0;
        @cppOnly virtual Child* fork(int* renamed) = 0;
        @cppOnly virtual int inspect(const int& value) = 0;
        @cppOnly virtual int helper() = 0;
        int count() { return 2; }
    }
}]=])
run(${generate})
file(READ "${work}/mixed/native/cocos/renderer/pipeline/custom/ProviderTypes.h" header)
foreach(line IN ITEMS "Item &assign(const Item &other);"
                      "const Item *link{nullptr};" "inline static Item *current{nullptr};"
                      "virtual ~Adapter() noexcept = default;"
                      "Item *clone() const;"
                      "    return select(2, 3);"
                      "    return this->select(select, 3);"
                      "virtual int &revise(int &value, const int &other) = 0;"
                      "Child *fork(int *renamed) const noexcept override = 0;"
                      "virtual Child *fork(int *renamed) = 0;"
                      "virtual void borrow(int Count, ::cc::Count *value, ::cc::Item *item, ::vendor::Device *device) = 0;"
                      "virtual int inspect(const int &value) = 0;"
                      "virtual int helper() = 0;"
                      "int count() override { return 2; }")
    string(FIND "${header}" "    ${line}" index)
    if(index EQUAL -1)
        message(FATAL_ERROR "Unexpected C++ override or overload declaration; expected ${line}")
    endif()
endforeach()
if(NOT header MATCHES "Native switch[.]\n \\*/\nconstexpr bool ENABLE_TEST = false;")
    message(FATAL_ERROR "Expected documented native constexpr bool in the module header")
endif()
string(JSON overrideStyle SET "${config}" modules 0 cpp spaceBeforeReference false)
file(WRITE "${workspace}" "${overrideStyle}")
run(${generate})
set(providerHeader "${work}/mixed/native/cocos/renderer/pipeline/custom/ProviderTypes.h")
file(READ "${providerHeader}" overridden)
foreach(line IN ITEMS "Item& assign(const Item& other);" "Item* clone() const;" "virtual int& revise(int& value, const int &other) = 0;"
                      "const Item* link{nullptr};" "inline static Item* current{nullptr};"
                      "Parent& operator=(Parent&& rhs) = delete;")
    string(FIND "${overridden}" "${line}" index)
    if(index EQUAL -1)
        message(FATAL_ERROR "Expected original reference spacing after module override: ${line}")
    endif()
endforeach()
string(JSON defaultStyle REMOVE "${config}" cppDefaults spaceBeforeReference)
file(WRITE "${workspace}" "${defaultStyle}")
run(${generate})
file(READ "${providerHeader}" original)
if(NOT original STREQUAL overridden)
    message(FATAL_ERROR "Omitting spaceBeforeReference must produce the original reference spacing")
endif()

file(WRITE "${work}/spec/shadow.spec" [=[extern module Shadow;
cpp { header "vendor/Fwd.h"; }
namespace vendor {
    header "vendor/{name}.h" { class Device; struct Options; class State; struct Other; type Handle; }
}]=])
string(JSON forwardConfig SET "${config}" modules [[ [{"spec":"../spec/shadow.spec"},{"spec":"../spec/provider.spec","cpp":{}}] ]])
file(WRITE "${workspace}" "${forwardConfig}")
set(forwardSpec [=[module Provider; import Shadow;
namespace cc {
    interface API {
        virtual const vendor::Device* get() const = 0;
        virtual void bind(const vendor::Device& dev, vendor::State* state, vendor::Other* other) = 0;
        virtual void configure(const vendor::Options& opts = {}) = 0;
        virtual vendor::State snapshot() const = 0;
        virtual vendor::Handle& handle() const = 0;
    }
}]=])
file(WRITE "${provider}" "${forwardSpec}")
run(${generate})
file(READ "${providerHeader}" header)
foreach(path IN ITEMS Options State Handle)
    if(NOT header MATCHES "vendor/${path}[.]h")
        message(FATAL_ERROR "Missing required interface header vendor/${path}.h")
    endif()
endforeach()
set(group "namespace vendor {\nclass Device;\nstruct Other;\n}")
string(FIND "${header}" "${group}" groupIndex)
if(groupIndex EQUAL -1 OR header MATCHES "vendor/(Device|Other|Fwd)[.]h|class Handle;|class State;|struct Options;")
    message(FATAL_ERROR "Expected grouped declarations only for Device and Other")
endif()
string(REPLACE "    interface API {" "    struct Fields { device: vendor::Device* = nullptr; other: const vendor::Other&; handle: vendor::Handle* = nullptr; constructor(other); }\n    interface API {" withFields "${forwardSpec}")
file(WRITE "${provider}" "${withFields}")
run(${generate})
file(READ "${providerHeader}" mixedHeader)
string(FIND "${mixedHeader}" "${group}" groupIndex)
if(groupIndex EQUAL -1 OR mixedHeader MATCHES "vendor/(Device|Other)[.]h|class Handle;")
    message(FATAL_ERROR "Fields and methods must share forward declarations")
endif()
file(WRITE "${provider}" "${forwardSpec}")
file(WRITE "${work}/spec/use.spec" [=[module Use; import Provider;
namespace cc { interface Reader : API { int read() const { return get()->status(); } } }]=])
string(JSON forwardConfig SET "${forwardConfig}" modules 2 [[{"spec":"../spec/use.spec","cpp":{}}]])
file(WRITE "${workspace}" "${forwardConfig}")
run(${generate})
file(READ "${work}/mixed/native/cocos/renderer/pipeline/custom/UseTypes.h" reader)
if(NOT reader MATCHES "vendor/Device[.]h")
    message(FATAL_ERROR "An opaque body using an inherited getter needs the definition in the consuming header")
endif()
string(REPLACE "interface API {" "interface API { int inspect() const { return get()->status(); }" withBody "${forwardSpec}")
file(WRITE "${provider}" "${withBody}")
run(${generate})
file(READ "${providerHeader}" header)
if(NOT header MATCHES "vendor/Device[.]h")
    message(FATAL_ERROR "Opaque inline member bodies must retain complete interface dependencies")
endif()
finish_workspace("${work}")
