/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once
#include <gsl/assert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

namespace Cocos {

inline size_t getFileSize(std::ifstream& ifs) {
    auto pos = ifs.tellg();
    ifs.seekg(0, ifs.end);
    auto sz = ifs.tellg();
    ifs.seekg(pos, ifs.beg);
    return sz;
}

// http://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
// Jerry Coffin
inline std::string readFile(const std::filesystem::path& file, bool mustExists = true) {
    std::ifstream ifs(file);
    if (mustExists) {
        ifs.exceptions(std::ios_base::failbit);
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

inline void readFileBuffer(const std::filesystem::path& file, std::pmr::string& buffer) {
    std::ifstream ifs(file, std::ios::binary);
    ifs.exceptions(std::ifstream::failbit);

    auto sz = getFileSize(ifs);
    buffer.resize(sz);
    ifs.read(buffer.data(), sz);
}

inline bool updateFile(const std::filesystem::path& file, std::string_view content) {
    std::string orig = readFile(file, false);
    if (orig != content) {
        std::ofstream ofs(file);
        ofs.exceptions(std::ostream::failbit);
        ofs.write(content.data(), content.size());
        return true;
    }
    return false;
}

inline void readBinary(const std::filesystem::path& file, std::pmr::string& buffer) {
    std::ifstream ifs(file, std::ios::binary);
    ifs.exceptions(std::ios_base::failbit);
    auto sz = getFileSize(ifs);
    buffer.resize(sz);
    ifs.read(buffer.data(), sz);
}

inline std::string readBinary(const std::filesystem::path& file, bool mustExists = true) {
    std::ifstream ifs(file, std::ios::binary);
    if (mustExists)
        ifs.exceptions(std::ios_base::failbit);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

inline void readBinary(const std::filesystem::path& file, std::pmr::string& buffer, bool mustExists = true) {
    std::ifstream ifs(file, std::ios::binary);
    if (mustExists)
        ifs.exceptions(std::ios_base::failbit);
    auto sz = getFileSize(ifs);
    buffer.resize(sz);
    ifs.read(buffer.data(), sz);
}

inline bool updateBinary(const std::filesystem::path& file, std::string_view content) {
    std::string orig = readBinary(file, false);
    if (orig != content) {
        std::ofstream ofs(file, std::ios::binary);
        ofs.exceptions(std::ostream::failbit);
        ofs.write(content.data(), content.size());
        return true;
    }
    return false;
}

inline bool updateLargeBinary(const std::filesystem::path& file, std::string_view content) {
    bool needUpdate = false;
    {
        std::ifstream ifs(file, std::ios::binary);
        const auto sz = getFileSize(ifs);

        if (sz != content.size()) {
            needUpdate = true;
        } else {
            constexpr size_t BufferSize = 1024 * 1024;

            std::string target;
            target.reserve(BufferSize);

            for (size_t pos = 0; pos != sz;) {
                size_t bytesRead = BufferSize;
                if (pos + bytesRead > sz) {
                    bytesRead = sz - pos;
                }

                target.resize(bytesRead);
                ifs.read(target.data(), target.size());
                std::string_view source(content.data() + pos, bytesRead);

                Ensures(source.size() == target.size());
                Ensures(source.size() == bytesRead);

                if (memcmp(source.data(), target.data(), bytesRead)) {
                    needUpdate = true;
                    break;
                }
                
                pos += bytesRead;
            }
        }
    }

    if (needUpdate) {
        std::ofstream ofs(file, std::ios::binary);
        ofs.exceptions(std::ostream::failbit);
        ofs.write(content.data(), content.size());
        return true;
    }

    return false;
}

inline bool copyFile(const std::filesystem::path& src, const std::filesystem::path& dst) {
    auto content = readBinary(src);
    if (content.empty())
        return false;

    const auto& folder = dst.parent_path();
    if (!exists(folder)) {
        create_directories(folder);
    }
    return updateBinary(dst, content);
}

[[nodiscard]] inline boost::iostreams::stream<boost::iostreams::basic_array_source<char>>
make_istream(std::string_view input) {
    return boost::iostreams::stream<boost::iostreams::basic_array_source<char>>{ input.data(), input.size() };
}

[[nodiscard]] inline boost::iostreams::stream<boost::iostreams::back_insert_device<std::pmr::string>>
make_ostream(std::pmr::string& output) {
    return boost::iostreams::stream<boost::iostreams::back_insert_device<std::pmr::string>>{ output };
}

}
