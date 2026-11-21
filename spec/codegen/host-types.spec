extern module HostTypes;

namespace std {
    header "cstddef" { struct size_t; }
    header "string" { @string class string; }
}

namespace std::filesystem {
    header "filesystem" { class path; }
}
