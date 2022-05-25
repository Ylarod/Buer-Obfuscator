#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"
#include "llvm/Support/FileSystem.h"
#include <string>

using namespace llvm;

namespace llvm {

ObfuscationOptions::ObfuscationOptions(const Twine &FileName) {
  if (sys::fs::exists(FileName)) {
    parseOptions(FileName);
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
  if (FName.str().find("goron_") == std::string::npos) {
    return hasFilter && FunctionFilter.count(FName.str()) == 0;
  } else {
    return true;
  }
}

void ObfuscationOptions::handleRoot(yaml::Node *n) {
  if (!n)
    return;
  if (auto *mn = dyn_cast<yaml::MappingNode>(n)) {
    for (auto & i : *mn) {
      StringRef K = getNodeString(i.getKey());
      if (K == "IndirectBr") {
        EnableIndirectBr = static_cast<bool>(getIntVal(i.getValue()));
      } else if (K == "IndirectCall") {
        EnableIndirectCall = static_cast<bool>(getIntVal(i.getValue()));
      } else if (K == "IndirectGV") {
        EnableIndirectGV = static_cast<bool>(getIntVal(i.getValue()));
      } else if (K == "ControlFlowFlatten") {
        EnableCFF = static_cast<bool>(getIntVal(i.getValue()));
      } else if (K == "ConstantStringEncryption") {
        EnableCSE = static_cast<bool>(getIntVal(i.getValue()));
      } else if (K == "Filter") {
        hasFilter = true;
        FunctionFilter = getStringList(i.getValue());
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
  dbgs() << "EnableIndirectBr: " << EnableIndirectBr << "\n"
         << "EnableIndirectCall: " << EnableIndirectCall << "\n"
         << "EnableIndirectGV: " << EnableIndirectGV << "\n"
         << "EnableCFF: " << EnableCFF << "\n"
         << "hasFilter:" << hasFilter << "\n";
}

}
