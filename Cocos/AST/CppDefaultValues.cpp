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

#include "CppDefaultValues.h"
#include "BuilderTypes.h"
#include "DSL.h"

namespace Cocos::Meta {

void addCppDefaultValues(ModuleBuilder& builder) {
    // Value
    VALUE(void);

    VALUE(bool);

    VALUE(int);
    VALUE(float);
    VALUE(double);

    VALUE(char);
    VALUE(char8_t);
    VALUE(char16_t);
    VALUE(char32_t);
    VALUE(wchar_t);

    VALUE(size_t);

    VALUE(int8_t);
    VALUE(int16_t);
    VALUE(int32_t);
    VALUE(int64_t);

    VALUE(uint8_t);
    VALUE(uint16_t);
    VALUE(uint32_t);
    VALUE(uint64_t);

    VALUE(uintptr_t);

    NAMESPACE(std) {
        VALUE(byte);
    }

    // Struct
    NAMESPACE(std) {
        IMPORT_CLASS(string_view);
        IMPORT_CLASS(u8string_view);

        IMPORT_CLASS(string);
        IMPORT_CLASS(u8string);

        IMPORT_CLASS(monostate);
        IMPORT_CLASS(mutex);
        IMPORT_CLASS(thread);
        IMPORT_CLASS(locale);

        IMPORT_CLASS(istream);

        NAMESPACE(chrono) {
            IMPORT_CLASS(system_clock);
            IMPORT_CLASS(high_resolution_clock) {
                IMPORT_CLASS(time_point);
            }
            IMPORT_CLASS(nanoseconds);
            IMPORT_CLASS(microseconds);
            IMPORT_CLASS(seconds);
        }

        NAMESPACE(pmr) {
            IMPORT_PMR_CLASS(string);
            IMPORT_PMR_CLASS(u8string);

            IMPORT_CLASS(memory_resource);
            IMPORT_CLASS(unsynchronized_pool_resource);
            IMPORT_CLASS(synchronized_pool_resource);
            IMPORT_CLASS(monotonic_buffer_resource);
        }

        NAMESPACE(filesystem) {
            IMPORT_CLASS(path);
        }

        CONTAINER(optional);
        CONTAINER(shared_ptr);
        CONTAINER(weak_ptr);
        CONTAINER(vector);
        CONTAINER(list);
        CONTAINER(deque);
        CONTAINER(set);
        CONTAINER(unordered_set);
        CONTAINER(unique_ptr);
        MAP(map);
        MAP(unordered_map);

        NAMESPACE(pmr) {
            PMR_CONTAINER(vector);
            PMR_CONTAINER(list);
            PMR_CONTAINER(deque);
            PMR_CONTAINER(set);
            PMR_CONTAINER(unordered_set);
            PMR_MAP(map);
            PMR_MAP(unordered_map);
        }
    }

    NAMESPACE(boost) {
        IMPORT_CLASS(blank);
        IMPORT_CLASS(default_color_type);

        NAMESPACE(uuids) {
            IMPORT_STRUCT(uuid);
        }
        NAMESPACE(asio) {
            IMPORT_CLASS(io_context) {
                IMPORT_CLASS(work);
                IMPORT_CLASS(strand);
            }
        }
        NAMESPACE(container) {
            CONTAINER(vector);
            CONTAINER(list);
            CONTAINER(set);
            NAMESPACE(pmr) {
                IMPORT_CLASS(memory_resource);
                PMR_CONTAINER(vector);
                PMR_CONTAINER(list);
                PMR_CONTAINER(set);
                PMR_MAP(map);
            }
        }
    }

    NAMESPACE(cc) {
        IMPORT_PMR_CLASS(PmrString, .mFlags = JSB);
        PMR_CONTAINER(PmrList);

        MAP(TransparentMap);
        MAP(TransparentMultiMap);
        MAP(FlatMap);
        MAP(FlatMultiMap);
        MAP(UnorderedStringMap);
        MAP(UnorderedStringMultiMap);

        PMR_MAP(PmrMap);
        PMR_MAP(PmrMultiMap);
        PMR_MAP(PmrTransparentMap);
        PMR_MAP(PmrTransparentMultiMap);
        PMR_MAP(PmrFlatMap);
        PMR_MAP(PmrFlatMultiMap);
        PMR_MAP(PmrUnorderedMap);
        PMR_MAP(PmrUnorderedMultiMap);
        PMR_MAP(PmrUnorderedStringMap);
        PMR_MAP(PmrUnorderedStringMultiMap);

        CONTAINER(TransparentSet);
        CONTAINER(TransparentMultiSet);
        CONTAINER(FlatSet);
        CONTAINER(FlatMultiSet);
        CONTAINER(UnorderedStringSet);
        CONTAINER(UnorderedStringMultiSet);

        PMR_CONTAINER(PmrTransparentSet);
        PMR_CONTAINER(PmrTransparentMultiSet);
        PMR_CONTAINER(PmrFlatSet);
        PMR_CONTAINER(PmrFlatMultiSet);
        PMR_CONTAINER(PmrUnorderedSet);
        PMR_CONTAINER(PmrUnorderedMultiSet);
        PMR_CONTAINER(PmrUnorderedStringSet);
        PMR_CONTAINER(PmrUnorderedStringMultiSet);
    }

    NAMESPACE(ccstd) {
        CONTAINER(vector);
        CLASS(string);
        NAMESPACE(pmr) {
            CONTAINER(vector);
            CLASS(string);
        }
    }
}

}
