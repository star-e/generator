include("${CMAKE_CURRENT_LIST_DIR}/TestSupport.cmake")
test_workspace(work)
file(WRITE "${work}/workspace.json" [[{
    "lineEnding":"lf", "preamble":"preamble.txt", "outputRoot":"out",
    "modules":[{"spec":"api.spec","cpp":{"swig":"binding.i"},"typescript":"api.ts"}]
}]])
file(WRITE "${work}/preamble.txt" "")
set(spec [[module Api;
namespace cc::test {
    struct Item {}
    interface Api {
        @setter virtual void setURLValue(int value) = 0;
        @getter virtual int getURLValue() const = 0;
        virtual Item* create() = 0;
        virtual Item* borrow() const = 0;
        virtual int method(bool flag) = 0;
        @cppOnly virtual int method(int value, int extra = 0) const = 0;
        @cppOnly int nativeHelper();
    }
}
binding typescript { Api => interface {
    method getURLValue => value;
    method setURLValue => value;
    getter borrow => item;
}; }
]])
set(generate "${TOOL}" generate --workspace "${work}/workspace.json")
file(WRITE "${work}/api.spec" "${spec}binding swig { macro FEATURE Api.urlValue; releaseInGC Api.create; }")
run(${generate})
file(READ "${work}/out/binding.i" result)
foreach(line IN ITEMS [[%module(target_namespace="test") binding]]
        [[%module_macro(FEATURE) cc::test::Api::urlValue;]]
        [[%attribute(cc::test::Api, int, urlValue, getURLValue, setURLValue);]]
        [[%ignore cc::test::Api::method(int) const;]] [[%ignore cc::test::Api::method(int, int) const;]]
        [[%ignore cc::test::Api::nativeHelper;]]
        [[%release_returned_cpp_object_in_gc(cc::test::Api::create);]])
    string(FIND "${result}" "${line}" pos)
    if(pos EQUAL -1)
        message(FATAL_ERROR "Missing SWIG directive ${line}")
    endif()
endforeach()
if(result MATCHES "Api::borrow|, item, borrow|%ignore cc::test::Api::method;")
    message(FATAL_ERROR "SWIG exposed TS-only rules or hid a visible overload")
endif()
function(check_rule rule diagnostic)
    file(WRITE "${work}/api.spec" "${spec}binding swig { ${rule} }")
    expect_failure(2 "${diagnostic}" ${generate})
endfunction()
check_rule("macro FEATURE Api.value;" "unknown SWIG macro target")
check_rule("releaseInGC Api.create; releaseInGC Api.create;" "duplicate SWIG releaseInGC")
check_rule("releaseInGC Api.method;" "requires an object pointer return")
check_rule("releaseInGC Missing.create;" "unknown SWIG releaseInGC target")
finish_workspace("${work}")
