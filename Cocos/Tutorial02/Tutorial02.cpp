#include <Cocos/AST/SyntaxGraphs.h>
#include <Cocos/AST/BuilderTypes.h>
#include <Cocos/AST/CppDefaultValues.h>
#include <Cocos/AST/TypescriptDefaultValues.h>
#include <Cocos/FileUtils.h>
#include <Cocos/AST/DSL.h>

using namespace Cocos;
using namespace Cocos::Meta;

int main() {
    std::filesystem::path outputFolder("../..");
    ModuleBuilder builder("cocos", outputFolder, outputFolder,
        std::pmr::get_default_resource(), std::pmr::get_default_resource());
    addCppDefaultValues(builder);
    projectTypescriptDefaultValues(builder);

    const auto features = Features::Typescripts;

    MODULE(Tutorial,
        .mTypescriptFolder = "examples/tutorial",
        .mTypescriptFilePrefix = "tutorial",
        .mAPI = "CC_API"
    ) {
        NAMESPACE(cc) {
            ENUM(TransversalEnum) {
                ENUMS(DepthFirst, BreadthFirst);
            }

            TAGS((_), Sphere_, Box_, Mesh_, Light_);

            VARIANT(NodeType, (Sphere_, Box_, Mesh_, Light_), LESS);

            STRUCT(Node) {
                PUBLIC(
                    (std::string, mContent, _)
                    (uint32_t, mFlags, 0)
                );
            }
            
            STRUCT(Sphere) {
                PUBLIC(
                    (float, mX, 0)
                    (float, mY, 0)
                    (float, mZ, 0)
                    (float, mRadius, 1)
                );
            }

            STRUCT(Box) {
                PUBLIC(
                    (float, mX, 0)
                    (float, mY, 0)
                    (float, mZ, 0)
                    (float, mSizeX, 1)
                    (float, mSizeY, 1)
                    (float, mSizeZ, 1)
                );
            }

            STRUCT(Mesh) {
                PUBLIC(
                    (std::string, mAssetPath, _)
                );
            }

            STRUCT(Light) {
                PUBLIC(
                    (float, mX, 0)
                    (float, mY, 0)
                    (float, mZ, 0)
                    (float, mDirX, 1)
                    (float, mDirY, 0)
                    (float, mDirZ, 0)
                );
            }

            PMR_GRAPH(SceneGraph, _, _) {
                NAMED_GRAPH(Name_);
                REFERENCE_GRAPH();
                ADDRESSABLE_GRAPH(mPathIndex);

                COMPONENT_GRAPH(
                    (Name_, std::pmr::string, mName)
                    (Node_, Node, mNodes)
                );

                POLYMORPHIC_GRAPH(
                    (Sphere_, Sphere, mSpheres)
                    (Box_, Box, mBoxes)
                    (Mesh_, Mesh, mMeshes)
                    (Light_, Light, mLights)
                );
            }
        }
    }

    // copy graph interface
    {
        auto content = readFile("../CocosBuilder/graph.ts");
        updateFile(outputFolder / "examples/tutorial/graph.ts", content);
    }

    builder.compile();

    std::pmr::set<std::pmr::string> files(std::pmr::get_default_resource());
    builder.outputModule("Tutorial", files);
}
