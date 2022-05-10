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

#include "TypescriptDefaultValues.h"
#include "BuilderTypes.h"
#include "DSL.h"

namespace Cocos::Meta {

void projectTypescriptDefaultValues(ModuleBuilder& builder) {
    PROJECT_TS(bool, boolean);
    PROJECT_TS(int, number);
    PROJECT_TS(float, number);
    PROJECT_TS(double, number);

    PROJECT_TS(char, number);
    PROJECT_TS(char8_t, number);

    PROJECT_TS(size_t, BigInt);

    PROJECT_TS(int8_t, number);
    PROJECT_TS(int16_t, number);
    PROJECT_TS(int32_t, number);
    PROJECT_TS(int64_t, BigInt);

    PROJECT_TS(uint8_t, number);
    PROJECT_TS(uint16_t, number);
    PROJECT_TS(uint32_t, number);
    PROJECT_TS(uint64_t, BigInt);

    PROJECT_TS(uintptr_t, BigInt);

    PROJECT_TS(std::byte, number);

    PROJECT_TS(std::string, string);
    PROJECT_TS(std::u8string, string);

    PROJECT_TS(std::pmr::string, string);
    PROJECT_TS(std::pmr::u8string, string);

    PROJECT_TS(std::list, Set);
    PROJECT_TS(std::map, Map);
    PROJECT_TS(std::unordered_map, Map);

    PROJECT_TS(std::pmr::list, Set);
    PROJECT_TS(std::pmr::map, Map);
    PROJECT_TS(std::pmr::unordered_map, Map);

    PROJECT_TS(boost::container::list, Set);
    PROJECT_TS(boost::container::pmr::list, Set);

    PROJECT_TS(std::vector<std::byte>, Uint8Array);
    PROJECT_TS(std::vector<int>, Int32Array);
    PROJECT_TS(std::vector<int8_t>, Int8Array);
    PROJECT_TS(std::vector<int16_t>, Int16Array);
    PROJECT_TS(std::vector<int32_t>, Int32Array);
    PROJECT_TS(std::vector<int64_t>, BigInt64Array);
    PROJECT_TS(std::vector<uint8_t>, Uint8Array);
    PROJECT_TS(std::vector<uint16_t>, Uint16Array);
    PROJECT_TS(std::vector<uint32_t>, Uint32Array);
    PROJECT_TS(std::vector<uint64_t>, BigUint64Array);

    PROJECT_TS(std::pmr::vector<std::byte>, Uint8Array);
    PROJECT_TS(std::pmr::vector<int>, Int32Array);
    PROJECT_TS(std::pmr::vector<int8_t>, Int8Array);
    PROJECT_TS(std::pmr::vector<int16_t>, Int16Array);
    PROJECT_TS(std::pmr::vector<int32_t>, Int32Array);
    PROJECT_TS(std::pmr::vector<int64_t>, BigInt64Array);
    PROJECT_TS(std::pmr::vector<uint8_t>, Uint8Array);
    PROJECT_TS(std::pmr::vector<uint16_t>, Uint16Array);
    PROJECT_TS(std::pmr::vector<uint32_t>, Uint32Array);
    PROJECT_TS(std::pmr::vector<uint64_t>, BigUint64Array);

    PROJECT_TS(ccstd::pmr::vector<std::byte>, Uint8Array);
    PROJECT_TS(ccstd::pmr::vector<int>, Int32Array);
    PROJECT_TS(ccstd::pmr::vector<int8_t>, Int8Array);
    PROJECT_TS(ccstd::pmr::vector<int16_t>, Int16Array);
    PROJECT_TS(ccstd::pmr::vector<int32_t>, Int32Array);
    PROJECT_TS(ccstd::pmr::vector<int64_t>, BigInt64Array);
    PROJECT_TS(ccstd::pmr::vector<uint8_t>, Uint8Array);
    PROJECT_TS(ccstd::pmr::vector<uint16_t>, Uint16Array);
    PROJECT_TS(ccstd::pmr::vector<uint32_t>, Uint32Array);
    PROJECT_TS(ccstd::pmr::vector<uint64_t>, BigUint64Array);

    PROJECT_TS(cc::PmrMultiMap, Map);
    PROJECT_TS(cc::TransparentMap, Map);
    PROJECT_TS(cc::TransparentMultiMap, Map);
    PROJECT_TS(cc::PmrTransparentMap, Map);
    PROJECT_TS(cc::PmrTransparentMultiMap, Map);
    PROJECT_TS(cc::FlatMap, Map);
    PROJECT_TS(cc::FlatMultiMap, Map);
    PROJECT_TS(cc::PmrFlatMap, Map);
    PROJECT_TS(cc::PmrFlatMultiMap, Map);
    PROJECT_TS(cc::PmrUnorderedMap, Map);
    PROJECT_TS(cc::PmrUnorderedMultiMap, Map);
    PROJECT_TS(cc::UnorderedStringMap, Map);
    PROJECT_TS(cc::UnorderedStringMultiMap, Map);
    PROJECT_TS(cc::PmrUnorderedStringMap, Map);
    PROJECT_TS(cc::PmrUnorderedStringMultiMap, Map);

    PROJECT_TS(cc::TransparentSet, Set);
    PROJECT_TS(cc::TransparentMultiSet, Set);
    PROJECT_TS(cc::PmrTransparentSet, Set);
    PROJECT_TS(cc::PmrTransparentMultiSet, Set);
    PROJECT_TS(cc::FlatSet, Set);
    PROJECT_TS(cc::FlatMultiSet, Set);
    PROJECT_TS(cc::PmrFlatSet, Set);
    PROJECT_TS(cc::PmrFlatMultiSet, Set);
    PROJECT_TS(cc::PmrUnorderedSet, Set);
    PROJECT_TS(cc::PmrUnorderedMultiSet, Set);
    PROJECT_TS(cc::UnorderedStringSet, Set);
    PROJECT_TS(cc::UnorderedStringMultiSet, Set);
    PROJECT_TS(cc::PmrUnorderedStringSet, Set);
    PROJECT_TS(cc::PmrUnorderedStringMultiSet, Set);

    PROJECT_TS(ccstd::string, string);
    PROJECT_TS(ccstd::pmr::string, string);
    PROJECT_TS(ccstd::pmr::map, Map);
    PROJECT_TS(ccstd::pmr::unordered_map, Map);
}

}
