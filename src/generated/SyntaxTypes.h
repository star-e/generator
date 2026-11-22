/*
 Copyright (c) 2021-2024 Xiamen Yaji Software Co., Ltd.

 https://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

/**
 * ========================= !DO NOT CHANGE THE FOLLOWING SECTION MANUALLY! =========================
 * The following section is auto-generated.
 * ========================= !DO NOT CHANGE THE FOLLOWING SECTION MANUALLY! =========================
 */
#pragma once
#include "generated/SourceLocation.h"
#include "generated/TypeGraphTypes.h"
#include <string>
#include <vector>
#include "SyntaxFwd.h"

namespace cc {

namespace codegen {

struct TypeRef {
    SourceLocation location;
    std::string name;
    bool pointer{false};
    std::vector<TypeRef> arguments;
    bool constPointee{false};
    bool reference{false};
    bool hasTemplateArguments{false};
};

struct Parameter {
    TypeRef type;
    std::string name;
    bool byConstReference{false};
    std::string defaultValue;
};

struct Method {
    TypeRef result;
    std::string name;
    std::vector<Parameter> parameters;
    bool isConst{false};
    bool isNoexcept{false};
    bool cppOnly{false};
    std::string cppBody;
    bool isStatic{false};
    bool isPureVirtual{false};
    bool isExplicitConversion{false};
    bool explicitCppOnly{false};
    AccessorKind accessor{AccessorKind::None};
    bool optionalMethod{false};
    ResultPresence resultPresence{ResultPresence::Required};
    bool cppPrivate{false};
    std::string documentation;
};

struct Field {
    TypeRef type;
    std::string name;
    std::string initializer;
    bool isReadonly{false};
    bool isNullable{false};
    bool transient{false};
    bool cppOnly{false};
    bool notElement{false};
    bool isMutable{false};
    bool cppPrivate{false};
    std::string documentation;
};

struct Constructor {
    SourceLocation location;
    std::vector<std::string> fields;
    bool defaults{false};
    bool cppOnly{false};
    bool custom{false};
    bool isNoexcept{false};
    bool cppPrivate{false};
    std::string documentation;
};

} // namespace codegen

} // namespace cc
