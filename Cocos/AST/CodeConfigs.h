#pragma once

namespace Cocos::Meta {

static constexpr bool sReduceTypescriptMemberFunction = true;
static constexpr bool sEnablePoolSettings = false;
static constexpr bool gReduceCode = true;
const auto* const gNameVertices = "x";
const auto* const gNameOutEdgeList = "o";
const auto* const gNameInEdgeList = "i";
const auto* const gNameChildrenList = "c";
const auto* const gNameParentsList = "p";
const auto* const gNamePolymorphicID = "t";
const auto* const gNameObject = "j";
const auto* const gNameGetComponent = "m";

const auto* const gNameGraphSource = "source";
const auto* const gNameGraphTarget = "target";

const auto* const gNameOutEdges = "outEdges";
const auto* const gNameOutDegree = "outDegree";

const auto* const gNameInEdges = "inEdges";
const auto* const gNameInDegree = "inDegree";

} // namespace Cocos::Meta
