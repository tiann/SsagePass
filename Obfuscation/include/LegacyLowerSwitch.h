#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"

#include "Utils.h"

struct IntRange {
    int64_t Low, High;
};

namespace llvm {
    /// Replace all SwitchInst instructions with chained branch instructions.
    struct LegacyLowerSwitch: public PassInfoMixin<LegacyLowerSwitch> {
        public:
            struct CaseRange {
                ConstantInt *Low;
                ConstantInt *High;
                BasicBlock *BB;

                CaseRange(ConstantInt *low, ConstantInt *high, BasicBlock *bb)
                    : Low(low), High(high), BB(bb) {}
            };

            using CaseVector = std::vector<CaseRange>;
            using CaseItr = std::vector<CaseRange>::iterator;

            bool flag;
            LegacyLowerSwitch(bool flag){
                this->flag = flag;
            } // 携带flag的构造函数
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
            PreservedAnalyses runOnFunction(Function &F); // Pass实现函数

            /// Used for debugging purposes.
        private:
            void processSwitchInst(SwitchInst *SI,
                    SmallPtrSetImpl<BasicBlock *> &DeleteList);

            BasicBlock *switchConvert(CaseItr Begin, CaseItr End, ConstantInt *LowerBound,
                    ConstantInt *UpperBound, Value *Val,
                    BasicBlock *Predecessor, BasicBlock *OrigBlock,
                    BasicBlock *Default,
                    const std::vector<IntRange> &UnreachableRanges);
            BasicBlock *newLeafBlock(CaseRange &Leaf, Value *Val, BasicBlock *OrigBlock,
                    BasicBlock *Default);
            unsigned Clusterify(CaseVector &Cases, SwitchInst *SI);
    };

    /// The comparison function for sorting the switch case values in the vector.
    /// WARNING: Case ranges should be disjoint!
    struct CaseCmp {
        bool operator()(const LegacyLowerSwitch::CaseRange &C1,
                const LegacyLowerSwitch::CaseRange &C2) {
            const ConstantInt *CI1 = cast<const ConstantInt>(C1.Low);
            const ConstantInt *CI2 = cast<const ConstantInt>(C2.High);
            return CI1->getValue().slt(CI2->getValue());
        }
    };

    /// Used for debugging purposes.
    LLVM_ATTRIBUTE_USED
        static raw_ostream &operator<<(raw_ostream &O,
                const LegacyLowerSwitch::CaseVector &C) {
            O << "[";

            for (LegacyLowerSwitch::CaseVector::const_iterator B = C.begin(), E = C.end();
                    B != E;) {
                O << *B->Low << " -" << *B->High;
                if (++B != E)
                    O << ", ";
            }

            return O << "]";
        }
}
