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
#include <Cocos/AST/SyntaxFwd.h>
#include <Cocos/GraphTypes.h>

namespace Cocos {

namespace Meta {

enum GenerationFlags : uint64_t {
    NO_FLAGS = 0,
    NO_DEFAULT_CNTR = 1 << 0,
    CUSTOM_CNTR = 1 << 1,
    CUSTOM_DTOR = 1 << 2,
    NO_COPY = 1 << 3,
    NO_MOVE_NO_COPY = 1 << 4,
    EQUAL = 1 << 5,
    NO_EQUAL = 1 << 6,
    LESS = 1 << 7,
    NO_LESS = 1 << 8,
    SPACESHIP = 1 << 9,
    NO_SPACESHIP = 1 << 10,
    REFLECTION = 1 << 11,
    NO_SERIALIZATION = 1 << 12,
    HASH_COMBINE = 1 << 13,
    NO_HASH_COMBINE = 1 << 14,
    ENUM_OPERATOR = 1 << 15,
    NO_ENUM_OPERATOR = 1 << 16,
    PMR_DEFAULT = 1 << 17,
    IMPL_DETAIL = 1 << 18,
    JSB = 1 << 19,
};

constexpr GenerationFlags operator|(const GenerationFlags lhs, const GenerationFlags rhs) noexcept {
    return (GenerationFlags)((uint64_t)lhs | (uint64_t)rhs);
}

constexpr GenerationFlags operator&(const GenerationFlags lhs, const GenerationFlags rhs) noexcept {
    return (GenerationFlags)((uint64_t)lhs & (uint64_t)rhs);
}

constexpr GenerationFlags& operator|=(GenerationFlags& lhs, const GenerationFlags rhs) noexcept {
    return lhs = lhs | rhs;
}

constexpr GenerationFlags& operator&=(GenerationFlags& lhs, const GenerationFlags rhs) noexcept {
    return lhs = lhs & rhs;
}

constexpr bool operator!(GenerationFlags e) noexcept {
    return e == static_cast<GenerationFlags>(0);
}

constexpr bool any(GenerationFlags e) noexcept {
    return !!e;
}

struct Container {};
struct Map {};

struct Instance {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTemplate.get_allocator().resource());
    }

    Instance(const allocator_type& alloc) noexcept;
    Instance(Instance&& rhs, const allocator_type& alloc);
    Instance(Instance const& rhs, const allocator_type& alloc);

    Instance(Instance&& rhs) = default;
    Instance(Instance const& rhs) = delete;
    Instance& operator=(Instance&& rhs) = default;
    Instance& operator=(Instance const& rhs) = default;
    ~Instance() noexcept;

    std::pmr::string mTemplate;
    std::pmr::vector<std::pmr::string> mParameters;
};

struct Namespace {};

struct Define {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mContent.get_allocator().resource());
    }

    Define(const allocator_type& alloc);
    Define(std::string_view content, const allocator_type& alloc);
    Define(Define&& rhs, const allocator_type& alloc);
    Define(Define const& rhs, const allocator_type& alloc);

    Define(Define&& rhs) = default;
    Define(Define const& rhs) = delete;
    Define& operator=(Define&& rhs) = default;
    Define& operator=(Define const& rhs) = default;
    ~Define() noexcept;

    std::pmr::string mContent;
};

struct Concept {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mParentPath.get_allocator().resource());
    }

    Concept(const allocator_type& alloc);
    Concept(std::string_view parentPath, const allocator_type& alloc);
    Concept(Concept&& rhs, const allocator_type& alloc);
    Concept(Concept const& rhs, const allocator_type& alloc);

    Concept(Concept&& rhs) = default;
    Concept(Concept const& rhs) = delete;
    Concept& operator=(Concept&& rhs) = default;
    Concept& operator=(Concept const& rhs) = default;
    ~Concept() noexcept;

    std::pmr::string mParentPath;
};

struct Constraints {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mConcepts.get_allocator().resource());
    }

    Constraints(const allocator_type& alloc) noexcept;
    Constraints(Constraints&& rhs, const allocator_type& alloc);
    Constraints(Constraints const& rhs, const allocator_type& alloc);

    Constraints(Constraints&& rhs) = default;
    Constraints(Constraints const& rhs) = delete;
    Constraints& operator=(Constraints&& rhs) = default;
    Constraints& operator=(Constraints const& rhs) = default;
    ~Constraints() noexcept;

    std::pmr::vector<std::pmr::string> mConcepts;
};

struct Inherits {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mBases.get_allocator().resource());
    }

    Inherits(const allocator_type& alloc) noexcept;
    Inherits(Inherits&& rhs, const allocator_type& alloc);
    Inherits(Inherits const& rhs, const allocator_type& alloc);

    Inherits(Inherits&& rhs) = default;
    Inherits(Inherits const& rhs) = delete;
    Inherits& operator=(Inherits&& rhs) = default;
    Inherits& operator=(Inherits const& rhs) = default;
    ~Inherits() noexcept;

    std::pmr::vector<std::pmr::string> mBases;
};

struct Alias {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTypePath.get_allocator().resource());
    }

    Alias(const allocator_type& alloc);
    Alias(std::string_view typePath, const allocator_type& alloc);
    Alias(Alias&& rhs, const allocator_type& alloc);
    Alias(Alias const& rhs, const allocator_type& alloc);

    Alias(Alias&& rhs) = default;
    Alias(Alias const& rhs) = delete;
    Alias& operator=(Alias&& rhs) = default;
    Alias& operator=(Alias const& rhs) = default;
    ~Alias() noexcept;

    std::pmr::string mTypePath;
};

struct Traits {
    bool mImport = false;
    bool mInterface = false;
    bool mClass = false;
    bool mPmr = false;
    bool mNoexcept = true;
    bool mUnknown = false;
    bool mTrivial = false;
    GenerationFlags mFlags = {};
    uint32_t mAlignment = 0;
};

struct Value {};

struct EnumValue {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    EnumValue(const allocator_type& alloc) noexcept;
    EnumValue(EnumValue&& rhs, const allocator_type& alloc);
    EnumValue(EnumValue const& rhs, const allocator_type& alloc);

