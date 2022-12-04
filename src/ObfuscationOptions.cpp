#include "ObfuscationOptions.h"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>
#include "utils/CryptoUtils.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <string>
#include <sstream>

using namespace llvm;

namespace llvm {

    static cl::opt<int> HelloWorldEnable("hello", cl::init(0), cl::desc("Enable the HelloWorld pass"));
    static cl::opt<std::string> RandomSeed("obf-seed", cl::init(""),
                                           cl::desc("random seed, 32bit hex, 0x is accepted"), cl::Optional);
    static cl::opt<bool> Verbose("obf-verbose", cl::init(false), cl::desc("Print obf log"));

    // 函数名混淆
    static cl::opt<int> FuncNameObfEnable("obf-fn", cl::init(0), cl::desc("Enable the FunctionNameObf pass"));
    static cl::opt<std::string> FuncNameObfPrefix("obf-fn-p", cl::init("Buer_"),
                                                  cl::desc("Custom prefix"), cl::Optional);
    static cl::opt<std::string> FuncNameObfSuffix("obf-fn-s", cl::init(""),
                                                  cl::desc("Custom suffix"), cl::Optional);
    static cl::opt<std::string> FuncNameObfChars("obf-fn-c", cl::init("oO0"),
                                                 cl::desc("Custom obf charset"), cl::Optional);
    static cl::opt<int> FuncNameObfLength("obf-fn-l", cl::init(32),
                                          cl::desc("Custom length"), cl::Optional);

    // 全局变量名混淆
    static cl::opt<int> GVNameObfEnable("obf-gvn", cl::init(0), cl::desc("Enable the GlobalVariableNameObf pass"));
    static cl::opt<std::string> GVNameObfPrefix("obf-gvn-p", cl::init("Buer_"),
                                                cl::desc("Custom prefix"), cl::Optional);
    static cl::opt<std::string> GVNameObfSuffix("obf-gvn-s", cl::init(""),
                                                cl::desc("Custom suffix"), cl::Optional);
    static cl::opt<std::string> GVNameObfChars("obf-gvn-c", cl::init("iIl1"),
                                               cl::desc("Custom obf charset"), cl::Optional);
    static cl::opt<int> GVNameObfLength("obf-gvn-l", cl::init(32),
                                        cl::desc("Custom length"), cl::Optional);

    // 函数包装器
    static cl::opt<int> FunctionWrapperEnable("obf-fw", cl::init(0),
                                              cl::desc("Enable the FunctionWrapper pass"));
    static cl::opt<int> FunctionWrapperProb("obf-fw-p", cl::init(100),
                                            cl::desc("Obfuscate probability [%]"), cl::Optional);
    static cl::opt<int> FunctionWrapperTimes("obf-fw-t", cl::init(5),
                                             cl::desc("Obfuscate times"), cl::Optional);


    ObfuscationOptions::ObfuscationOptions() { // 获取home目录失败才执行
        loadCommandLineArgs();
        checkOptions();
    }

    ObfuscationOptions::ObfuscationOptions(const Twine &FileName) {
        if (sys::fs::exists(FileName)) {
            parseOptions(FileName);
        }
        loadCommandLineArgs();
        checkOptions();
    }

    void ObfuscationOptions::loadCommandLineArgs() {
        if (Verbose.getNumOccurrences()) {
            verbose = Verbose;
        }
        if (HelloWorldEnable.getNumOccurrences()) {
            HelloWorld.enable = HelloWorldEnable;
        }
        if (RandomSeed.getNumOccurrences()) {
            crypto->prng_seed(RandomSeed);
        } else {
            crypto->prng_seed();
        }
        // 函数名混淆
        if (FuncNameObfEnable.getNumOccurrences()) {
            FuncNameObf.enable = FuncNameObfEnable;
        }
        if (FuncNameObfPrefix.getNumOccurrences()) {
            FuncNameObf.prefix = FuncNameObfPrefix;
        }
        if (FuncNameObfSuffix.getNumOccurrences()) {
            FuncNameObf.suffix = FuncNameObfSuffix;
        }
        if (FuncNameObfChars.getNumOccurrences()) {
            FuncNameObf.charset = FuncNameObfChars;
        }
        if (FuncNameObfLength.getNumOccurrences()) {
            FuncNameObf.length = FuncNameObfLength;
        }
        // 全局变量名混淆
        if (GVNameObfEnable.getNumOccurrences()) {
            GVNameObf.enable = GVNameObfEnable;
        }
        if (GVNameObfPrefix.getNumOccurrences()) {
            GVNameObf.prefix = GVNameObfPrefix;
        }
        if (GVNameObfSuffix.getNumOccurrences()) {
            GVNameObf.suffix = GVNameObfSuffix;
        }
        if (GVNameObfChars.getNumOccurrences()) {
            GVNameObf.charset = GVNameObfChars;
        }
        if (GVNameObfLength.getNumOccurrences()) {
            GVNameObf.length = GVNameObfLength;
        }
        // 函数包装器
        if (FunctionWrapperEnable.getNumOccurrences()) {
            FunctionWrapper.enable = FunctionWrapperEnable;
        }
        if (FunctionWrapperProb.getNumOccurrences()) {
            FunctionWrapper.prob = FunctionWrapperProb;
        }
        if (FunctionWrapperTimes.getNumOccurrences()) {
            FunctionWrapper.times = FunctionWrapperTimes;
        }
    }

