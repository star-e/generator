#include "CppMethod.h"
#include "SyntaxUtils.h"
#include "SyntaxGraphs.h"

namespace Cocos::Meta {

namespace {

std::pmr::vector<std::pmr::string> splitFunctions(
    std::string_view functions, std::pmr::memory_resource* scratch) {
    std::pmr::vector<std::pmr::string> results(scratch);

    size_t pos = 0;
    size_t funcBeg = pos;
    for (pos = functions.find(';', pos); pos != functions.npos; pos = functions.find(';', pos + 1)) {
        const auto funcEnd = pos;
        std::pmr::string func(functions.substr(funcBeg, funcEnd - funcBeg), scratch);
        convertTypename(func);
        results.emplace_back(std::move(func));
        funcBeg = pos + 1;
    }
    return results;
}

void parseParameter(ModuleBuilder& builder, std::string_view& parameters, Parameter& param) {
    auto scratch = builder.mScratch;
    Ensures(!parameters.empty());
    Ensures(parameters.back() != ' ');

    if (parameters.ends_with("&&")) {
        param.mReference = true;
        param.mRvalue = true;
        parameters = parameters.substr(0, parameters.size() - 2);
    } else if (parameters.back() == '&') {
        param.mReference = true;
        parameters = parameters.substr(0, parameters.size() - 1);
    }

    if (parameters.back() == '*') {
        param.mPointer = true;
        parameters = parameters.substr(0, parameters.size() - 1);
    }

    std::string_view typeName;
    if (parameters.back() == '>') {
        auto typeEnd = parameters.size();
        auto templateBeg = findBegMatch(parameters, '<', '>', parameters.size() - 1);
        auto commaPos = parameters.find_last_of(",", templateBeg);
        if (commaPos != std::string_view::npos) {
            auto typeBeg = commaPos + 1;
            typeName = parameters.substr(typeBeg, typeEnd - typeBeg);
            parameters = parameters.substr(0, commaPos);
        } else {
            typeName = parameters.substr(0, typeEnd);
            parameters = {};
        }
    } else {
        auto typeEnd = parameters.size();
        auto commaPos = parameters.rfind(",");
        if (commaPos != std::string_view::npos) {
            auto typeBeg = commaPos + 1;
            typeName = parameters.substr(typeBeg, typeEnd - typeBeg);
            parameters = parameters.substr(0, commaPos);
        } else {
            typeName = parameters.substr(0, typeEnd);
            parameters = {};
        }
    }
    if (boost::algorithm::starts_with(typeName, "[[optional]]")) {
        param.mOptional = true;
        typeName = typeName.substr(12);
    }
    if (typeName.starts_with("const ")) {
        param.mConst = true;
        typeName = typeName.substr(6);
    }
    auto& g = builder.mSyntaxGraph;
    if (isInstance(typeName)) {
        g.instantiate(builder.mCurrentScope, typeName, scratch);
    }

    param.mTypePath = g.getTypePath(builder.mCurrentScope,
        typeName, scratch, scratch);
    Expects(locate(param.mTypePath, g) != g.null_vertex());
}

Parameter findLastParameter(ModuleBuilder& builder, std::string_view& parameters) {
    auto scratch = builder.mScratch;
    Parameter param(scratch);

    {
        auto pos = parameters.rfind('=');
        if (pos != std::string_view::npos) {
            param.mDefaultValue = parameters.substr(pos + 1);
            parameters = parameters.substr(0, pos);
        }
    }

    Expects(!parameters.empty());
    auto pos = parameters.find_last_of(" *&>");
    Expects(pos != std::string_view::npos);

    param.mName = parameters.substr(pos + 1);
    Ensures(!param.mName.empty());
    Ensures(param.mName.front() != ' ');

    if (parameters[pos] == ' ') {
        parameters = parameters.substr(0, pos);
    } else {
        parameters = parameters.substr(0, pos + 1);
    }

    parseParameter(builder, parameters, param);

    return param;
}

}

Method parseFunction(ModuleBuilder& builder, std::string_view function) {
    auto scratch = builder.mScratch;
    Method method(scratch);

    if (boost::algorithm::contains(function, "[[skip]]")) {
        method.mSkip = true;
    }

    if (boost::algorithm::contains(function, "[[getter]]")) {
        method.mGetter = true;
    }

    if (boost::algorithm::contains(function, "[[setter]]")) {
        method.mSetter = true;
    }

    {
        auto pos = function.find('(');
        Expects(pos != function.npos);
        auto function2 = function.substr(0, pos);
        if (boost::algorithm::contains(function2, "[[optional]]")) {
            method.mOptional = true;
        }
    }

    if (boost::algorithm::contains(function, "[[covariant]]")) {
        method.mCovariant = true;
    }

    if (boost::algorithm::contains(function, "[[deprecated]]")) {
        method.mFlags |= Doc::Deprecated;
    }

    if (boost::algorithm::contains(function, "[[beta]]")) {
        method.mFlags |= Doc::Beta;
    }

    if (boost::algorithm::contains(function, "[[experimental]]")) {
        method.mFlags |= Doc::Experimental;
    }

    // skip attributes
    {
        int count = 0;
        bool inAttribute = false;
        for (size_t pos = 0; pos != function.size(); ++pos) {
            if (function[pos] == ' ') {
                continue;
            }
            if (function[pos] == '[') {
                Expects(pos + 1 != function.size());
                Expects(function[pos + 1] == '[');
                ++pos;
                inAttribute = true;
            } else if (function[pos] == ']') {
                Expects(pos + 1 != function.size());
                Expects(function[pos + 1] == ']');
                ++pos;
                Expects(inAttribute);
                inAttribute = false;
            } else if (inAttribute) {
                continue;
            } else {
                function = function.substr(pos);
                break;
            }
        }
    }

    // check pure
    if (function.ends_with("=0")) {
        method.mPure = true;
        function = function.substr(0, function.size() - 2);
    }

    // check static
    if (function.starts_with("static ")) {
        method.mStatic = true;
        function = function.substr(7);
    }

    // check virtual
    if (function.starts_with("virtual ")) {
        method.mVirtual = true;
        function = function.substr(8);
    }

    // check noexcept
    if (function.ends_with(" noexcept")) {
        method.mNoexcept = true;
        function = function.substr(0, function.size() - 9);
    }

    // check const
    if (function.ends_with(" const")) {
        method.mConst = true;
        function = function.substr(0, function.size() - 6);
    }

    // parameters end
    Expects(function.ends_with(')'));

    // parameters beg
    {
        auto paramBeg = findBegMatch(function, '(', ')', function.size() - 1) + 1;
        auto paramEnd = function.size() - 1;
        auto parameters = function.substr(paramBeg, paramEnd - paramBeg);
        while (!parameters.empty()) {
            auto param = findLastParameter(builder, parameters);
            method.mParameters.emplace_back(std::move(param));
        }
        std::reverse(method.mParameters.begin(), method.mParameters.end());
        function = function.substr(0, paramBeg - 1);
    }

    // function name
    {
        auto funcBeg = function.find_last_of(" *&>");
        method.mFunctionName = function.substr(funcBeg + 1);
        if (function[funcBeg] == ' ') {
            function = function.substr(0, funcBeg);
        } else {
            function = function.substr(0, funcBeg + 1);
        }
    }

    // return type
    parseParameter(builder, function, method.mReturnType);

    return method;
}

std::pmr::vector<Method> parseFunctions(ModuleBuilder& builder, std::string_view functions) {
    auto scratch = builder.mScratch;
    std::pmr::vector<Method> methods(scratch);
    auto cleaned = splitFunctions(functions, scratch);

    for (const auto& func : cleaned) {
        auto method = parseFunction(builder, func);
        if (method.mVirtual) {
            Expects(!method.mStatic);
            methods.emplace_back(std::move(method));
        }
    }

    return methods;
}

}
