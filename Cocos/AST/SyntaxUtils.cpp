/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "SyntaxUtils.h"

namespace Cocos::Meta {

size_t findEndMatch(std::string_view str, const char left, const char right, size_t pos) {
    int count = 0;
    Expects(pos < str.size());
    Expects(left == str[pos]);

    for (; pos != str.size(); ++pos) {
        if (left == str[pos]) {
            ++count;
        }
        if (right == str[pos]) {
            --count;
        }
        if (count == 0) {
            return pos;
        }
    }
    Expects(false); // not matched
    return std::string_view::npos;
}

size_t findBegMatch(std::string_view str, const char left, const char right, size_t pos) {
    int count = 0;
    Expects(pos < str.size());
    Expects(right == str[pos]);

    if (right == str[pos])
        ++count;

    while (pos-- > 0) {
        if (right == str[pos]) {
            ++count;
        }
        if (left == str[pos])
            --count;
        if (count == 0) {
            return pos;
        }
    }
    Expects(false); // not matched
    return std::string_view::npos;
}

// Typename
void removeParenthesis(std::pmr::string& name) {
    boost::algorithm::trim(name);
    while (!name.empty() && name.front() == '(' && name.back() == ')') {
        name.pop_back();
        name.erase(name.begin());
    }
}

void convertTypename(std::pmr::string& path) {
    boost::algorithm::replace_all(path, "::", "/");
    boost::algorithm::replace_all(path, "\t", " ");
    boost::algorithm::replace_all(path, "\n", " ");
    Expects(!boost::algorithm::contains(path, "\r"));

    boost::algorithm::trim(path);

    while (!path.empty() && path.front() == '(' && path.back() == ')') {
        path.pop_back();
        path.erase(path.begin());
    }

    while (boost::algorithm::contains(path, ", ")) {
        boost::algorithm::replace_all(path, ", ", ",");
    }

    while (boost::algorithm::contains(path, " ,")) {
        boost::algorithm::replace_all(path, " ,", ",");
    }

    while (boost::algorithm::contains(path, "< ")) {
        boost::algorithm::replace_all(path, "< ", "<");
    }

    while (boost::algorithm::contains(path, " <")) {
        boost::algorithm::replace_all(path, " <", "<");
    }

    while (boost::algorithm::contains(path, "> ")) {
        boost::algorithm::replace_all(path, "> ", ">");
    }

    while (boost::algorithm::contains(path, " >")) {
        boost::algorithm::replace_all(path, " >", ">");
    }

    while (boost::algorithm::contains(path, "/ ")) {
        boost::algorithm::replace_all(path, "/ ", "/");
    }

    while (boost::algorithm::contains(path, " /")) {
        boost::algorithm::replace_all(path, " /", "/");
    }

    while (boost::algorithm::contains(path, "= ")) {
        boost::algorithm::replace_all(path, "= ", "=");
    }

    while (boost::algorithm::contains(path, " =")) {
        boost::algorithm::replace_all(path, " =", "=");
    }

    while (boost::algorithm::contains(path, "& ")) {
        boost::algorithm::replace_all(path, "& ", "&");
    }

    while (boost::algorithm::contains(path, " &")) {
        boost::algorithm::replace_all(path, " &", "&");
    }

    while (boost::algorithm::contains(path, "* ")) {
        boost::algorithm::replace_all(path, "* ", "*");
    }

    while (boost::algorithm::contains(path, " *")) {
        boost::algorithm::replace_all(path, " *", "*");
    }

    while (boost::algorithm::contains(path, "[[ ")) {
        boost::algorithm::replace_all(path, "[[ ", "[[");
    }

    while (boost::algorithm::contains(path, " [[")) {
        boost::algorithm::replace_all(path, " [[", "[[");
    }

    while (boost::algorithm::contains(path, "]] ")) {
        boost::algorithm::replace_all(path, "]] ", "]]");
    }

    while (boost::algorithm::contains(path, " ]]")) {
        boost::algorithm::replace_all(path, " ]]", "]]");
    }

    while (boost::algorithm::contains(path, "  ")) {
        boost::algorithm::replace_all(path, "  ", " ");
    }

    boost::algorithm::trim(path);
}

std::pmr::string convertTypename(std::string_view name,
    std::pmr::memory_resource* scratch) {
    std::pmr::string type(name, scratch);
    convertTypename(type);
    return type;
}

bool validateTypename(std::string_view name) {
    if (boost::algorithm::contains(name, "::"))
        return false;

    if (name.find_first_of("\t\n\r{}[]") != name.npos)
        return false;

    int count = 0;
    for (const auto& c : name) {
        if (c == '(')
            ++count;
        if (c == ')')
            --count;
    }
    if (count)
        return false;

    for (const auto& c : name) {
        if (c == '<')
            ++count;
        if (c == '>')
            --count;
    }
    if (count)
        return false;

    return true;
}

// General Path
std::string_view parentPath(std::string_view path) {
    if (path.empty())
        return path;

    std::string_view res;

    auto pos = path.find_last_of('/');
    if (pos == path.size() - 1) {
        if (pos != 0) {
            pos = path.find_last_of('/', pos - 1);
        }
    }

    if (pos != std::string_view::npos) {
        res = path.substr(0, pos);
    }

    return res;
}

// TypePath
bool isTypePath(std::string_view typePath) {
    return typePath.empty() || typePath.front() == '/';
}

std::string_view extractScope(std::string_view name) {
    Expects(!name.empty());
    Expects(name.back() != '/');

    std::string_view parent;

    if (auto range = boost::algorithm::find_last(name, "/"); !range.empty()) {
        auto count = std::distance(name.begin(), range.begin());
        parent = name.substr(0, count);
    }

    return parent;
}

std::string_view getDependentPath(std::string_view scope, std::string_view typePath) {
    if (scope.empty()) {
        return typePath;
    }

    Expects(scope.front() == '/');
    Expects(typePath.front() == '/');

    for (size_t pos = 1; pos != scope.size() && pos != scope.npos;) {
        auto end0 = scope.find_first_of('/', pos);
        auto end1 = typePath.find_first_of('/', pos);
        auto ns0 = scope.substr(pos, end0 - pos);
        auto ns1 = typePath.substr(pos, end1 - pos);

        if (ns0 != ns1) {
            // scopes are different now
            return typePath.substr(pos);
        }

        if (end0 == scope.npos) {
            // scope ended first
            if (end1 == typePath.npos) {
                // scope == typePath
                return {};
            } else {
                // output full scope
                return typePath.substr(end1 + 1);
            }
        }
        pos = end0 + 1;
    }
    // should never reach here
    Expects(false);
    return typePath;
}

std::string_view getTemplateName(std::string_view instanceName) {
    // find name name
    auto pos = instanceName.find_first_of("<");
    Ensures(pos != instanceName.npos);
    auto name = instanceName.substr(0, pos);
    Expects(pos + 1 < instanceName.size());
    Ensures(!name.empty());
    return name;
}

void extractTemplate(std::string_view instanceName,
    std::pmr::string& name,
    std::pmr::vector<std::pmr::string>& parameters) {
    name.clear();
    parameters.clear();

    // find name name
    name = getTemplateName(instanceName);

    // process parameters
    Expects(parameters.empty());
    bool bInstance = false;
    int count = 0;

    auto addParameter = [&](std::string_view range) {
        // not instance, might be parameter
        std::pmr::string candidate(range, parameters.get_allocator());
        boost::algorithm::trim(candidate);
        // might be white space
        if (!candidate.empty()) {
            Expects(validateTypename(candidate));
            parameters.emplace_back(std::move(candidate));
        }
    };

    size_t paramBeg = name.size() + 1;
    size_t paramEnd = paramBeg;
    for (auto i = paramBeg; i + 1 != instanceName.size(); ++i, ++paramEnd) {
        if (instanceName[i] == ',') {
            if (bInstance) {
                // comma in instance, ignore
                continue;
            }
            addParameter(instanceName.substr(paramBeg, paramEnd - paramBeg));
            // skip comma
            paramBeg = paramEnd + 1;
        } else {
            if (instanceName[i] == '<') {
                if (bInstance == false) {
                    Expects(count == 0);
                    bInstance = true;
                }
                ++count;
            }
            if (instanceName[i] == '>') {
                Expects(bInstance);
                --count;
            }

            if (bInstance && count == 0) {
                auto range = instanceName.substr(paramBeg, paramEnd + 1 - paramBeg);
                Expects(isInstance(range));

                addParameter(range);
                bInstance = false;
                paramBeg = paramEnd + 1;
            }
        }
    }
    addParameter(instanceName.substr(paramBeg, paramEnd - paramBeg));

    Ensures(!name.empty());
}

std::string_view extractName(std::string_view typePath) {
    if (typePath.empty())
        return {};

    auto pos = typePath.find_last_of("/");
    Expects(pos != typePath.npos);
    Expects(pos != typePath.size() - 1);
    return typePath.substr(pos + 1);
}

ParameterTraits getParameterTraits(std::string_view typePath) {
    ParameterTraits traits{};
    if (typePath.starts_with("const ")) {
        typePath = typePath.substr(6);
        traits.mConst = true;
    }
    if (typePath.starts_with("volatile ")) {
        typePath = typePath.substr(6);
        traits.mVolatile = true;
    }
    Expects(!(traits.mConst && traits.mVolatile));

    if (typePath.ends_with("&")) {
        typePath = typePath.substr(0, typePath.size() - 1);
        traits.mReference = true;
    }
    if (typePath.ends_with("*")) {
        typePath = typePath.substr(0, typePath.size() - 1);
        traits.mPointer = true;
    }
    return traits;
}

std::string_view removeCvPointerRef(std::string_view typePath) {
    bool bConst = false;
    bool bVolatile = false;
    if (typePath.starts_with("const ")) {
        typePath = typePath.substr(6);
        bConst = true;
    }
    if (typePath.starts_with("volatile ")) {
        typePath = typePath.substr(6);
        bVolatile = true;
    }
    Expects(!(bConst && bVolatile));

    if (typePath.ends_with("&")) {
        typePath = typePath.substr(0, typePath.size() - 1);
    }
    if (typePath.ends_with("*")) {
        typePath = typePath.substr(0, typePath.size() - 1);
    }

    Expects(!boost::algorithm::contains(typePath, "*"));
    return typePath;
}

// Struct
std::pmr::string camelToVariable(std::string_view camelName,
    std::pmr::memory_resource* scratch) {
    std::pmr::string name(camelName, scratch);
    for (uint32_t i = 0; i != name.size(); ++i) {
        if (i == 0) {
            name[i] = tolower(name[i]);
        }

        if (i + 1 < name.size()) {
            if (std::isupper(name[i + 1]))
                name[i] = tolower(name[i]);
            else
                break;
        } else {
            if (std::isupper(name[i]))
                name[i] = tolower(name[i]);
        }
    }
    return name;
}

std::pmr::string getParameterName(std::string_view member,
    std::pmr::memory_resource* scratch) {
    if (member.size() < 2)
        throw std::runtime_error("member format incorrect");

    auto name = camelToVariable(member.substr(1), scratch);
    name.append("In");
    return name;
}

std::pmr::string getParameterPrefix(std::string_view member,
    std::pmr::memory_resource* scratch) {
    if (member.size() < 2)
        throw std::runtime_error("member format incorrect");

    return camelToVariable(member.substr(1), scratch);
}

// Syntax
bool isInstance(std::string_view name) {
    if (name.back() == '>') {
        return true;
    }
    return false;
}

// Generation
std::pmr::string getCppPath(std::string_view typePath,
    std::pmr::memory_resource* mr) {
    if (typePath.empty())
        return std::pmr::string(mr);

    Expects(!typePath.empty());

    if (typePath.front() == '/')
        typePath = typePath.substr(1);

    auto result = std::pmr::string(typePath, mr);
    boost::algorithm::replace_all(result, "/", "::");

    return result;
}

std::pmr::string getVariableName(std::string_view typeName,
    std::pmr::memory_resource* mr) {
    if (typeName.empty()) {
        return std::pmr::string(mr);
    } else {
        auto type = std::pmr::string(typeName, mr);
        type.front() = tolower(typeName.front());
        return type;
    }
}

std::string_view convertTag(std::string_view tagName) {
    Expects(!tagName.empty() && tagName.back() == '_');
    if (!tagName.empty() && tagName.back() == '_') {
        return tagName.substr(0, tagName.size() - 1);
    }
    return tagName;
}

std::pmr::string getTagType(std::string_view tagName, std::pmr::memory_resource* scratch) {
    std::pmr::string typeName(convertTag(tagName));
    typeName.append("Tag");
    return typeName;
}

std::pmr::string getTypescriptTagType(std::string_view tagName, std::pmr::memory_resource* scratch) {
    return std::pmr::string(convertTag(tagName), scratch);
}

std::pmr::string getTagVariableName(std::string_view typeName,
    std::pmr::memory_resource* mr) {
    auto name = convertTag(typeName);
    return getVariableName(name, mr);
}

std::pmr::string getRelativePath(std::string_view current0, std::string_view target0,
    std::pmr::memory_resource* mr) {
    auto currentFile = current0.find_last_of('/');
    auto targetFile = target0.find_last_of('/');

    auto current = current0.substr(0, currentFile);
    auto target = target0.substr(0, targetFile);

    auto pos0 = current.find_first_of('/');
    auto pos1 = target.find_first_of('/');
    for (;pos0 != current.npos && pos1 != target.npos;) {
        
        auto end0 = current.find_first_of('/', pos0 + 1);
        auto end1 = target.find_first_of('/', pos1 + 1);

        if (current.substr(pos0, end0 - pos0) == target.substr(pos1, end1 - pos1)) {
            pos0 = end0;
            pos1 = end1;
        } else {
            break;
        }
    }
    
    std::pmr::string path(mr);
    if (pos0 == current.npos) {
        path += "./";
    } else {
        auto str = current.substr(pos0);
        for (const auto c : str) {
            if (c == '/') {
                path += "../";
            }
        }
    }

    if (pos1 != target.npos) {
        path += target.substr(pos1 + 1);
        path += '/';
    }

    path += target0.substr(targetFile + 1);

    return path;
}

}
