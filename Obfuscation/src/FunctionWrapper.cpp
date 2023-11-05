/*
 *  LLVM FunctionWrapper Pass
    Copyright (C) 2017 Zhang(https://github.com/Naville/)
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// Last imported from: https://github.com/61bcdefg/Hikari-LLVM15-Core/blob/be20ec074511b74ced5e8f79892abc90d1a376a8/FunctionWrapper.cpp
#include "FunctionWrapper.h"

static cl::opt<uint32_t>
    ProbRate(
        "fw_prob",
        cl::desc("Choose the probability [%] For Each CallSite To Be "
                 "Obfuscated By FunctionWrapper"
        ),
        cl::value_desc("Probability Rate"), cl::init(30), cl::Optional
);
static uint32_t ProbRateTemp = 30;

static cl::opt<uint32_t> ObfTimes(
    "fw_times",
    cl::desc(
        "Choose how many time the FunctionWrapper pass loop on a CallSite"
    ),
    cl::value_desc("Number of Times"), cl::init(2), cl::Optional
);

PreservedAnalyses FunctionWrapperPass::run(Module &M, ModuleAnalysisManager& AM) {
    vector<CallSite *> callsites;
    for (Function &F : M) {
      if (toObfuscate(flag, &F, "fw")) {
        errs() << "Running FunctionWrapper On " << F.getName() << "\n";
        if (!toObfuscateUint32Option(&F, "fw_prob", &ProbRateTemp))
          ProbRateTemp = ProbRate;
        if (ProbRateTemp > 100) {
          errs() << "FunctionWrapper application CallSite percentage "
            "-fw_prob=x must be 0 < x <= 100";
          return PreservedAnalyses::all();
        }
        for (Instruction &Inst : instructions(F))
          if ((isa<CallInst>(&Inst) || isa<InvokeInst>(&Inst)))
            if (cryptoutils->get_range(100) <= ProbRateTemp)
              callsites.emplace_back(new CallSite(&Inst));
      }
    } 

    for (CallSite *CS : callsites){ // 嵌套混淆发生在全局 而不是针对某个函数
      for (uint32_t i = 0; i < ObfTimes && CS != nullptr; i++){
        CS = HandleCallSite(CS);
      }
    }
    return PreservedAnalyses::all();
}

CallSite* FunctionWrapperPass::HandleCallSite(CallSite *CS) {
    Value *calledFunction = CS->getCalledFunction();
    if (calledFunction == nullptr)
      calledFunction = CS->getCalledValue()->stripPointerCasts();
    // Filter out IndirectCalls that depends on the context
    // Otherwise It'll be blantantly troublesome since you can't reference an
    // Instruction outside its BB  Too much trouble for a hobby project
    // To be precise, we only keep CS that refers to a non-intrinsic function
    // either directly or through casting
    if (calledFunction == nullptr ||
        (!isa<ConstantExpr>(calledFunction) &&
         !isa<Function>(calledFunction)) ||
        CS->getIntrinsicID() != Intrinsic::not_intrinsic)
      return nullptr;
    std::vector<unsigned int> byvalArgNums;
    if (Function *tmp = dyn_cast<Function>(calledFunction)) {
      if (tmp->getName().startswith("clang.")) {
        // Clang Intrinsic
        return nullptr;
      }
      for (Argument &arg : tmp->args()) {
        if (arg.hasStructRetAttr() || arg.hasSwiftSelfAttr()) {
          return nullptr;
        }
        if (arg.hasByValAttr())
          byvalArgNums.emplace_back(arg.getArgNo());
      }
    }
    // Create a new function which in turn calls the actual function
    std::vector<Type *> types;
    for (unsigned int i = 0; i < CS->getNumArgOperands(); i++)
      types.emplace_back(CS->getArgOperand(i)->getType());
    FunctionType *ft =
        FunctionType::get(CS->getType(), ArrayRef<Type *>(types), false);
    Function *func =
        Function::Create(ft, GlobalValue::LinkageTypes::InternalLinkage,
                         "HikariFunctionWrapper", CS->getParent()->getModule());
    func->setCallingConv(CS->getCallingConv());
    // Trolling was all fun and shit so old implementation forced this symbol to
    // exist in all objects
    appendToCompilerUsed(*func->getParent(), {func});
    BasicBlock *BB = BasicBlock::Create(func->getContext(), "", func);
    std::vector<Value *> params;
    if (byvalArgNums.empty())
      for (Argument &arg : func->args())
        params.emplace_back(&arg);
    else
      for (Argument &arg : func->args()) {
        if (std::find(byvalArgNums.begin(), byvalArgNums.end(),
                      arg.getArgNo()) != byvalArgNums.end()) {
          params.emplace_back(&arg);
        } else {
          AllocaInst *AI = nullptr;
          if (!BB->empty()) {
            BasicBlock::iterator InsertPoint = BB->begin();
            while (isa<AllocaInst>(InsertPoint))
              ++InsertPoint;
            AI = new AllocaInst(arg.getType(), 0, "", &*InsertPoint);
          } else
            AI = new AllocaInst(arg.getType(), 0, "", BB);
          new StoreInst(&arg, AI, BB);
          LoadInst *LI = new LoadInst(AI->getAllocatedType(), AI, "", BB);
          params.emplace_back(LI);
        }
      }
    Value *retval = CallInst::Create(
        CS->getFunctionType(),
        ConstantExpr::getBitCast(cast<Function>(calledFunction),
                                 CS->getCalledValue()->getType()),
#if LLVM_VERSION_MAJOR >= 16
        ArrayRef<Value *>(params), std::nullopt, "", BB);
#else
        ArrayRef<Value *>(params), None, "", BB);
#endif
    if (ft->getReturnType()->isVoidTy()) {
      ReturnInst::Create(BB->getContext(), BB);
    } else {
      ReturnInst::Create(BB->getContext(), retval, BB);
    }
    CS->setCalledFunction(func);
    CS->mutateFunctionType(ft);
    Instruction *Inst = CS->getInstruction();
    delete CS;
    return new CallSite(Inst);
}

/**
 * @brief 便于调用函数包装器
 *
 * @param flag
 * @return FunctionPass*
 */
FunctionWrapperPass *llvm::createFunctionWrapper(bool flag){
    return new FunctionWrapperPass(flag);
}
