module RenderSettings;
namespace cc::render {
    struct MSAA {
        enabled: bool = false;
        sampleCount: gfx::SampleCount = gfx::SampleCount::X4;
    }

    struct ForwardPassConfigs {
        enableMainLightShadowMap: bool = false;
        enableMainLightPlanarShadowMap: bool = false;
        enablePlanarReflectionProbe: bool = false;
        enableMSAA: bool = false;
        enableSingleForwardPass: bool = false;
    }

    struct HBAO {
        enabled: bool = false;
        radiusScale: float = 1;
        angleBiasDegree: float = 10;
        blurSharpness: float = 3;
        aoSaturation: float = 1;
        needBlur: bool = false;
    }

    enum class BloomType {
        KawaseDualFilter,
        MipmapFilter,
    }

    struct Bloom {
        enabled: bool = false;
        type: BloomType = BloomType::KawaseDualFilter;
        nullable material: IntrusivePtr<Material> = null;
        nullable kawaseFilterMaterial: IntrusivePtr<Material> = null;
        nullable mipmapFilterMaterial: IntrusivePtr<Material> = null;
        enableAlphaMask: bool = false;
        iterations: uint32_t = 3;
        threshold: float = 0.8;
        intensity: float = 1;
    }

    struct ColorGrading {
        enabled: bool = false;
        nullable material: IntrusivePtr<Material> = null;
        contribute: float = 1;
        nullable colorGradingMap: IntrusivePtr<Texture2D> = null;
    }

    struct FSR {
        enabled: bool = false;
        nullable material: IntrusivePtr<Material> = null;
        sharpness: float = 0.8;
    }

    struct FXAA {
        enabled: bool = false;
        nullable material: IntrusivePtr<Material> = null;
    }

    struct ToneMapping {
        nullable material: IntrusivePtr<Material> = null;
    }

    struct PipelineSettings {
        readonly msaa: MSAA;
        enableShadingScale: bool = false;
        shadingScale: float = 0.5;
        readonly bloom: Bloom;
        readonly toneMapping: ToneMapping;
        readonly colorGrading: ColorGrading;
        readonly fsr: FSR;
        readonly fxaa: FXAA;
    }
}
