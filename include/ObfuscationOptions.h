#ifndef OBFUSCATION_OBFUSCATIONOPTIONS_H
#define OBFUSCATION_OBFUSCATIONOPTIONS_H

#include <llvm/Support/YAMLParser.h>
#include <set>

namespace llvm {

    struct PassHelloWorld{
        int enable;
    };

    struct PassNameObf{
        int enable;
        std::string prefix;
        std::string suffix;
        std::string charset;
        int length;
    };

    struct ObfuscationOptions {
        explicit ObfuscationOptions(const Twine &FileName);

        explicit ObfuscationOptions();

        bool skipFunction(const Twine &FName);

        void dump() const;

        bool verbose = false;

        PassHelloWorld HelloWorld{};

        PassNameObf FuncNameObf{
            .prefix = "Buer_",
            .charset = "oO0",
            .length = 32,
        };

        PassNameObf GVNameObf{
            .prefix = "Buer_",
            .charset = "iIl1",
            .length = 32,
        };

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
