extern module HostTypes;

namespace std {
    header "cstddef" { struct size_t; }
    header "string" { @string class string; }
    header "vector" { @container("array") template<T> class vector; }
    header "optional" { template<T> class optional; }
}

namespace std::filesystem {
    header "filesystem" { class path; }
}
