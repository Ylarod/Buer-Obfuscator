#ifndef OBFUSCATION_OBFUSCATIONOPTIONS_H
#define OBFUSCATION_OBFUSCATIONOPTIONS_H

#include <llvm/Support/YAMLParser.h>
#include <set>

namespace llvm {

    struct PassHelloWorld{
        int enable;
    };

    struct ObfuscationOptions {
        explicit ObfuscationOptions(const Twine &FileName);

        explicit ObfuscationOptions() = default;

        bool skipFunction(const Twine &FName);

        void dump() const;

        PassHelloWorld HelloWorld{};

    private:
        void handleRoot(yaml::Node *n);

        void handleHelloWorld(yaml::MappingNode *n);

        bool parseOptions(const Twine &FileName);

        void loadCommandLineArgs();

        void checkOptions() const;

        std::set<std::string> FunctionFilter;

        std::map<std::string, bool> PassEnableList;
    };

}

#endif
