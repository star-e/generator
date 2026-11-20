module PrivateInterface;
import EngineTypes;
import Containers;
import Strings;
import GfxDescriptorSet;
import GfxResource;
import PipelineExternals;

namespace cc::render {
    interface ProgramProxy : RefCounted {
        virtual const ccstd::string& getName() const noexcept = 0;
        virtual gfx::Shader* getShader() const noexcept = 0;
    }

    interface ProgramLibrary {
        virtual void addEffect(const EffectAsset* effectAsset) = 0;
        virtual void precompileEffect(gfx::Device* device, EffectAsset* effectAsset) = 0;
        virtual ccstd::string getKey(uint32_t phaseID, const ccstd::string& programName, const MacroRecord& defines) const = 0;
        virtual IntrusivePtr<gfx::PipelineLayout> getPipelineLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName) = 0;
        virtual const gfx::DescriptorSetLayout& getMaterialDescriptorSetLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName) = 0;
        virtual const gfx::DescriptorSetLayout& getLocalDescriptorSetLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName) = 0;
        virtual const IProgramInfo& getProgramInfo(uint32_t phaseID, const ccstd::string& programName) const = 0;
        virtual const gfx::ShaderInfo& getShaderInfo(uint32_t phaseID, const ccstd::string& programName) const = 0;
        virtual ProgramProxy* getProgramVariant(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines, const ccstd::pmr::string* key) = 0;
        @cppOnly virtual gfx::PipelineState* getComputePipelineState(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines, const ccstd::pmr::string* key) = 0;
        virtual const ccstd::vector<int>& getBlockSizes(uint32_t phaseID, const ccstd::string& programName) const = 0;
        virtual const ccstd::unordered_map<ccstd::string, uint32_t>& getHandleMap(uint32_t phaseID, const ccstd::string& programName) const = 0;
        virtual uint32_t getProgramID(uint32_t phaseID, const ccstd::pmr::string& programName) = 0;
        virtual uint32_t getDescriptorNameID(const ccstd::pmr::string& name) = 0;
        virtual const ccstd::pmr::string& getDescriptorName(uint32_t nameID) = 0;
        ProgramProxy* getProgramVariant(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines) { return getProgramVariant(device, phaseID, name, defines, nullptr); }
        gfx::PipelineState* getComputePipelineState(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines) { return getComputePipelineState(device, phaseID, name, defines, nullptr); }
    }
}

binding typescript {
    ProgramProxy => interface {
        getter getName => name;
        getter getShader => shader;
    };
    ProgramLibrary => interface {
        method getProgramVariant optional(key) nullable;
    };
}
