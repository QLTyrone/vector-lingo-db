
#include "mlir/Dialect/DB/IR/DBOps.h"
#include "mlir/Dialect/RelAlg/IR/RelAlgOps.h"

#include "mlir/Dialect/RelAlg/Passes.h"
#include "mlir/IR/BlockAndValueMapping.h"

#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include <llvm/ADT/TypeSwitch.h>
#include <iostream>
#include <list>
#include <queue>
#include <unordered_set>
#include <mlir/Dialect/RelAlg/IR/RelAlgDialect.h>

namespace {

    class Pushdown : public mlir::PassWrapper<Pushdown, mlir::FunctionPass> {
        using attribute_set = llvm::SmallPtrSet<mlir::relalg::RelationalAttribute *, 8>;

        bool intersects(const attribute_set &a, const attribute_set &b) {
            for (auto x : a) {
                if (b.contains(x)) {
                    return true;
                }
            }
            return false;
        }

        bool subset(const attribute_set &a, const attribute_set &b) {
            for (auto x : a) {
                if (!b.contains(x)) {
                    return false;
                }
            }
            return true;
        }

        void print(const attribute_set &a) {
            auto &attributeManager = getContext().getLoadedDialect<mlir::relalg::RelAlgDialect>()->getRelationalAttributeManager();
            for (auto x:a) {
                auto[scope, name]=attributeManager.getName(x);
                llvm::dbgs() << x << "(" << scope << "," << name << "),";
            }

        }

        Operator pushdown(Operator topush, Operator curr) {

            attribute_set used_attributes = topush.getUsedAttributes();
            auto res = ::llvm::TypeSwitch<mlir::Operation *, Operator>(curr.getOperation())
                    .Case<mlir::relalg::CrossProductOp>([&](Operator cp) {
                        auto children = cp.getChildren();
                        if (subset(used_attributes, children[0].getAvailableAttributes())) {
                            topush->moveBefore(cp.getOperation());
                            children[0] = pushdown(topush, children[0]);
                            cp.setChildren(children);
                            return cp;
                        } else if (subset(used_attributes, children[1].getAvailableAttributes())) {
                            topush->moveBefore(cp.getOperation());
                            children[1] = pushdown(topush, children[1]);
                            cp.setChildren(children);
                            return cp;
                        } else {
                            topush.setChildren({curr});
                            return topush;
                        }
                    })
                    .Case<Join>([&](Join join) {
                        Operator opjoin = mlir::dyn_cast_or_null<Operator>(join.getOperation());

                        auto left = mlir::dyn_cast_or_null<Operator>(join.leftChild());
                        auto right = mlir::dyn_cast_or_null<Operator>(join.rightChild());
                        if (!mlir::isa<mlir::relalg::InnerJoinOp>(join.getOperation())) {
                            mlir::relalg::JoinDirection joinDirection = mlir::relalg::symbolizeJoinDirection(
                                    join->getAttr(
                                            "join_direction").dyn_cast_or_null<mlir::IntegerAttr>().getInt()).getValue();
                            switch (joinDirection) {
                                case mlir::relalg::JoinDirection::left:
                                    if (subset(used_attributes, left.getAvailableAttributes())) {
                                        topush->moveBefore(opjoin.getOperation());
                                        left = pushdown(topush, left);
                                        opjoin.setChildren({left, right});
                                        return opjoin;
                                    }
                                    [[fallthrough]];
                                case mlir::relalg::JoinDirection::right:
                                    if (subset(used_attributes, right.getAvailableAttributes())) {
                                        topush->moveBefore(opjoin.getOperation());
                                        right = pushdown(topush, right);
                                        opjoin.setChildren({left, right});
                                        return opjoin;
                                    }
                                    [[fallthrough]];
                                default:
                                    topush.setChildren({curr});
                                    return topush;
                            }
                        } else {
                            auto children = opjoin.getChildren();
                            if (subset(used_attributes, children[0].getAvailableAttributes())) {
                                topush->moveBefore(opjoin.getOperation());
                                children[0] = pushdown(topush, children[0]);
                                opjoin.setChildren(children);
                                return opjoin;
                            } else if (subset(used_attributes, children[1].getAvailableAttributes())) {
                                topush->moveBefore(opjoin.getOperation());
                                children[1] = pushdown(topush, children[1]);
                                opjoin.setChildren(children);
                                return opjoin;
                            } else {
                                topush.setChildren({curr});
                                return topush;
                            }
                        }
                    })
                    .Case<mlir::relalg::SelectionOp>([&](Operator sel) {
                        topush->moveBefore(sel.getOperation());
                        sel.setChildren({pushdown(topush, sel.getChildren()[0])});
                        return sel;
                    })
                    .Default([&](Operator others) {
                        topush.setChildren({others});
                        return topush;
                    });
            return res;
        }

        void runOnFunction() override {
            using namespace mlir;
            auto rel_type = relalg::RelationType::get(&getContext());

            auto &attributeManager = getContext().getLoadedDialect<mlir::relalg::RelAlgDialect>()->getRelationalAttributeManager();
            getFunction()->walk([&](mlir::relalg::SelectionOp sel) {
                SmallPtrSet<mlir::Operation *, 4> users;
                for (auto u:sel->getUsers()) {
                    users.insert(u);
                }
                Operator pushed_down = pushdown(sel, sel.getChildren()[0]);
                if (sel.getOperation() != pushed_down.getOperation()) {
                    //sel.replaceAllUsesWith(pushed_down.getOperation());
                    sel.getResult().replaceUsesWithIf(pushed_down->getResult(0), [&](mlir::OpOperand &operand) {
                        return users.contains(operand.getOwner());
                    });
                }


                //return WalkResult::interrupt();

            });
        }
    };
} // end anonymous namespace

namespace mlir {
    namespace relalg {
        std::unique_ptr<Pass> createPushdownPass() { return std::make_unique<Pushdown>(); }
    } // end namespace relalg
} // end namespace mlir