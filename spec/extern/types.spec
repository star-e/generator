module EngineTypes;
import Containers;

binding typescript {
    cc::gfx::SampleCount => "gfx.SampleCount";
    cc::Material => "Material";
    cc::Texture2D => "Texture2D";
    std::string_view => "string";
    ccstd::vector<uint8_t> => Uint8Array;
}
