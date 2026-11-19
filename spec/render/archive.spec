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

binding typescript {
    OutputArchive => interface {
        method writeBool => b;
        method writeNumber => n;
        method writeString => s;
    };
    InputArchive => interface {
        method readBool => b;
        method readNumber => n;
        method readString => s;
    };
}
