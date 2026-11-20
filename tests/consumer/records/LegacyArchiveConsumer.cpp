#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>

using namespace cc::render;

template<class T>
void transcode(BinaryInputArchive& input, BinaryOutputArchive& output, boost::container::pmr::memory_resource& resource) {
    T value{&resource};
    load(input, value);
    save(output, value);
}

int main(int argc, char** argv) {
    if (argc != 3) return 1;
    std::ifstream source(argv[1], std::ios::binary);
    std::ofstream target(argv[2], std::ios::binary);
    source.exceptions(std::ios::failbit | std::ios::badbit);
    target.exceptions(std::ios::failbit | std::ios::badbit);
    boost::container::pmr::monotonic_buffer_resource resource;
    BinaryInputArchive input(source, &resource);
    BinaryOutputArchive output(target, &resource);
    transcode<DescriptorSetData>(input, output, resource);
    transcode<ShaderProgramData>(input, output, resource);
    transcode<EffectData>(input, output, resource);
    transcode<RenderStageData>(input, output, resource);
    transcode<RenderPhaseData>(input, output, resource);
    transcode<LayoutGraphData>(input, output, resource);
    transcode<LayoutGraphData>(input, output, resource);
    target.close();
    return source.peek() == std::char_traits<char>::eof() ? 0 : 1;
}
