module SourceLocation;
namespace cc::codegen {
    struct SourceLocation {
        file: std::filesystem::path;
        line: std::size_t = 0;
        column: std::size_t = 0;

        std::string describe() const;
    }
}
