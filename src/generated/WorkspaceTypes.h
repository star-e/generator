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
#pragma once // NOLINT(portability-avoid-pragma-once)
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "WorkspaceFwd.h"
#include "generated/SyntaxTypes.h"

namespace cc {

namespace codegen {

struct ModuleInput {
    Module syntax;
    std::string nativeHeader;
    std::string forwardHeader;
    std::string typescriptModule;
    bool typescriptSettings{false};
    bool modelOnly() const noexcept { return nativeHeader.empty() && typescriptModule.empty(); }
};

struct RecordOutput {
    std::filesystem::path nativeRoot;
    std::filesystem::path header;
    std::filesystem::path forwardHeader;
    std::set<std::string> forwardIncludes;
    std::optional<std::filesystem::path> namesHeader;
    std::vector<std::filesystem::path> importedNamesHeaders;
    std::optional<std::filesystem::path> serializationHeader;
    std::optional<std::filesystem::path> jsbHeader;
    std::optional<std::filesystem::path> swigFile;
    std::set<std::string> jsbRecords;
    std::set<std::filesystem::path> jsbIncludes;
    std::optional<std::filesystem::path> typescript;
    std::set<std::string> namedEnums;
    std::set<std::string> runtimeEnums;
    InterfaceBindings interfaceBindings;
    std::map<std::string, TypeScriptImport> bases;
    std::map<std::string, TypeScriptClassMethods> classMethods;
    std::vector<TypeScriptNamedImport> typescriptImports;
    std::map<std::string, std::filesystem::path> externalClasses;
    std::filesystem::path poolModule;
    std::vector<std::pair<std::string, TypeScriptImport>> poolDependencies;
    std::map<std::string, PoolBinding> pools;
};

struct ModuleOutput {
    RecordOutput types;
    std::optional<std::filesystem::path> settingsFile;
};

struct SettingsBinding {
    std::set<std::string> interfaceOnly;
    std::set<std::string> runtimeEnums;
    std::set<std::string> registerEnums;
    std::map<std::string, std::map<std::string, std::string>> renamedFrom;
};

} // namespace codegen

} // namespace cc
