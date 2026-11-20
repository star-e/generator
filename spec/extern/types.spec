module EngineTypes;
import Containers;

binding typescript {
    cc::Material => Material from "cocos/asset/assets";
    cc::Texture2D => "Texture2D";
    std::string_view => "string";
    ccstd::vector<uint8_t> => Uint8Array;
}
