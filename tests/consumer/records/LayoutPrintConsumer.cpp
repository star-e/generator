#include "LayoutGraphPrint.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>
#include <sstream>

bool printLayoutArchive(const char* inputPath, const char* outputPath) {
    using namespace cc::render;
    boost::container::pmr::monotonic_buffer_resource arena;
    std::ifstream input(inputPath, std::ios::binary);
    input.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(input, &arena);
    LayoutGraphData graph{&arena};
    std::ostringstream empty;
    printLayoutGraphData(graph, empty, &arena);
    if (empty.str() != "\n") return false;
    load(archive, graph);
    if (input.peek() != std::char_traits<char>::eof()) return false;
    std::ofstream output(outputPath, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    printLayoutGraphData(graph, output, &arena);
    output.close();
    return true;
}
