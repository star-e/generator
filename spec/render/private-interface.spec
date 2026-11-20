module PrivateInterface;
import EngineTypes;
import Containers;
import Strings;
import GfxDescriptorSet;
import GfxResource;
import PipelineExternals;

namespace cc::render {
    interface ProgramProxy : RefCounted {
        const ccstd::string& getName() const noexcept;
        gfx::Shader* getShader() const noexcept;
    }

    interface ProgramLibrary {
        void addEffect(const EffectAsset* effectAsset);
        void precompileEffect(gfx::Device* device, EffectAsset* effectAsset);
        ccstd::string getKey(uint32_t phaseID, const ccstd::string& programName, const MacroRecord& defines) const;
        IntrusivePtr<gfx::PipelineLayout> getPipelineLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName);
        const gfx::DescriptorSetLayout& getMaterialDescriptorSetLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName);
        const gfx::DescriptorSetLayout& getLocalDescriptorSetLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName);
        const IProgramInfo& getProgramInfo(uint32_t phaseID, const ccstd::string& programName) const;
        const gfx::ShaderInfo& getShaderInfo(uint32_t phaseID, const ccstd::string& programName) const;
        ProgramProxy* getProgramVariant(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines, const ccstd::pmr::string* key);
        @cppOnly gfx::PipelineState* getComputePipelineState(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines, const ccstd::pmr::string* key);
        const ccstd::vector<int>& getBlockSizes(uint32_t phaseID, const ccstd::string& programName) const;
        const ccstd::unordered_map<ccstd::string, uint32_t>& getHandleMap(uint32_t phaseID, const ccstd::string& programName) const;
        uint32_t getProgramID(uint32_t phaseID, const ccstd::pmr::string& programName);
        uint32_t getDescriptorNameID(const ccstd::pmr::string& name);
        const ccstd::pmr::string& getDescriptorName(uint32_t nameID);
        fn getProgramVariant(device: gfx::Device*, phaseID: uint32_t, name: const ccstd::string&, defines: MacroRecord&) -> ProgramProxy* => "getProgramVariant(device, phaseID, name, defines, nullptr)";
        fn getComputePipelineState(device: gfx::Device*, phaseID: uint32_t, name: const ccstd::string&, defines: MacroRecord&) -> gfx::PipelineState* => "getComputePipelineState(device, phaseID, name, defines, nullptr)";
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
