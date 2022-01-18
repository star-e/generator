#include <Cocos/AST/SyntaxGraphs.h>
#include <Cocos/AST/BuilderTypes.h>
#include <Cocos/AST/CppDefaultValues.h>
#include <Cocos/AST/TypescriptDefaultValues.h>
#include <Cocos/FileUtils.h>
// notice: we do not use DSL in this example

using namespace Cocos;
using namespace Cocos::Meta;

int main() {
    std::filesystem::path outputFolder("../..");
    ModuleBuilder builder("cc", outputFolder, std::pmr::get_default_resource());
    addCppDefaultValues(builder);
    projectTypescriptDefaultValues(builder);

    { // Open Module
        const auto features = Features::Typescripts;

        auto m = builder.openModule("Tutorial",
            ModuleInfo{
                .mFeatures = features,
                .mFolder = "examples/tutorial",
                .mFilePrefix = "tutorial",
                .mAPI = "CC_API",
            });
        {
            // Open Namespace
            auto cc = builder.openNamespace("cc");
            
            // Enum
            auto enumID = builder.addEnum(
                "TransversalEnum",
                Traits {
                    .mClass = true,
                });
            builder.addEnumElement(enumID, "DepthFirst", "_");
            builder.addEnumElement(enumID, "BreadthFirst", "_");
            
            // Tags
            builder.addTag("Sphere_");
            builder.addTag("Box_");
            builder.addTag("Mesh_");
            builder.addTag("Light_");

            // Variant
            auto varID = builder.addVariant("NodeType", LESS);
            builder.addVariantElement(varID, "Sphere_");
            builder.addVariantElement(varID, "Box_");
            builder.addVariantElement(varID, "Mesh_");
            builder.addVariantElement(varID, "Light_");

            // Struct
            { // Node
                auto structHandle = builder.addStruct("Node");
                auto structID = structHandle.mVertexDescriptor;
                builder.addMember(structID, true, "std::string", "mContent", "_");
                builder.addMember(structID, true, "uint32_t", "mFlags", "0");
            }
            { // Sphere
                auto structHandle = builder.addStruct("Sphere");
                auto structID = structHandle.mVertexDescriptor;
                builder.addMember(structID, true, "float", "mX", "0");
                builder.addMember(structID, true, "float", "mY", "0");
                builder.addMember(structID, true, "float", "mZ", "0");
                builder.addMember(structID, true, "float", "mRadius", "1");
            }
            { // Box
                auto structHandle = builder.addStruct("Box");
                auto structID = structHandle.mVertexDescriptor;
                builder.addMember(structID, true, "float", "mX", "0");
                builder.addMember(structID, true, "float", "mY", "0");
                builder.addMember(structID, true, "float", "mZ", "0");
                builder.addMember(structID, true, "float", "mSizeX", "1");
                builder.addMember(structID, true, "float", "mSizeY", "1");
                builder.addMember(structID, true, "float", "mSizeZ", "1");
            }
            { // Mesh
                auto structHandle = builder.addStruct("Mesh");
                auto structID = structHandle.mVertexDescriptor;
                builder.addMember(structID, true, "std::string", "mAssetPath", "_");
            }
            { // Light
                auto structHandle = builder.addStruct("Light");
                auto structID = structHandle.mVertexDescriptor;
                builder.addMember(structID, true, "float", "mX", "0");
                builder.addMember(structID, true, "float", "mY", "0");
                builder.addMember(structID, true, "float", "mZ", "0");
                builder.addMember(structID, true, "float", "mDirX", "1");
                builder.addMember(structID, true, "float", "mDirY", "0");
                builder.addMember(structID, true, "float", "mDirZ", "0");
            }

            // Graph
            {
                auto graphHandle = builder.addGraph("SceneGraph", "_", "_",
                    Traits{});
                auto graphID = graphHandle.mVertexDescriptor;
                auto& g = get_by_tag<Graph_>(graphID, builder.mSyntaxGraph);
                g.mNamed = true;
                g.mReferenceGraph = true;
                g.mAddressable = true;

                builder.addGraphComponent(graphID, "node", "Node", "mNodes");

                builder.addGraphPolymorphic(graphID, "Sphere_", "Sphere", "mSpheres");
                builder.addGraphPolymorphic(graphID, "Box_", "Box", "mBoxes");
                builder.addGraphPolymorphic(graphID, "Mesh_", "Mesh", "mMeshes");
                builder.addGraphPolymorphic(graphID, "Light_", "Light", "mLights");
            }
        }
    }

    // copy graph interface
    {
        auto content = readFile("../CocosBuilder/graph.ts");
        updateFile(outputFolder / "examples/tutorial/graph.ts", content);
    }

    builder.outputModule("Tutorial");
}
