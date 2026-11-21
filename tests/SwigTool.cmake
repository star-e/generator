# Match the engine flags in cc_gen_swig_files.
function(render_swig_command result native input)
    if(CMAKE_HOST_WIN32)
        set(swig "${native}/external/win64/bin/swig")
        set(executable "${swig}/bin/swig.exe")
    elseif(CMAKE_HOST_APPLE)
        set(swig "${native}/external/mac/bin/swig")
        set(executable "${swig}/bin/swig")
    else()
        set(swig "${native}/external/linux/bin/swig")
        set(executable "${swig}/bin/swig")
    endif()
    set(lib "${swig}/share/swig/4.1.0")
    set(${result} "${executable}" -c++ -cocos -fvirtual -noexcept -cpperraswarn
        -D__clang__ -Dfinal= -DCC_PLATFORM=3 -Dconstexpr=const -DCC_PLATFORM_ANDROID=3
        "-I${lib}/javascript/cocos" "-I${lib}" "-I${input}" "-I${input}/cocos" "-I${native}" "-I${native}/cocos"
        PARENT_SCOPE)
endfunction()
