//
// Created by Ylarod on 2022/12/4.
//

#include "core/FuncNameObf.h"
#include "utils/Utils.h"
#include "utils/CryptoUtils.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <sstream>

using namespace llvm;

PreservedAnalyses FuncNameObf::run(Module &M, ModuleAnalysisManager &) const {
    PassNameObf& config = Options->FuncNameObf;
    if (!config.enable){
        return PreservedAnalyses::all();
    }
    for (auto &F: M) {
        if (!toObfuscate(config.enable, &F, "fno") AND_VERBOSE2){
            outs() << fmt::format(fmt::fg(fmt::color::red),
                                  "FuncNameObf: Ignore {}\n",F.getName().str());
            continue;
        }



        std::stringstream ss;
        ss << config.prefix;
        uint32_t range = config.charset.length();
        for (int i = 0; i < config.length; i++) {
            ss << config.charset[crypto->get_uint32_t() % range];
        }
        ss << config.suffix;

        StringRef origName = F.getName();

        F.setName(Twine(ss.str())); // 重命名

        StringRef newName = F.getName();
        IF_VERBOSE{
            outs() << fmt::format(fmt::fg(fmt::color::sky_blue),
                                  "FuncNameObf: {} => {}\n", origName.str(), newName.str());
        }
    }
    return PreservedAnalyses::all();
}
