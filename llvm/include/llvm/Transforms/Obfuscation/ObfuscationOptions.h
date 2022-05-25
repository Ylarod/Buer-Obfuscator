#ifndef OBFUSCATION_OBFUSCATIONOPTIONS_H
#define OBFUSCATION_OBFUSCATIONOPTIONS_H

#include <set>
#include <llvm/Support/YAMLParser.h>

namespace llvm {

struct ObfuscationOptions {
  explicit ObfuscationOptions(const Twine &FileName);
  explicit ObfuscationOptions() = default;
  bool skipFunction(const Twine &FName);
  void dump() const;

  bool EnableIndirectBr = false;
  bool EnableIndirectCall = false;
  bool EnableIndirectGV = false;
  bool EnableCFF = false;
  bool EnableCSE = false;
  bool hasFilter = false;

private:
  void handleRoot(yaml::Node *n);
  bool parseOptions(const Twine &FileName);
  std::set<std::string> FunctionFilter;
};

}

#endif
