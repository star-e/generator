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

const auto* const gNameVertexTypeIndex = "w";
const auto* const gNameVertexHolds = "h";

const auto* const gNameGraphSource = "source";
const auto* const gNameGraphTarget = "target";

const auto* const gNameOutEdges = "oe";
const auto* const gNameOutDegree = "od";

const auto* const gNameInEdges = "ie";
const auto* const gNameInDegree = "id";

const auto* const gNameDegree = "d";

const auto* const gNameAdjacentVertices = "adj";

const auto* const gNameGetVertices = "v";
const auto* const gNameNumVertices = "nv";

} // namespace Cocos::Meta
