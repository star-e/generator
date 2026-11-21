include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(WRITE "${work}/preamble.txt" "")
file(WRITE "${work}/workspace.json" [[{
  "outputRoot":"out","lineEnding":"lf","preamble":"preamble.txt",
  "modules":[{"spec":"api.spec","cpp":{"header":"Api.h"},"typescript":"api.ts"}]
}]])
set(spec [[module Api;
namespace cc {
    // @zh private-note
    /* private-note
       @en also a private-note */
    /** A setting. */
    enum class Mode {
        /**
         * First value.
         * @deprecated Use B.
         */
        A, B
    }
    /** Storage. */
    struct Data {
        /** Current value. */ value: uint32_t = 0;
        /** Make storage. */ constructor(value);
    }
    /**
     * @en Public API.
     *
     * @zh 接口说明。
     */
    interface Api {
        /** Read the value. */
        @getter virtual int getValue() const = 0;
        /** Write the value. */
        @setter virtual void setValue(int value) = 0;
        /** Enabled state. */
        @getter virtual bool getEnabled() const = 0;
        @getter virtual int getLimit() const = 0;
        /** Upper bound. */ @setter virtual void setLimit(int value) = 0;
        /** Native only. */
        @cppOnly virtual void internals() = 0;
        /** Integer overload. */
        @cppOnly virtual void internals(int index) = 0;
        /// private-note
        virtual void clear(/* private-note */) = 0;
    }
}
binding typescript {
    Api => interface {
        /** Web callback. */ callback notify?(): void;
    };
    Data => class {
        /** Web method. */ read(): number { return this.value; }
    };
}
]])
set(generate "${TOOL}" generate --workspace "${work}/workspace.json")
string(REPLACE "\n" "\r\n" crlf "${spec}")
file(WRITE "${work}/api.spec" "${crlf}")
run(${generate})
file(READ "${work}/out/Api.h" cpp)
file(READ "${work}/out/api.ts" ts)
function(has output snippet)
    string(FIND "${${output}}" "${snippet}" at)
    if(at EQUAL -1)
        message(FATAL_ERROR "${output} missing [${snippet}]")
    endif()
endfunction()
foreach(output IN ITEMS cpp ts)
    has(${output} "/**\n * @en Public API.\n *\n * @zh 接口说明。\n */")
    has(${output} "     * First value.\n     * @deprecated Use B.\n     */\n    A,")
    has(${output} " * Storage.\n */")
    has(${output} "     * Current value.\n     */")
    has(${output} "     * Make storage.\n     */")
    if(${output} MATCHES "private-note")
        message(FATAL_ERROR "Maintenance comment reached ${output}")
    endif()
endforeach()
has(cpp "     * Read the value.\n     */\n    virtual int getValue() const = 0;")
has(cpp "     * Write the value.\n     */\n    virtual void setValue(int value) = 0;")
has(cpp "     * Native only.\n     */\n    virtual void internals() = 0;")
has(cpp "     * Integer overload.\n     */\n    virtual void internals(int index) = 0;")
has(ts "     * Read the value.\n     */\n    value: number;")
has(ts "     * Enabled state.\n     */\n    readonly enabled: boolean;")
has(ts "     * Upper bound.\n     */\n    limit: number;")
has(ts "     * Web callback.\n     */\n    notify? (")
has(ts "     * Web method.\n     */\n    read (")
if(ts MATCHES "Write the value|Native only" OR cpp MATCHES "Web callback|Web method")
    message(FATAL_ERROR "Documentation attached to an unrelated declaration")
endif()
file(WRITE "${work}/api.spec" "module Api;\nnamespace cc {\n  /* missing terminator")
expect_failure(2 "api[.]spec:3:3: unterminated block comment" ${generate})
finish_workspace("${work}")
