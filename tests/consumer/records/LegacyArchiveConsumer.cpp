#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>
#include <string_view>

using namespace cc::render;

template<class T>
void transcode(BinaryInputArchive& input, BinaryOutputArchive& output, boost::container::pmr::memory_resource& resource) {
    T value{&resource};
    load(input, value);
    save(output, value);
}

bool printLayoutArchive(const char* inputPath, const char* outputPath);
bool checkLayoutDescriptors(const char* inputPath, const char* outputPath);

int main(int argc, char** argv) {
    if (argc != 4) return 1;
    const std::string_view mode{argv[1]};
    if (mode == "layout-descriptors")
        return checkLayoutDescriptors(argv[2], argv[3]) ? 0 : 1;
    if (mode == "layout-print")
        return printLayoutArchive(argv[2], argv[3]) ? 0 : 1;
    if (mode != "layout" && mode != "pipeline") return 1;
    std::ifstream source(argv[2], std::ios::binary);
    std::ofstream target(argv[3], std::ios::binary);
    source.exceptions(std::ios::failbit | std::ios::badbit);
    target.exceptions(std::ios::failbit | std::ios::badbit);
    boost::container::pmr::monotonic_buffer_resource resource;
    BinaryInputArchive input(source, &resource);
    BinaryOutputArchive output(target, &resource);
    if (mode == "layout") {
        transcode<LayoutGraph>(input, output, resource);
        transcode<LayoutGraph>(input, output, resource);
    } else {
        transcode<DescriptorSetData>(input, output, resource);
        transcode<ShaderProgramData>(input, output, resource);
        transcode<EffectData>(input, output, resource);
        transcode<RenderStageData>(input, output, resource);
        transcode<RenderPhaseData>(input, output, resource);
        transcode<LayoutGraphData>(input, output, resource);
        transcode<LayoutGraphData>(input, output, resource);
    }
    target.close();
    return source.peek() == std::char_traits<char>::eof() ? 0 : 1;
}