    void ObfuscationOptions::checkOptions() const {
        auto red = fmt::fg(fmt::color::red);
#define echo_err(msg) errs() << fmt::format(red, msg)
#define check_enable(value, name) do { \
        if (value > 2 || value < 0){ \
            echo_err(name ".enable: 值只能为 0(关闭) 1(白名单模式) 2(黑名单模式) 之一\n"); \
            abort(); \
        } \
        } while (false)

        check_enable(HelloWorld.enable, "HelloWorld");
        check_enable(FuncNameObf.enable, "FunctionNameObf");
        check_enable(GVNameObf.enable, "GlobalVariableNameObf");
        check_enable(FunctionWrapper.enable, "FunctionWrapper");
#undef echo_err
#undef check_enable
    }

    static StringRef getNodeString(yaml::Node *n) {
        if (auto *sn = dyn_cast<yaml::ScalarNode>(n)) {
            SmallString<32> Storage;
            StringRef Val = sn->getValue(Storage);
            return Val;
        } else {
            return "";
        }
    }

    static unsigned long getIntVal(yaml::Node *n) {
        return strtoul(getNodeString(n).str().c_str(), nullptr, 10);
    }

    static std::set<std::string> getStringList(yaml::Node *n) {
        std::set<std::string> filter;
        if (auto *sn = dyn_cast<yaml::SequenceNode>(n)) {
            for (auto i = sn->begin(), e = sn->end(); i != e; ++i) {
                filter.insert(getNodeString(i).str());
            }
        }
        return filter;
    }

    bool ObfuscationOptions::skipFunction(const Twine &FName) {
        if (FName.str().find(".buer_") == std::string::npos) {
            return FunctionFilter.count(FName.str()) == 0;
        } else {
            return true;
        }
    }

    void ObfuscationOptions::handleHelloWorld(yaml::MappingNode *n) {
        for (auto &i: *n) {
            StringRef K = getNodeString(i.getKey());
            if (K == "enable") {
                HelloWorld.enable = static_cast<int>(getIntVal(i.getValue()));
            }
        }
    }

    void ObfuscationOptions::handleGVNameObf(yaml::MappingNode *n) {
        for (auto &i: *n) {
            StringRef K = getNodeString(i.getKey());
            if (K == "enable") {
                GVNameObf.enable = static_cast<int>(getIntVal(i.getValue()));
            } else if (K == "prefix") {
                GVNameObf.prefix = getNodeString(i.getValue()).str();
            } else if (K == "suffix") {
                GVNameObf.suffix = getNodeString(i.getValue()).str();
            } else if (K == "charset") {
                GVNameObf.charset = getNodeString(i.getValue()).str();
            } else if (K == "length") {
                GVNameObf.length = static_cast<int>(getIntVal(i.getValue()));
            }
        }
    }

    void ObfuscationOptions::handleFuncNameObf(yaml::MappingNode *n) {
        for (auto &i: *n) {
            StringRef K = getNodeString(i.getKey());
            if (K == "enable") {
                FuncNameObf.enable = static_cast<int>(getIntVal(i.getValue()));
            } else if (K == "prefix") {
                FuncNameObf.prefix = getNodeString(i.getValue()).str();
            } else if (K == "suffix") {
                FuncNameObf.suffix = getNodeString(i.getValue()).str();
            } else if (K == "charset") {
                FuncNameObf.charset = getNodeString(i.getValue()).str();
            } else if (K == "length") {
                FuncNameObf.length = static_cast<int>(getIntVal(i.getValue()));
            }
        }
    }


    void ObfuscationOptions::handleFunctionWrapper(yaml::MappingNode *n) {
        for (auto &i: *n) {
            StringRef K = getNodeString(i.getKey());
            if (K == "enable") {
                FunctionWrapper.enable = static_cast<int>(getIntVal(i.getValue()));
            } else if (K == "prob") {
                FunctionWrapper.prob = static_cast<int>(getIntVal(i.getValue()));
            } else if (K == "times") {
                FunctionWrapper.times = static_cast<int>(getIntVal(i.getValue()));
            }
        }
    }

    void ObfuscationOptions::handleRoot(yaml::Node *n) {
        if (!n)
            return;
        if (auto *mn = dyn_cast<yaml::MappingNode>(n)) {
            for (auto &i: *mn) {
                StringRef K = getNodeString(i.getKey());
                if (K == "HelloWorld") {
                    handleHelloWorld(dyn_cast<yaml::MappingNode>(i.getValue()));
                } else if (K == "FuncNameObf") {
                    handleFuncNameObf(dyn_cast<yaml::MappingNode>(i.getValue()));
                } else if (K == "GVNameObf") {
                    handleGVNameObf(dyn_cast<yaml::MappingNode>(i.getValue()));
                } else if (K == "FunctionWrapper") {
                    handleFunctionWrapper(dyn_cast<yaml::MappingNode>(i.getValue()));
                }
            }
        }
    }

    bool ObfuscationOptions::parseOptions(const Twine &FileName) {
        ErrorOr<std::unique_ptr<MemoryBuffer>> BufOrErr =
                MemoryBuffer::getFileOrSTDIN(FileName);
        MemoryBuffer &Buf = *BufOrErr.get();

        llvm::SourceMgr sm;

        yaml::Stream stream(Buf.getBuffer(), sm, false);
        for (yaml::document_iterator di = stream.begin(), de = stream.end(); di != de;
             ++di) {
            yaml::Node *n = di->getRoot();
            if (n)
                handleRoot(n);
            else
                break;
        }
        return true;
    }

    void ObfuscationOptions::dump() const {
        auto red = fmt::fg(fmt::color::red);
        auto pink = fmt::fg(fmt::color::pink);
        auto sky_blue = fmt::fg(fmt::color::sky_blue);
        std::stringstream seed_hex;
        auto *seed = (unsigned char *) crypto->get_seed();
        if (seed) {
            seed_hex << "0x";
            for (int i = 0; i < 16; i++) {
                seed_hex << fmt::format("{:02x}", seed[i]);
            }
        } else {
            seed_hex << "Uninitialized";
        }
#define echo_pass(name) dbgs() << fmt::format(red, name ":\n")
#define echo_config(name, ...) dbgs() << fmt::format(pink, "\t" name ": ") << fmt::format(sky_blue, __VA_ARGS__) << "\n"
#define enable_value(value) value == 0 ? "false" : value == 1 ? "true(whitelist)" : "true(blacklist)"
#define echo_enable(value) echo_config("Enable", "{}", enable_value(value))

        echo_pass("ObfuscationOptions");
        echo_pass("Global");
        echo_config("RandomSeed", seed_hex.str());

        echo_pass("HelloWorld");
        echo_enable(HelloWorld.enable);

        echo_pass("FuncNameObf");
        echo_enable(FuncNameObf.enable);
        echo_config("Prefix", "{}", FuncNameObf.prefix);
        echo_config("Suffix", "{}", FuncNameObf.suffix);
        echo_config("Charset", "{}", FuncNameObf.charset);
        echo_config("Length", "{}", FuncNameObf.length);

        echo_pass("GlobalVariableNameObf");
        echo_enable(GVNameObf.enable);
        echo_config("Prefix", "{}", GVNameObf.prefix);
        echo_config("Suffix", "{}", GVNameObf.suffix);
        echo_config("Charset", "{}", GVNameObf.charset);
        echo_config("Length", "{}", GVNameObf.length);

#undef echo_pass
#undef echo_config
#undef echo_enable
    }

}
