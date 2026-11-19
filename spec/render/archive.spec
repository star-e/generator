module ArchiveInterface;
namespace cc::render {
    interface OutputArchive {
        void writeBool(bool value);
        void writeNumber(double value);
        void writeString(std::string_view value);
        @cppOnly boost::container::pmr::memory_resource* scratch() const noexcept;
    }
    interface InputArchive {
        bool readBool();
        double readNumber();
        std::string_view readString();
        @cppOnly boost::container::pmr::memory_resource* scratch() const noexcept;
    }
}
