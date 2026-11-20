#include "LayoutGraphPrint.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphNames.h"
#include <ostream>

namespace cc::render {
namespace {

void formatVisibility(gfx::ShaderStageFlagBit visibility, ccstd::pmr::string& text) {
    using Stage = gfx::ShaderStageFlagBit;
    struct StageName { Stage flag; const char* name; };
    static constexpr StageName stages[] = {
        {Stage::VERTEX, "Vertex"}, {Stage::CONTROL, "Control"}, {Stage::EVALUATION, "Evaluation"},
        {Stage::GEOMETRY, "Geometry"}, {Stage::FRAGMENT, "Fragment"}, {Stage::COMPUTE, "Compute"}, {Stage::ALL, "All"},
    };
    text.clear();
    for (const auto& stage : stages) {
        if (hasAllFlags(visibility, stage.flag)) {
            if (!text.empty()) text += " | ";
            text += stage.name;
        }
    }
}

void printBlock(const LayoutGraphData& graph, const DescriptorBlockData& block, std::ostream& out,
                ccstd::pmr::string& visibility) {
    formatVisibility(block.visibility, visibility);
    out << "        Block<" << getName(block.type) << ", " << visibility << "> {\n";
    out << "            capacity: " << block.capacity << ",\n";
    out << "            count: " << block.descriptors.size() << ",\n";
    if (!block.descriptors.empty()) {
        out << "            Descriptors{ ";
        const char* separator = "";
        for (const auto& descriptor : block.descriptors) {
            out << separator << '"' << graph.valueNames.at(descriptor.descriptorID.value);
            if (descriptor.count != 1) out << '[' << descriptor.count << ']';
            out << '"';
            separator = ", ";
        }
        out << " }\n";
    }
    out << "        }\n";
}

} // namespace

void printLayoutGraphData(const LayoutGraphData& graph, std::ostream& out,
                          boost::container::pmr::memory_resource* scratch) {
    ccstd::pmr::string visibility(scratch);
    out << '\n';
    for (const auto vertex : graph.vertices()) {
        if (graph.get_parent(vertex) != graph.null_vertex()) continue;
        const auto* tag = graph.holds<RenderStage_>(vertex) ? getName(RenderStage_{}) : getName(RenderPhase_{});
        out << '"' << graph.getName(vertex) << "\": " << tag << " {\n";
        for (const auto& [frequency, set] : graph.getLayout(vertex).descriptorSets) {
            out << "    Set<" << getName(frequency) << "> {\n";
            for (const auto& block : set.descriptorSetLayoutData.descriptorBlocks) printBlock(graph, block, out, visibility);
            out << "    }\n";
        }
        out << "}\n";
    }
}

} // namespace cc::render
