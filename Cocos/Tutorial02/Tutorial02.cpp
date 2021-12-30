#include <Cocos/AST/SyntaxGraphs.h>
#include <Cocos/AST/BuilderTypes.h>
#include <Cocos/AST/CppDefaultValues.h>
#include <Cocos/AST/TypescriptDefaultValues.h>
#include <Cocos/AST/DSL.h>

using namespace Cocos;
using namespace Cocos::Meta;

int main() {
    ModuleBuilder builder(std::pmr::get_default_resource());
    addCppDefaultValues(builder);
    projectTypescriptDefaultValues(builder);

    MODULE(Tutorial,
        .mAPI = "CC_API",
        .mFolder = "../../examples/tutorial",
        .mFilePrefix = "tutorial",
    ) {
        NAMESPACE(cc) {
            ENUM(TransversalEnum, (DepthFirst, BreadthFirst));

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

            GRAPH(SceneGraph, _, _) {
                NAMED_GRAPH();
                REFERENCE_GRAPH();
                ADDRESSABLE_GRAPH();

                COMPONENT_GRAPH(
                    (node, Node, mNodes)
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

    builder.outputModule(".", "Tutorial", Typescripts);
}
