module Syntax;
import HostTypes;
import SourceLocation;
import TypeGraph;

namespace cc::codegen {
    struct TypeRef {
        location: SourceLocation;
        name: std::string;
        pointer: bool = false;
        arguments: std::vector<TypeRef>;
        constPointee: bool = false;
        reference: bool = false;
        hasTemplateArguments: bool = false;
    }

    struct Parameter {
        type: TypeRef;
        name: std::string;
        byConstReference: bool = false;
        defaultValue: std::string;
    }

    struct Method {
        result: TypeRef;
        name: std::string;
        parameters: std::vector<Parameter>;
        isConst: bool = false;
        isNoexcept: bool = false;
        cppOnly: bool = false;
        cppBody: std::string;
        isStatic: bool = false;
        isPureVirtual: bool = false;
        isExplicitConversion: bool = false;
        explicitCppOnly: bool = false;
        accessor: AccessorKind = AccessorKind::None;
        optionalMethod: bool = false;
        resultPresence: ResultPresence = ResultPresence::Required;
        cppPrivate: bool = false;
        documentation: std::string;
    }

    struct Field {
        type: TypeRef;
        name: std::string;
        initializer: std::string;
        isReadonly: bool = false;
        isNullable: bool = false;
        transient: bool = false;
        cppOnly: bool = false;
        notElement: bool = false;
        isMutable: bool = false;
        cppPrivate: bool = false;
        documentation: std::string;
    }

    struct Constructor {
        location: SourceLocation;
        fields: std::vector<std::string>;
        defaults: bool = false;
        cppOnly: bool = false;
        custom: bool = false;
        isNoexcept: bool = false;
        cppPrivate: bool = false;
        documentation: std::string;
    }
}