    EnumValue(EnumValue&& rhs) = default;
    EnumValue(EnumValue const& rhs) = delete;
    EnumValue& operator=(EnumValue&& rhs) = default;
    EnumValue& operator=(EnumValue const& rhs) = default;
    ~EnumValue() noexcept;

    std::pmr::string mName;
    std::pmr::string mValue;
    std::pmr::string mReflectionName;
};

struct Enum {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mUnderlyingType.get_allocator().resource());
    }

    Enum(const allocator_type& alloc) noexcept;
    Enum(Enum&& rhs, const allocator_type& alloc);
    Enum(Enum const& rhs, const allocator_type& alloc);

    Enum(Enum&& rhs) = default;
    Enum(Enum const& rhs) = delete;
    Enum& operator=(Enum&& rhs) = default;
    Enum& operator=(Enum const& rhs) = default;
    ~Enum() noexcept;

    bool mIsFlags = false;
    bool mEnumOperator = true;
    bool mHasName = true;
    std::pmr::string mUnderlyingType;
    std::pmr::vector<EnumValue> mValues;
};

struct Tag {
    bool mEntity = false;
};

struct Member {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTypePath.get_allocator().resource());
    }

    Member(const allocator_type& alloc) noexcept;
    Member(Member&& rhs, const allocator_type& alloc);
    Member(Member const& rhs, const allocator_type& alloc);

    Member(Member&& rhs) = default;
    Member(Member const& rhs) = delete;
    Member& operator=(Member&& rhs) = default;
    Member& operator=(Member const& rhs) = default;
    ~Member() noexcept;

    std::pmr::string getMemberName() const;

    std::pmr::string mTypePath;
    std::pmr::string mMemberName;
    std::pmr::string mDefaultValue;
    std::pmr::string mComments;
    bool mConst = false;
    bool mPointer = false;
    bool mReference = false;
    bool mPublic = true;
    GenerationFlags mFlags = {};
    std::pmr::string mTypescriptType;
    std::pmr::string mTypescriptDefaultValue;
    bool mTypescriptArray = false;
};

struct Parameter {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTypePath.get_allocator().resource());
    }

    Parameter(const allocator_type& alloc) noexcept;
    Parameter(Parameter&& rhs, const allocator_type& alloc);
    Parameter(Parameter const& rhs, const allocator_type& alloc);

    Parameter(Parameter&& rhs) = default;
    Parameter(Parameter const& rhs) = delete;
    Parameter& operator=(Parameter&& rhs) = default;
    Parameter& operator=(Parameter const& rhs) = default;
    ~Parameter() noexcept;

    std::pmr::string mTypePath;
    std::pmr::string mName;
    std::pmr::string mDefaultValue;
    bool mConst = false;
    bool mPointer = false;
    bool mReference = false;
    bool mNullable = false;
};

struct Method {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mReturnType.get_allocator().resource());
    }

    Method(const allocator_type& alloc) noexcept;
    Method(Method&& rhs, const allocator_type& alloc);
    Method(Method const& rhs, const allocator_type& alloc);

    Method(Method&& rhs) = default;
    Method(Method const& rhs) = delete;
    Method& operator=(Method&& rhs) = default;
    Method& operator=(Method const& rhs) = default;
    ~Method() noexcept;

    Parameter mReturnType;
    std::pmr::string mFunctionName;
    std::pmr::vector<Parameter> mParameters;
    bool mVirtual = false;
    bool mConst = false;
    bool mNoexcept = false;
    bool mPure = false;
    bool mStatic = false;
    bool mGetter = false;
    bool mSetter = false;
    bool mNullable = false;
    bool mSkip = false;
};

struct Constructor {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mIndices.get_allocator().resource());
    }

    Constructor(const allocator_type& alloc) noexcept;
    Constructor(Constructor&& rhs, const allocator_type& alloc);
    Constructor(Constructor const& rhs, const allocator_type& alloc);

    Constructor(Constructor&& rhs) = default;
    Constructor(Constructor const& rhs) = delete;
    Constructor& operator=(Constructor&& rhs) = default;
    Constructor& operator=(Constructor const& rhs) = default;
    ~Constructor() noexcept;

    std::pmr::vector<uint32_t> mIndices;
    bool mHasDefault = false;
};

struct Struct {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMembers.get_allocator().resource());
    }

    Struct(const allocator_type& alloc) noexcept;
    Struct(Struct&& rhs, const allocator_type& alloc);
    Struct(Struct const& rhs, const allocator_type& alloc);

    Struct(Struct&& rhs) = default;
    Struct(Struct const& rhs) = delete;
    Struct& operator=(Struct&& rhs) = default;
    Struct& operator=(Struct const& rhs) = default;
    ~Struct() noexcept;

    std::pmr::vector<Member> mMembers;
    std::pmr::vector<Constructor> mConstructors;
    std::pmr::vector<std::pmr::string> mMemberFunctions;
    std::pmr::vector<Member> mTypescriptMembers;
    std::pmr::vector<std::pmr::string> mTypescriptFunctions;
    std::pmr::vector<Method> mMethods;
};

struct Variant {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mVariants.get_allocator().resource());
    }

    Variant(const allocator_type& alloc) noexcept;
    Variant(Variant&& rhs, const allocator_type& alloc);
    Variant(Variant const& rhs, const allocator_type& alloc);

    Variant(Variant&& rhs) = default;
    Variant(Variant const& rhs) = delete;
    Variant& operator=(Variant&& rhs) = default;
    Variant& operator=(Variant const& rhs) = default;
    ~Variant() noexcept;

    std::pmr::vector<std::pmr::string> mVariants;
    bool mUseIndex = true;
};

struct Namespace_ {};
struct Define_ {};
struct Concept_ {};
struct Alias_ {};
struct Value_ {};
struct Enum_ {};
struct Tag_ {};
struct Struct_ {};
struct Variant_ {};
struct Graph_ {};
struct Container_ {};
struct Map_ {};
struct Instance_ {};

