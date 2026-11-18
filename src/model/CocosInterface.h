#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cc::codegen {

struct SourceLocation {
    std::filesystem::path file;
    std::size_t line;
    std::size_t column;
    std::string describe() const;
};

struct TypeRef {
    SourceLocation location;
    std::string name;
    bool pointer{};
};

struct Parameter {
    TypeRef type;
    std::string name;
};

struct Method {
    TypeRef result;
    std::string name;
    std::vector<Parameter> parameters;
    bool isConst{};
    bool isNoexcept{};
};

struct Interface {
    std::string name;
    std::vector<Method> methods;
};

struct Module {
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<Interface> interfaces;
};

struct ExternalType {
    std::string header;
    std::optional<std::string> typescript;
};
using TypeTable = std::map<std::string, ExternalType>;
using Projection = std::map<std::string, std::map<std::string, std::optional<std::string>>>;
using Files = std::map<std::filesystem::path, std::string>;

struct OutputLayout {
    std::filesystem::path nativeRoot;
    std::filesystem::path includeDir;
    std::string prefix;
    std::filesystem::path typescript;
};

} // namespace cc::codegen
