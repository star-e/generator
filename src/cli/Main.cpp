#include "workspace/Workspace.h"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
constexpr std::string_view usage =
    "Usage: cocos-codegen generate --workspace FILE [--output DIR] [--check [--module NAME] [--diff]]\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout << usage;
            return 0;
        }
        if (argc < 2 || std::string_view(argv[1]) != "generate") {
            throw std::runtime_error("expected generate");
        }
        std::optional<std::filesystem::path> workspace, output;
        std::optional<std::string_view> module;
        bool check = false, diff = false;
        for (int i = 2; i < argc; ++i) {
            const std::string_view arg = argv[i];
            if (arg == "--workspace" || arg == "--output") {
                auto& path = arg == "--workspace" ? workspace : output;
                if (path || ++i == argc || std::string_view(argv[i]).starts_with("--")) {
                    throw std::runtime_error("invalid " + std::string(arg));
                }
                path = argv[i];
            } else if (arg == "--module") {
                if (module || ++i == argc || !*argv[i] || std::string_view(argv[i]).starts_with("--"))
                    throw std::runtime_error("--module expects one module name");
                module = argv[i];
            } else if (arg == "--check") {
                check = true;
            } else if (arg == "--diff") {
                diff = true;
            } else {
                throw std::runtime_error("unknown option " + std::string(arg));
            }
        }
        if (!workspace || (diff && !check)) {
            throw std::runtime_error(std::string(usage));
        }
        return cc::codegen::generate(*workspace, output, check, diff, module);
    } catch (const std::exception& error) {
        std::cerr << "cocos-codegen: " << error.what() << '\n';
        return 2;
    }
}