struct PolymorphicPair {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mTag.get_allocator().resource());
    }

    PolymorphicPair(const allocator_type& alloc) noexcept;
    PolymorphicPair(PolymorphicPair&& rhs, const allocator_type& alloc);
    PolymorphicPair(PolymorphicPair const& rhs, const allocator_type& alloc);

    PolymorphicPair(PolymorphicPair&& rhs) = default;
    PolymorphicPair(PolymorphicPair const& rhs) = delete;
    PolymorphicPair& operator=(PolymorphicPair&& rhs) = default;
    PolymorphicPair& operator=(PolymorphicPair const& rhs) = default;
    ~PolymorphicPair() noexcept;

    bool isIntrusive() const noexcept {
        return mMemberName.empty();
    }
    bool isVector() const noexcept {
        return mVector;
    }

    std::pmr::string mTag;
    std::pmr::string mValue;
    std::pmr::string mMemberName;
    std::pmr::string mContainerPath;
    bool mVector = true;
};

struct Polymorphic {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mConcepts.get_allocator().resource());
    }

    Polymorphic(const allocator_type& alloc) noexcept;
    Polymorphic(Polymorphic&& rhs, const allocator_type& alloc);
    Polymorphic(Polymorphic const& rhs, const allocator_type& alloc);

    Polymorphic(Polymorphic&& rhs) = default;
    Polymorphic(Polymorphic const& rhs) = delete;
    Polymorphic& operator=(Polymorphic&& rhs) = default;
    Polymorphic& operator=(Polymorphic const& rhs) = default;
    ~Polymorphic() noexcept;

    bool contains(const PolymorphicPair& rhs) const {
        for (const auto& t : mConcepts) {
            if (t.mTag == rhs.mTag)
                return true;
        }
        return false;
    }

    std::pmr::vector<PolymorphicPair> mConcepts;
};

struct Component {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    Component(const allocator_type& alloc) noexcept;
    Component(Component&& rhs, const allocator_type& alloc);
    Component(Component const& rhs, const allocator_type& alloc);

    Component(Component&& rhs) = default;
    Component(Component const& rhs) = delete;
    Component& operator=(Component&& rhs) = default;
    Component& operator=(Component const& rhs) = default;
    ~Component() noexcept;

    bool isValid() const noexcept {
        return !mMemberName.empty();
    }
    bool isVector() const noexcept {
        return mVector;
    }
    std::pmr::string getTypescriptComponentType(const SyntaxGraph& g,
        std::pmr::memory_resource* mr,
        std::pmr::memory_resource* scratch) const noexcept;

    std::pmr::string mName;
    std::pmr::string mValuePath;
    std::pmr::string mMemberName;
    std::pmr::string mContainerPath;
    bool mVector = true;
};

struct VertexMap {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMapType.get_allocator().resource());
    }

    VertexMap(const allocator_type& alloc) noexcept;
    VertexMap(VertexMap&& rhs, const allocator_type& alloc);
    VertexMap(VertexMap const& rhs, const allocator_type& alloc);

    VertexMap(VertexMap&& rhs) = default;
    VertexMap(VertexMap const& rhs) = delete;
    VertexMap& operator=(VertexMap&& rhs) = default;
    VertexMap& operator=(VertexMap const& rhs) = default;
    ~VertexMap() noexcept;

    bool isBimap() const noexcept {
        return !mComponentName.empty();
    }
    bool isComponentMember() const noexcept {
        return !mComponentMemberName.empty();
    }

    std::pmr::string mMapType;
    std::pmr::string mMemberName;
    std::pmr::string mKeyType;
    std::pmr::string mComponentName;
    std::pmr::string mComponentMemberName;
    std::pmr::string mTypePath;
};

struct Vector_ {};
struct List_ {};
struct Set_ {};
struct MultiSet_ {};

using VertexListType = std::variant<Vector_, List_>;

inline bool operator<(const VertexListType& lhs, const VertexListType& rhs) noexcept {
    return lhs.index() < rhs.index();
}

inline bool operator==(const VertexListType& lhs, const VertexListType& rhs) noexcept {
    return lhs.index() == rhs.index();
}

inline bool operator!=(const VertexListType& lhs, const VertexListType& rhs) noexcept {
    return !(lhs == rhs);
}
using EdgeListType = std::variant<List_, Set_, MultiSet_>;

inline bool operator<(const EdgeListType& lhs, const EdgeListType& rhs) noexcept {
    return lhs.index() < rhs.index();
}

inline bool operator==(const EdgeListType& lhs, const EdgeListType& rhs) noexcept {
    return lhs.index() == rhs.index();
}

inline bool operator!=(const EdgeListType& lhs, const EdgeListType& rhs) noexcept {
    return !(lhs == rhs);
}
using OutEdgeListType = std::variant<Vector_, List_, Set_, MultiSet_>;

inline bool operator<(const OutEdgeListType& lhs, const OutEdgeListType& rhs) noexcept {
    return lhs.index() < rhs.index();
}

inline bool operator==(const OutEdgeListType& lhs, const OutEdgeListType& rhs) noexcept {
    return lhs.index() == rhs.index();
}

inline bool operator!=(const OutEdgeListType& lhs, const OutEdgeListType& rhs) noexcept {
    return !(lhs == rhs);
}

struct Direct_ {};
struct Trie_ {};

using PathIndexType = std::variant<Direct_, Map_>;

inline bool operator<(const PathIndexType& lhs, const PathIndexType& rhs) noexcept {
    return lhs.index() < rhs.index();
}

inline bool operator==(const PathIndexType& lhs, const PathIndexType& rhs) noexcept {
    return lhs.index() == rhs.index();
}

inline bool operator!=(const PathIndexType& lhs, const PathIndexType& rhs) noexcept {
    return !(lhs == rhs);
}

struct Layer {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mContainer.get_allocator().resource());
    }

    Layer(const allocator_type& alloc) noexcept;
    Layer(Layer&& rhs, const allocator_type& alloc);
    Layer(Layer const& rhs, const allocator_type& alloc);

    Layer(Layer&& rhs) = default;
    Layer(Layer const& rhs) = delete;
    Layer& operator=(Layer&& rhs) = default;
    Layer& operator=(Layer const& rhs) = default;
    ~Layer() noexcept;

    bool isIntrusive() const noexcept {
        return mMemberName.empty();
    }

    std::pmr::string mContainer;
    std::pmr::string mMemberName;
    std::pmr::string mGraphPath;
    std::pmr::string mTagPath;
    VertexListType mContainerType;
};

