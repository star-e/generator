#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include <boost/container/pmr/global_resource.hpp>
#include <fstream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    auto* memory = boost::container::pmr::new_delete_resource();
    const std::string_view command(argv[1]);
    if (command == "write") {
        std::ofstream file(argv[2], std::ios::binary);
        cc::render::BinaryOutputArchive archive(file, memory);
        archive.writeBool(true);
        archive.writeBool(false);
        archive.writeNumber(-17.25);
        archive.writeString("Cocos archive");
        file.close();
        return file.good() ? 0 : 1;
    }
    if (command == "read") {
        std::ifstream file(argv[2], std::ios::binary);
        cc::render::BinaryInputArchive archive(file, memory);
        const bool matches = archive.readBool() && !archive.readBool() &&
                             archive.readNumber() == -17.25 && archive.readString() == "Cocos archive";
        return matches && file.good() && file.peek() == std::char_traits<char>::eof() ? 0 : 1;
    }
    return 2;
}
