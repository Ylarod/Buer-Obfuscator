#include "ObfuscationOptions.h"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>
#include <string>

using namespace llvm;

namespace llvm {

    cl::opt<int> HelloWorldEnable("hello", cl::init(0), cl::desc("Enable the HelloWorld pass"));

    ObfuscationOptions::ObfuscationOptions(const Twine &FileName) {
        if (sys::fs::exists(FileName)) {
            parseOptions(FileName);
        }
        loadCommandLineArgs();
        checkOptions();
    }

    void ObfuscationOptions::loadCommandLineArgs(){
        if (HelloWorldEnable.hasArgStr()){
            HelloWorld.enable = HelloWorldEnable;
        }
    }

    void ObfuscationOptions::checkOptions() const {
        if (HelloWorld.enable > 2 || HelloWorld.enable < 0){
            errs() << "\033[1;31m" << "HelloWorld.enable: 值只能为 0(关闭) 1(白名单模式) 2(黑名单模式) 之一\n" << "\033[0m";
            abort();
        }
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
            if (K == "enable"){
                HelloWorld.enable = static_cast<bool>(getIntVal(i.getValue()));
            }
        }
    }

    void ObfuscationOptions::handleRoot(yaml::Node *n) {
        if (!n)
            return;
        if (auto *mn = dyn_cast<yaml::MappingNode>(n)) {
            for (auto &i: *mn) {
                StringRef K = getNodeString(i.getKey());
                if (K == "HelloWorld"){
                    handleHelloWorld(dyn_cast<yaml::MappingNode>(i.getValue()));
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
        dbgs() << "\033[1;31m" << "ObfuscationOptions:\n" << "\033[0m";
        dbgs() << "\033[1;31m" << "HelloWorld:\n" << "\033[0m";
        dbgs() << "\033[1;34m" << "\tEnable: " << "\033[1;36m" << HelloWorld.enable << "\n" << "\033[0m";
    }

}