struct Stack {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mLayers.get_allocator().resource());
    }

    Stack(const allocator_type& alloc) noexcept;
    Stack(Stack&& rhs, const allocator_type& alloc);
    Stack(Stack const& rhs, const allocator_type& alloc);

    Stack(Stack&& rhs) = default;
    Stack(Stack const& rhs) = delete;
    Stack& operator=(Stack&& rhs) = default;
    Stack& operator=(Stack const& rhs) = default;
    ~Stack() noexcept;

    std::pmr::vector<Layer> mLayers;
    std::pmr::string mContainer = std::pmr::string("/boost/container/pmr/vector", get_allocator());
};

struct Named {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mComponentName.get_allocator().resource());
    }

    Named(const allocator_type& alloc) noexcept;
    Named(Named&& rhs, const allocator_type& alloc);
    Named(Named const& rhs, const allocator_type& alloc);

    Named(Named&& rhs) = default;
    Named(Named const& rhs) = delete;
    Named& operator=(Named&& rhs) = default;
    Named& operator=(Named const& rhs) = default;
    ~Named() noexcept;

    bool mComponent = true;
    std::pmr::string mComponentName;
    std::pmr::string mComponentMemberName;
};

struct Addressable {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMemberName.get_allocator().resource());
    }

    Addressable(const allocator_type& alloc) noexcept;
    Addressable(Addressable&& rhs, const allocator_type& alloc);
    Addressable(Addressable const& rhs, const allocator_type& alloc);

    Addressable(Addressable&& rhs) = default;
    Addressable(Addressable const& rhs) = delete;
    Addressable& operator=(Addressable&& rhs) = default;
    Addressable& operator=(Addressable const& rhs) = default;
    ~Addressable() noexcept;

    bool mUtf8 = false;
    bool mPathPropertyMap = false;
    PathIndexType mType = Map_{};
    std::pmr::string mMemberName;
};

struct MemberRecord {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mComment.get_allocator().resource());
    }

    MemberRecord(const allocator_type& alloc) noexcept;
    MemberRecord(MemberRecord&& rhs, const allocator_type& alloc);
    MemberRecord(MemberRecord const& rhs, const allocator_type& alloc);

    MemberRecord(MemberRecord&& rhs) = default;
    MemberRecord(MemberRecord const& rhs) = delete;
    MemberRecord& operator=(MemberRecord&& rhs) = default;
    MemberRecord& operator=(MemberRecord const& rhs) = default;
    ~MemberRecord() noexcept;

    std::pmr::string mComment;
    std::pmr::string mType;
    std::pmr::string mMember;
    std::pmr::string mDefaultValue;
    uint32_t mDefaultValueOffset = 0;
};

struct MemberFormatter {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMembers.get_allocator().resource());
    }

    MemberFormatter(const allocator_type& alloc) noexcept;
    MemberFormatter(MemberFormatter&& rhs, const allocator_type& alloc);
    MemberFormatter(MemberFormatter const& rhs, const allocator_type& alloc);

    MemberFormatter(MemberFormatter&& rhs) = default;
    MemberFormatter(MemberFormatter const& rhs) = delete;
    MemberFormatter& operator=(MemberFormatter&& rhs) = default;
    MemberFormatter& operator=(MemberFormatter const& rhs) = default;
    ~MemberFormatter() noexcept;
    void clear() noexcept {
        mMembers.clear();
        mTypeLength = 0;
    }

    std::pmr::vector<MemberRecord> mMembers;
    uint32_t mTypeLength = 0;
};

