module Workspace;
import HostTypes;
import Syntax;

namespace cc::codegen {
    struct ModuleInput {
        syntax: Module;
        nativeHeader: std::string;
        forwardHeader: std::string;
        typescriptModule: std::string;
        typescriptSettings: bool = false;

        bool modelOnly() const noexcept { return nativeHeader.empty() && typescriptModule.empty(); }
    }

    struct RecordOutput {
        nativeRoot: std::filesystem::path;
        header: std::filesystem::path;
        forwardHeader: std::filesystem::path;
        forwardIncludes: std::set<std::string>;
        namesHeader: std::optional<std::filesystem::path>;
        importedNamesHeaders: std::vector<std::filesystem::path>;
        serializationHeader: std::optional<std::filesystem::path>;
        jsbHeader: std::optional<std::filesystem::path>;
        swigFile: std::optional<std::filesystem::path>;
        jsbRecords: std::set<std::string>;
        jsbIncludes: std::set<std::filesystem::path>;
        typescript: std::optional<std::filesystem::path>;
        namedEnums: std::set<std::string>;
        runtimeEnums: std::set<std::string>;
        interfaceBindings: InterfaceBindings;
        bases: std::map<std::string, TypeScriptImport>;
        classMethods: std::map<std::string, TypeScriptClassMethods>;
        typescriptImports: std::vector<TypeScriptNamedImport>;
        externalClasses: std::map<std::string, std::filesystem::path>;
        poolModule: std::filesystem::path;
        poolDependencies: std::vector<std::pair<std::string, TypeScriptImport>>;
        pools: std::map<std::string, PoolBinding>;
    }

    struct ModuleOutput {
        types: RecordOutput;
        settingsFile: std::optional<std::filesystem::path>;
    }

    struct SettingsBinding {
        interfaceOnly: std::set<std::string>;
        runtimeEnums: std::set<std::string>;
        registerEnums: std::set<std::string>;
        renamedFrom: std::map<std::string, std::map<std::string, std::string>>;
    }
}
