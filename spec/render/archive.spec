module ArchiveInterface;
namespace cc::render {
    @archive(output) interface OutputArchive {
        virtual void writeBool(bool value) = 0;
        virtual void writeNumber(double value) = 0;
        virtual void writeString(std::string_view value) = 0;
        @cppOnly virtual boost::container::pmr::memory_resource* scratch() const noexcept = 0;
    }
    @archive(input) interface InputArchive {
        virtual bool readBool() = 0;
        virtual double readNumber() = 0;
        virtual std::string_view readString() = 0;
        @cppOnly virtual boost::container::pmr::memory_resource* scratch() const noexcept = 0;
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
