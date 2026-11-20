#pragma once

#include <boost/container/pmr/memory_resource.hpp>
#include <iosfwd>
#include "cocos/renderer/pipeline/custom/LayoutGraphFwd.h"

namespace cc::render {

void printLayoutGraphData(const LayoutGraphData& graph, std::ostream& out,
                          boost::container::pmr::memory_resource* scratch);

} // namespace cc::render
