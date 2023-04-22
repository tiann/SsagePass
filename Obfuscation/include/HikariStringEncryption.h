#ifndef LLVM_HIKARI_STRING_ENCRYPTION_H
#define LLVM_HIKARI_STRING_ENCRYPTION_H
// LLVM libs
#include "llvm/Transforms/Utils/GlobalStatus.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SHA1.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

// User libs
#include "CryptoUtils.h"
#include "Utils.h"
// System libs
#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace std;
namespace llvm {
    struct HikariStringEncryptionPass : public PassInfoMixin<HikariStringEncryptionPass> {
        public:
            bool flag;
            bool appleptrauth;
            bool opaquepointers;
            std::map<Function * /*Function*/, GlobalVariable * /*Decryption Status*/>
                encstatus;
            std::map<GlobalVariable *, std::pair<Constant *, GlobalVariable *>> mgv2keys;
            std::map<Constant *, std::vector<unsigned int>> unencryptedindex;
            std::vector<GlobalVariable *> genedgv;

            HikariStringEncryptionPass(bool flag){this->flag = flag;}
            PreservedAnalyses run(Module &M, ModuleAnalysisManager& AM);

        private:
            bool handleableGV(GlobalVariable *GV);
            void HandleFunction(Function *Func);

            GlobalVariable *ObjectiveCString(GlobalVariable *GV, std::string name,
                    GlobalVariable *newString,
                    ConstantStruct *CS);

            void HandleDecryptionBlock(
                    BasicBlock *B, BasicBlock *C,
                    std::map<GlobalVariable *, std::pair<Constant *, GlobalVariable *>>
                    &GV2Keys);
    };
    HikariStringEncryptionPass *createHikariStringEncryption(bool flag); // 创建字符串加密
}
#endif