struct Graph {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mMembers.get_allocator().resource());
    }

    Graph(const allocator_type& alloc) noexcept;
    Graph(Graph&& rhs, const allocator_type& alloc);
    Graph(Graph const& rhs, const allocator_type& alloc);

    Graph(Graph&& rhs) = default;
    Graph(Graph const& rhs) = delete;
    Graph& operator=(Graph&& rhs) = default;
    Graph& operator=(Graph const& rhs) = default;
    ~Graph() noexcept;

    bool hasVertexProperty() const noexcept {
        return !mVertexProperty.empty();
    }

    bool hasEdgeProperty() const noexcept {
        return !mEdgeProperty.empty();
    }

    bool isVector() const noexcept {
        return holds_alternative<Vector_>(mVertexListType);
    }

    bool isEdgeListVector() const noexcept {
        return false;
    }

    bool isDirectedOnly() const noexcept {
        return !mUndirected && !mBidirectional;
    }

    bool isBidirectionalOnly() const noexcept {
        return !mUndirected && mBidirectional;
    }

    bool needEdgeList() const noexcept {
        if (mUndirected)
            return true;

        if (isDirectedOnly())
            return false;

        Expects(mBidirectional);

        if (mEdgeProperty.empty())
            return false;

        return true;
    }

    bool hasReserve() const noexcept {
        return isVector();
    }

    bool isContinuousContainer() const noexcept {
        return isVector();
    }

    bool isAddressable() const noexcept {
        Expects(mReferenceGraph || !mAddressable);
        return !!mAddressable;
    }

    bool isPolymorphic() const noexcept {
        return !mPolymorphic.mConcepts.empty();
    }

    bool hasProperties() const noexcept {
        return mNamed || !mVertexProperty.empty() || !mComponents.empty();
    }

    bool isComponentInVertex() const noexcept {
        return !mComponents.empty() && !isVector();
    }

    bool isReference() const noexcept {
        Expects(mReferenceGraph || !mAddressable);
        return mReferenceGraph;
    }

    bool isAliasGraph() const noexcept {
        return mReferenceGraph && mAliasGraph;
    }

    bool isMutableReference() const noexcept {
        return mReferenceGraph && mMutableReference;
    }

    bool needReferenceEdges() const noexcept {
        return mReferenceGraph && !mAliasGraph;
    }

    bool hasAddressIndex() const noexcept {
        return mAddressable && mAddressIndex;
    }
    bool hasIteratorComponent() const noexcept {
        return !isVector();
    }
    const Component& getComponent(std::string_view name) const {
        for (const auto& c : mComponents) {
            if (c.mName == name) {
                return c;
            }
        }
        throw std::out_of_range("component not found");
    }

    // Typescript
    std::string_view getTypescriptNullVertex() const;

    std::pmr::string getTypescriptVertexPropertyType(const SyntaxGraph& g,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept;

    std::pmr::string getTypescriptVertexDescriptorType(std::string_view tsName,
        std::pmr::memory_resource* scratch) const;
    std::string_view getTypescriptEdgeDescriptorType() const;
    std::string_view getTypescriptReferenceDescriptorType() const;

    std::pmr::string getTypescriptVertexDereference(std::string_view v,
        std::pmr::memory_resource* scratch) const;

    std::string_view getTypescriptOutEdgeList(bool bAddressable) const;
    std::string_view getTypescriptInEdgeList(bool bAddressable) const;

    std::pmr::vector<Member> mMembers;
    std::pmr::vector<Constructor> mConstructors;
    std::pmr::vector<std::pmr::string> mMemberFunctions;
    std::pmr::vector<Method> mMethods;
    std::pmr::string mVertexProperty;
    std::pmr::string mEdgeProperty;
    std::pmr::vector<Component> mComponents;
    bool mIncidence = true;
    bool mAdjacency = true;
    bool mUndirected = false;
    bool mBidirectional = true;
    bool mVertexList = true;
    bool mEdgeList = true;
    bool mMutableGraphVertex = true;
    bool mMutableGraphEdge = true;
    bool mNamed = false;
    bool mReferenceGraph = false;
    bool mAliasGraph = false;
    bool mAddressable = false;
    bool mAddressIndex = true;
    bool mMutableReference = true;
    bool mColorMap = true;
    Polymorphic mPolymorphic;
    VertexListType mVertexListType;
    EdgeListType mEdgeListType;
    OutEdgeListType mOutEdgeListType;
    std::string mVertexListPath;
    std::string mEdgeListPath;
    std::string mOutEdgeListPath;
    std::string mVertexDescriptor;
    std::string mVertexSizeType = "uint32_t";
    std::string mDifferenceType = "int32_t";
    std::string mEdgeSizeType = "uint32_t";
    std::string mEdgeDifferenceType = "int32_t";
    std::string mDegreeSizeType = "uint32_t";
    std::pmr::vector<VertexMap> mVertexMaps;
    Named mNamedConcept;
    Addressable mAddressableConcept;
    std::pmr::vector<Member> mTypescriptMembers;
    std::pmr::vector<std::pmr::string> mTypescriptFunctions;
};

struct Typescript {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    Typescript(const allocator_type& alloc) noexcept;
    Typescript(Typescript&& rhs, const allocator_type& alloc);
    Typescript(Typescript const& rhs, const allocator_type& alloc);

    Typescript(Typescript&& rhs) = default;
    Typescript(Typescript const& rhs) = delete;
    Typescript& operator=(Typescript&& rhs) = default;
    Typescript& operator=(Typescript const& rhs) = default;
    ~Typescript() noexcept;

    std::pmr::string mName;
    bool mArray = false;
};

enum class ImplEnum : uint32_t {
    None,
    Inline,
    Separated,
    Delete,
};

struct SyntaxGraph {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept;

    inline std::pmr::memory_resource* resource() const noexcept {
        return get_allocator().resource();
    }

    SyntaxGraph(const allocator_type& alloc);
    SyntaxGraph(SyntaxGraph&& rhs, const allocator_type& alloc);
    SyntaxGraph(SyntaxGraph const& rhs, const allocator_type& alloc);

    SyntaxGraph(SyntaxGraph&& rhs);
    SyntaxGraph(SyntaxGraph const& rhs) = delete;
    SyntaxGraph& operator=(SyntaxGraph&& rhs);
    SyntaxGraph& operator=(SyntaxGraph const& rhs);
    ~SyntaxGraph() noexcept;

    // Graph
    using directed_category = boost::bidirectional_tag;
    using vertex_descriptor = uint32_t;
    using edge_descriptor = Impl::EdgeDescriptor<directed_category, vertex_descriptor>;
    using edge_parallel_category = boost::allow_parallel_edge_tag;
    struct traversal_category
        : virtual boost::incidence_graph_tag
        , virtual boost::bidirectional_graph_tag
        , virtual boost::adjacency_graph_tag
        , virtual boost::vertex_list_graph_tag
        , virtual boost::edge_list_graph_tag {};

    constexpr static vertex_descriptor null_vertex() noexcept {
        return std::numeric_limits<vertex_descriptor>::max();
    }

    // IncidenceGraph
    using out_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using out_edge_iterator = Impl::OutEdgeIter<
        std::pmr::vector<out_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;
    using degree_size_type = uint32_t;

    // BidirectionalGraph
    using in_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using in_edge_iterator = Impl::InEdgeIter<
        std::pmr::vector<in_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;

    // AdjacencyGraph
    using adjacency_iterator = boost::adjacency_iterator_generator<
        SyntaxGraph, vertex_descriptor, out_edge_iterator>::type;

    // VertexListGraph
    using vertex_iterator = boost::integer_range<vertex_descriptor>::iterator;
    using vertices_size_type = uint32_t;

