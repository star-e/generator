module EngineTypes;

binding typescript {
    cc::gfx::SampleCount => "gfx.SampleCount";
    cc::Material => "Material";
    cc::Texture2D => "Texture2D";
    std::string_view => "string";
    ccstd::pmr::string => "string";
}
