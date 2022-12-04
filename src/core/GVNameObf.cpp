//
// Created by Ylarod on 2022/12/4.
//

#include "core/GVNameObf.h"
#include "utils/Utils.h"
#include "utils/CryptoUtils.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <sstream>

using namespace llvm;

PreservedAnalyses GVNameObf::run(Module &M, ModuleAnalysisManager &) const {
    PassNameObf& config = Options->GVNameObf;
    if (!config.enable){
        return PreservedAnalyses::all();
    }
    for (auto &GV: M.globals()) {
        if (!toObfuscate(config.enable, &GV, "gvn") AND_VERBOSE2){
            outs() << fmt::format(fmt::fg(fmt::color::red),
                                  "GVNameObf: Ignore {}\n", GV.getName().str());
            continue;
        }

        StringRef name = GV.getName();
        if (name.startswith("_") || name.contains(".") || name.empty()){
            continue;
        }
        if (!GV.getSection().empty()){ // 有自定义section的也不混淆
            continue;
        }

        std::stringstream ss;
        ss << config.prefix;
        uint32_t range = config.charset.length();
        for (int i = 0; i < config.length; i++) {
            ss << config.charset[crypto->get_uint32_t() % range];
        }
        ss << config.suffix;

        StringRef origName = GV.getName();

        GV.setName(Twine(ss.str())); // 重命名

        StringRef newName = GV.getName();
        IF_VERBOSE{
            outs() << fmt::format(fmt::fg(fmt::color::sky_blue),
                                  "GVNameObf: {} => {}\n", origName.str(), newName.str());
            outs() << GV.getSection() << "\n";
        }
    }
    return PreservedAnalyses::all();
}