    // VertexList help functions
    inline std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mOutEdges;
    }
    inline const std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mOutEdges;
    }

    inline std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mInEdges;
    }
    inline const std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mInEdges;
    }

    inline boost::integer_range<vertex_descriptor> vertex_set() const noexcept {
        return boost::integer_range<vertex_descriptor>(0, gsl::narrow_cast<vertices_size_type>(mVertices.size()));
    }

    inline vertex_descriptor current_id() const noexcept {
        return gsl::narrow_cast<vertex_descriptor>(mVertices.size());
    }

    [[nodiscard]] inline std::pmr::vector<boost::default_color_type> colors(std::pmr::memory_resource* mr) const {
        return std::pmr::vector<boost::default_color_type>{ mVertices.size(), mr };
    }

    // EdgeListGraph
    using edge_iterator = Impl::DirectedEdgeIterator<vertex_iterator, out_edge_iterator, SyntaxGraph>;
    using edges_size_type = uint32_t;

    // AddressableGraph (Separated)
    using ownership_descriptor = Impl::EdgeDescriptor<boost::bidirectional_tag, vertex_descriptor>;

    using children_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using children_iterator = Impl::OutEdgeIter<
        std::pmr::vector<children_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;
    using children_size_type = uint32_t;

    using parent_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using parent_iterator = Impl::InEdgeIter<
        std::pmr::vector<parent_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;

    using ownership_iterator = Impl::OwnershipIterator<vertex_iterator, children_iterator, SyntaxGraph>;
    using ownerships_size_type = edges_size_type;

    // AddressableGraph help functions
    inline std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) noexcept {
        return mObjects[v].mChildren;
    }
    inline const std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mChildren;
    }

    inline std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) noexcept {
        return mObjects[v].mParents;
    }
    inline const std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mParents;
    }

    // PolymorphicGraph
    using vertex_tag_type = std::variant<Define_, Namespace_, Concept_, Alias_, Value_, Enum_, Tag_, Struct_, Graph_, Variant_, Container_, Map_, Instance_>;
    using vertex_value_type = std::variant<Define*, Namespace*, Concept*, Alias*, Value*, Enum*, Tag*, Struct*, Graph*, Variant*, Container*, Map*, Instance*>;
    using vertex_const_value_type = std::variant<const Define*, const Namespace*, const Concept*, const Alias*, const Value*, const Enum*, const Tag*, const Struct*, const Graph*, const Variant*, const Container*, const Map*, const Instance*>;
    using vertex_handle_type = std::variant<
        Impl::ValueHandle<Define_, vertex_descriptor>,
        Impl::ValueHandle<Namespace_, Namespace>,
        Impl::ValueHandle<Concept_, vertex_descriptor>,
        Impl::ValueHandle<Alias_, vertex_descriptor>,
        Impl::ValueHandle<Value_, Value>,
        Impl::ValueHandle<Enum_, vertex_descriptor>,
        Impl::ValueHandle<Tag_, vertex_descriptor>,
        Impl::ValueHandle<Struct_, vertex_descriptor>,
        Impl::ValueHandle<Graph_, vertex_descriptor>,
        Impl::ValueHandle<Variant_, vertex_descriptor>,
        Impl::ValueHandle<Container_, Container>,
        Impl::ValueHandle<Map_, Map>,
        Impl::ValueHandle<Instance_, vertex_descriptor>>;

    bool isNamespace(std::string_view typePath) const noexcept;
    bool isValueType(vertex_descriptor vertID) const noexcept;
    bool isInstantiation(vertex_descriptor vertID) const noexcept;
    bool isTag(vertex_descriptor vertID) const noexcept;
    bool isPmr(vertex_descriptor vertID) const noexcept;
    bool isNoexcept(vertex_descriptor vertID) const noexcept;
    bool isComposition(vertex_descriptor vertID) const noexcept;
    bool isString(vertex_descriptor vertID) const noexcept;
    bool isUtf8(vertex_descriptor vertID) const noexcept;
    bool isPair(vertex_descriptor vertID, std::pmr::memory_resource* scratch) const noexcept;
    bool isOptional(vertex_descriptor vertID) const noexcept;
    bool isDLL(vertex_descriptor vertID, const ModuleGraph& mg) const noexcept;
    bool isJsb(vertex_descriptor vertID, const ModuleGraph& mg) const noexcept;

    // struct
    bool isDerived(vertex_descriptor vertID) const noexcept;
    ImplEnum needDefaultCntr(vertex_descriptor vertID) const noexcept;
    ImplEnum needMoveCntr(vertex_descriptor vertID) const noexcept;
    ImplEnum needCopyCntr(vertex_descriptor vertID) const noexcept;
    ImplEnum needDtor(vertex_descriptor vertID, bool bDLL) const noexcept;
    bool hasPmrOptional(vertex_descriptor vertID) const noexcept;
    bool hasString(vertex_descriptor vertID) const noexcept;
    bool hasImpl(vertex_descriptor vertID, bool bDLL) const noexcept;
    bool hasHeader(vertex_descriptor vertID) const noexcept;
    bool hasType(vertex_descriptor vertID, vertex_descriptor typeID) const noexcept;
    bool hasConsecutiveParameters(vertex_descriptor vertID, const Constructor& cntr) const noexcept;

    vertex_descriptor getMemberType(vertex_descriptor vertID, std::string_view member) const noexcept;
    vertex_descriptor getFirstMemberString(vertex_descriptor vertID) const noexcept;
    vertex_descriptor getFirstMemberUtf8(vertex_descriptor vertID) const noexcept;

    // graph
    bool isPathPmr(const Graph& s) const noexcept;

    // general
    std::pmr::string getMemberName(std::string_view memberName, bool bPublic) const;

    std::pmr::string getTypePath(vertex_descriptor vertID, std::pmr::memory_resource* mr) const;

    vertex_descriptor lookupIdentifier(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypePath(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getDependentName(std::string_view ns, vertex_descriptor vertID,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getDependentCppName(std::string_view ns, vertex_descriptor vertID,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    vertex_descriptor lookupType(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* scratch) const;

    std::pmr::string getNamespace(vertex_descriptor vertID, std::pmr::memory_resource* mr) const;
    std::pmr::string getScope(vertex_descriptor vertID, std::pmr::memory_resource* mr) const;

    std::pair<std::string_view, std::string_view> splitTypePath(std::string_view typePath) const;

    void instantiate(std::string_view currentScope, std::string_view dependentName,
        std::pmr::memory_resource* scratch);

    void propagate(vertex_descriptor vertID, GenerationFlags flags = {});

    vertex_descriptor getTemplate(vertex_descriptor instanceID, std::pmr::memory_resource* scratch) const;

    bool moduleHasMap(std::string_view modulePath, std::string_view mapPath) const;

    bool moduleHasContainer(std::string_view modulePath, std::string_view typePath) const;

    bool moduleHasType(std::string_view modulePath, std::string_view typePath) const;

    bool moduleHasGraph(std::string_view modulePath) const;

    bool moduleUsesHashCombine(std::string_view modulePath) const;

    bool moduleHasImpl(std::string_view modulePath, bool bDLL) const;

    // Typescript
    bool isTypescriptData(std::string_view name) const;
    bool isTypescriptValueType(vertex_descriptor vertID) const;

    bool isTypescriptBoolean(vertex_descriptor vertID) const;
    bool isTypescriptNumber(vertex_descriptor vertID) const;
    bool isTypescriptString(vertex_descriptor vertID) const;
    bool isTypescriptArray(vertex_descriptor vertID, std::pmr::memory_resource* scratch) const;
    bool isTypescriptSet(vertex_descriptor vertID) const;
    bool isTypescriptMap(vertex_descriptor vertID) const;
    bool isTypescriptPointer(vertex_descriptor vertID) const;

    std::pmr::string getTypescriptTypename(vertex_descriptor vertID,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTypename(std::string_view typePath,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTagName(vertex_descriptor vertID,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptTagName(std::string_view typePath,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptInitialValue(
        vertex_descriptor vertID, const Member& member,
        std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypescriptGraphPolymorphicVariant(const Graph& s,
        std::pmr::memory_resource* mr,
        std::pmr::memory_resource* scratch) const;

    std::pmr::string getTypedParameterName(const Parameter& p, bool bPublic, bool bFull = false, bool bOptional = false) const;

    PmrMap<std::pmr::string, PmrSet<std::pmr::string>> getImportedTypes(
        std::string_view modulePath, std::pmr::memory_resource* mr) const;

    // ContinuousContainer
    void reserve(vertices_size_type sz);

    // Members
    struct object_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        object_type(const allocator_type& alloc) noexcept;
        object_type(object_type&& rhs, const allocator_type& alloc);
        object_type(object_type const& rhs, const allocator_type& alloc);

        object_type(object_type&& rhs) noexcept;
        object_type(object_type const& rhs) = delete;
        object_type& operator=(object_type&& rhs);
        object_type& operator=(object_type const& rhs);
        ~object_type() noexcept;

        std::pmr::vector<children_edge_type> mChildren;
        std::pmr::vector<parent_edge_type> mParents;
    };

    struct vertex_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        vertex_type(const allocator_type& alloc) noexcept;
        vertex_type(vertex_type&& rhs, const allocator_type& alloc);
        vertex_type(vertex_type const& rhs, const allocator_type& alloc);

        vertex_type(vertex_type&& rhs) noexcept;
        vertex_type(vertex_type const& rhs) = delete;
        vertex_type& operator=(vertex_type&& rhs);
        vertex_type& operator=(vertex_type const& rhs);
        ~vertex_type() noexcept;

        std::pmr::vector<out_edge_type> mOutEdges;
        std::pmr::vector<in_edge_type> mInEdges;
        vertex_handle_type mHandle;
    };

    struct names_ {
    } static constexpr names = {};
    struct traits_ {
    } static constexpr traits = {};
    struct constraints_ {
    } static constexpr constraints = {};
    struct inherits_ {
    } static constexpr inherits = {};
    struct modulePaths_ {
    } static constexpr modulePaths = {};
    struct typescripts_ {
    } static constexpr typescripts = {};

    // Owners
    std::pmr::vector<object_type> mObjects;
    // Vertices
    std::pmr::vector<vertex_type> mVertices;
    // Components
    std::pmr::vector<std::pmr::string> mNames;
    std::pmr::vector<Traits> mTraits;
    std::pmr::vector<Constraints> mConstraints;
    std::pmr::vector<Inherits> mInherits;
    std::pmr::vector<std::pmr::string> mModulePaths;
    std::pmr::vector<Typescript> mTypescripts;
    // PolymorphicGraph
    std::pmr::vector<Define> mDefines;
    std::pmr::vector<Concept> mConcepts;
    std::pmr::vector<Alias> mAliases;
    std::pmr::vector<Enum> mEnums;
    std::pmr::vector<Tag> mTags;
    std::pmr::vector<Struct> mStructs;
    std::pmr::vector<Graph> mGraphs;
    std::pmr::vector<Variant> mVariants;
    std::pmr::vector<Instance> mInstances;
    // Members
    std::pmr::memory_resource* mScratch = nullptr;
    // Path
    PmrMap<std::pmr::string, uint32_t> mPathIndex;
};

enum Features : uint32_t {
    Fwd = 1 << 0,
    Types = 1 << 1,
    Serialization = 1 << 2,
    Names = 1 << 3,
    NVP = 1 << 4,
    Reflection = 1 << 5,
    Graphs = 1 << 6,
    Sender = 1 << 7,
    Typescripts = 1 << 8,
    Jsb = 1 << 9,
    ToJs = 1 << 10,
};

constexpr Features operator|(const Features lhs, const Features rhs) noexcept {
    return (Features)((uint32_t)lhs | (uint32_t)rhs);
}

constexpr Features operator&(const Features lhs, const Features rhs) noexcept {
    return (Features)((uint32_t)lhs & (uint32_t)rhs);
}

constexpr Features& operator|=(Features& lhs, const Features rhs) noexcept {
    return lhs = lhs | rhs;
}

constexpr Features& operator&=(Features& lhs, const Features rhs) noexcept {
    return lhs = lhs & rhs;
}

constexpr bool operator!(Features e) noexcept {
    return e == static_cast<Features>(0);
}

constexpr bool any(Features e) noexcept {
    return !!e;
}

struct ModuleInfo {
    bool isDLL() const noexcept {
        return !mAPI.empty();
    }

    Features mFeatures = {};
    std::string mFolder;
    std::string mFilePrefix;
    std::string mJsbHeaders;
    std::string mToJsFilename;
    std::string mToJsPrefix;
    std::string mToJsNamespace;
    std::string mToJsHppHeaders;
    std::string mToJsCppHeaders;
    std::string mTypescriptFolder;
    std::string mTypescriptFilePrefix;
    std::string mAPI;
    std::vector<std::string> mRequires;
    std::string mHeader;
    std::string mTypescriptInclude;
};

struct ModuleGraph {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept;

    inline std::pmr::memory_resource* resource() const noexcept {
        return get_allocator().resource();
    }

    ModuleGraph(const allocator_type& alloc);
    ModuleGraph(ModuleGraph&& rhs, const allocator_type& alloc);
    ModuleGraph(ModuleGraph const& rhs, const allocator_type& alloc);

    ModuleGraph(ModuleGraph&& rhs);
    ModuleGraph(ModuleGraph const& rhs) = delete;
    ModuleGraph& operator=(ModuleGraph&& rhs);
    ModuleGraph& operator=(ModuleGraph const& rhs);
    ~ModuleGraph() noexcept;

    // Graph
    using directed_category = boost::bidirectional_tag;
    using vertex_descriptor = uint32_t;
    using edge_descriptor = Impl::EdgeDescriptor<directed_category, vertex_descriptor>;
    using edge_parallel_category = boost::allow_parallel_edge_tag;
    struct traversal_category
        : virtual boost::incidence_graph_tag
        , virtual boost::bidirectional_graph_tag
        , virtual boost::adjacency_graph_tag
        , virtual boost::vertex_list_graph_tag
        , virtual boost::edge_list_graph_tag {};

    constexpr static vertex_descriptor null_vertex() noexcept {
        return std::numeric_limits<vertex_descriptor>::max();
    }

    // IncidenceGraph
    using out_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using out_edge_iterator = Impl::OutEdgeIter<
        std::pmr::vector<out_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;
    using degree_size_type = uint32_t;

    // BidirectionalGraph
    using in_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using in_edge_iterator = Impl::InEdgeIter<
        std::pmr::vector<in_edge_type>::iterator,
        vertex_descriptor, edge_descriptor, int32_t>;

    // AdjacencyGraph
    using adjacency_iterator = boost::adjacency_iterator_generator<
        ModuleGraph, vertex_descriptor, out_edge_iterator>::type;

    // VertexListGraph
    using vertex_iterator = boost::integer_range<vertex_descriptor>::iterator;
    using vertices_size_type = uint32_t;

    // VertexList help functions
    inline std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mOutEdges;
    }
    inline const std::pmr::vector<out_edge_type>& out_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mOutEdges;
    }

    inline std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) noexcept {
        return mVertices[v].mInEdges;
    }
    inline const std::pmr::vector<in_edge_type>& in_edge_list(vertex_descriptor v) const noexcept {
        return mVertices[v].mInEdges;
    }

    inline boost::integer_range<vertex_descriptor> vertex_set() const noexcept {
        return boost::integer_range<vertex_descriptor>(0, gsl::narrow_cast<vertices_size_type>(mVertices.size()));
    }

    inline vertex_descriptor current_id() const noexcept {
        return gsl::narrow_cast<vertex_descriptor>(mVertices.size());
    }

    [[nodiscard]] inline std::pmr::vector<boost::default_color_type> colors(std::pmr::memory_resource* mr) const {
        return std::pmr::vector<boost::default_color_type>{ mVertices.size(), mr };
    }

    // EdgeListGraph
    using edge_iterator = Impl::DirectedEdgeIterator<vertex_iterator, out_edge_iterator, ModuleGraph>;
    using edges_size_type = uint32_t;

    // AddressableGraph (Separated)
    using ownership_descriptor = Impl::EdgeDescriptor<boost::bidirectional_tag, vertex_descriptor>;

    using children_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using children_iterator = Impl::OutEdgeIter<
        std::pmr::vector<children_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;
    using children_size_type = uint32_t;

    using parent_edge_type = Impl::StoredEdge<vertex_descriptor>;
    using parent_iterator = Impl::InEdgeIter<
        std::pmr::vector<parent_edge_type>::iterator,
        vertex_descriptor, ownership_descriptor, int32_t>;

    using ownership_iterator = Impl::OwnershipIterator<vertex_iterator, children_iterator, ModuleGraph>;
    using ownerships_size_type = edges_size_type;

    // AddressableGraph help functions
    inline std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) noexcept {
        return mObjects[v].mChildren;
    }
    inline const std::pmr::vector<children_edge_type>& children_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mChildren;
    }

    inline std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) noexcept {
        return mObjects[v].mParents;
    }
    inline const std::pmr::vector<parent_edge_type>& parents_list(vertex_descriptor v) const noexcept {
        return mObjects[v].mParents;
    }

    // ContinuousContainer
    void reserve(vertices_size_type sz);

    // Members
    struct object_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        object_type(const allocator_type& alloc) noexcept;
        object_type(object_type&& rhs, const allocator_type& alloc);
        object_type(object_type const& rhs, const allocator_type& alloc);

        object_type(object_type&& rhs) noexcept;
        object_type(object_type const& rhs) = delete;
        object_type& operator=(object_type&& rhs);
        object_type& operator=(object_type const& rhs);
        ~object_type() noexcept;

        std::pmr::vector<children_edge_type> mChildren;
        std::pmr::vector<parent_edge_type> mParents;
    };

    struct vertex_type {
        using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
        allocator_type get_allocator() const noexcept;

        vertex_type(const allocator_type& alloc) noexcept;
        vertex_type(vertex_type&& rhs, const allocator_type& alloc);
        vertex_type(vertex_type const& rhs, const allocator_type& alloc);

        vertex_type(vertex_type&& rhs) noexcept;
        vertex_type(vertex_type const& rhs) = delete;
        vertex_type& operator=(vertex_type&& rhs);
        vertex_type& operator=(vertex_type const& rhs);
        ~vertex_type() noexcept;

        std::pmr::vector<out_edge_type> mOutEdges;
        std::pmr::vector<in_edge_type> mInEdges;
    };

    struct names_ {
    } static constexpr names = {};
    struct modules_ {
    } static constexpr modules = {};

    // Owners
    std::pmr::vector<object_type> mObjects;
    // Vertices
    std::pmr::vector<vertex_type> mVertices;
    // Components
    std::pmr::vector<std::pmr::string> mNames;
    std::pmr::vector<ModuleInfo> mModules;
    // Path
    PmrMap<std::pmr::string, uint32_t> mPathIndex;
};

} // namespace Meta

} // namespace Cocos
