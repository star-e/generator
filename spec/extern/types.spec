module EngineTypes;

binding typescript {
    void => "void";
    bool => "boolean";
    double => "number";
    float => "number";
    uint8_t => "number";
    uint32_t => "number";
    cc::gfx::SampleCount => "gfx.SampleCount";
    cc::Material => "Material";
    cc::Texture2D => "Texture2D";
    std::string_view => "string";
    ccstd::pmr::string => "string";
}
