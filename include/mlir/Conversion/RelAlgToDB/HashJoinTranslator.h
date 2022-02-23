#ifndef MLIR_CONVERSION_RELALGTODB_HASHJOINTRANSLATOR_H
#define MLIR_CONVERSION_RELALGTODB_HASHJOINTRANSLATOR_H
#include "JoinTranslator.h"
#include "Translator.h"
#include "mlir/Conversion/RelAlgToDB/OrderedAttributes.h"
#include "mlir/Dialect/DB/IR/DBOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/RelAlg/Attributes.h"
#include "mlir/Dialect/RelAlg/IR/RelAlgOps.h"
#include "mlir/Dialect/util/UtilOps.h"
#include "mlir/IR/BlockAndValueMapping.h"
#include <tuple>

namespace mlir::relalg {
class HashJoinUtils {
   public:
   static std::tuple<mlir::relalg::Attributes, mlir::relalg::Attributes, std::vector<mlir::Type>, std::vector<Attributes>, std::vector<bool>> analyzeHJPred(mlir::Block* block, mlir::relalg::Attributes availableLeft, mlir::relalg::Attributes availableRight) {
      llvm::DenseMap<mlir::Value, mlir::relalg::Attributes> required;
      llvm::DenseSet<mlir::Value> pureAttribute;

      mlir::relalg::Attributes leftKeys, rightKeys;
      std::vector<Attributes> leftKeyAttributes;
      std::vector<bool> canSave;
      std::vector<mlir::Type> types;
      block->walk([&](mlir::Operation* op) {
         if (auto getAttr = mlir::dyn_cast_or_null<mlir::relalg::GetAttrOp>(op)) {
            required.insert({getAttr.getResult(), mlir::relalg::Attributes::from(getAttr.attr())});
            pureAttribute.insert(getAttr.getResult());
         } else if (auto cmpOp = mlir::dyn_cast_or_null<mlir::db::CmpOp>(op)) {
            if (cmpOp.predicate() == mlir::db::DBCmpPredicate::eq && isAndedResult(op)) {
               auto leftAttributes = required[cmpOp.left()];
               auto rightAttributes = required[cmpOp.right()];
               if (leftAttributes.isSubsetOf(availableLeft) && rightAttributes.isSubsetOf(availableRight)) {
                  leftKeys.insert(leftAttributes);
                  rightKeys.insert(rightAttributes);
                  leftKeyAttributes.push_back(leftAttributes);
                  canSave.push_back(pureAttribute.contains(cmpOp.left()));
                  types.push_back(cmpOp.left().getType());

               } else if (leftAttributes.isSubsetOf(availableRight) && rightAttributes.isSubsetOf(availableLeft)) {
                  leftKeys.insert(rightAttributes);
                  rightKeys.insert(leftAttributes);
                  leftKeyAttributes.push_back(rightAttributes);
                  canSave.push_back(pureAttribute.contains(cmpOp.right()));
                  types.push_back(cmpOp.right().getType());
               }
            }
         } else {
            mlir::relalg::Attributes attributes;
            for (auto operand : op->getOperands()) {
               if (required.count(operand)) {
                  attributes.insert(required[operand]);
               }
            }
            for (auto result : op->getResults()) {
               required.insert({result, attributes});
            }
         }
      });
      return {leftKeys, rightKeys, types, leftKeyAttributes, canSave};
   }

   static bool isAndedResult(mlir::Operation* op, bool first = true) {
      if (mlir::isa<mlir::relalg::ReturnOp>(op)) {
         return true;
      }
      if (mlir::isa<mlir::db::AndOp>(op) || first) {
         for (auto* user : op->getUsers()) {
            if (!isAndedResult(user, false)) return false;
         }
         return true;
      } else {
         return false;
      }
   }
   static std::vector<mlir::Value> inlineKeys(mlir::Block* block, mlir::relalg::Attributes keyAttributes, mlir::Block* newBlock, mlir::Block::iterator insertionPoint, mlir::relalg::TranslatorContext& context) {
      llvm::DenseMap<mlir::Value, mlir::relalg::Attributes> required;
      mlir::BlockAndValueMapping mapping;
      std::vector<mlir::Value> keys;
      block->walk([&](mlir::Operation* op) {
         if (auto getAttr = mlir::dyn_cast_or_null<mlir::relalg::GetAttrOp>(op)) {
            required.insert({getAttr.getResult(), mlir::relalg::Attributes::from(getAttr.attr())});
            if (keyAttributes.intersects(mlir::relalg::Attributes::from(getAttr.attr()))) {
               mapping.map(getAttr.getResult(), context.getValueForAttribute(&getAttr.attr().getRelationalAttribute()));
            }
         } else if (auto cmpOp = mlir::dyn_cast_or_null<mlir::db::CmpOp>(op)) {
            if (cmpOp.predicate() == mlir::db::DBCmpPredicate::eq && isAndedResult(op)) {
               auto leftAttributes = required[cmpOp.left()];
               auto rightAttributes = required[cmpOp.right()];
               mlir::Value keyVal;
               if (leftAttributes.isSubsetOf(keyAttributes)) {
                  keyVal = cmpOp.left();
               } else if (rightAttributes.isSubsetOf(keyAttributes)) {
                  keyVal = cmpOp.right();
               }
               if (keyVal) {
                  if (!mapping.contains(keyVal)) {
                     //todo: remove nasty hack:
                     mlir::OpBuilder builder(cmpOp->getContext());
                     builder.setInsertionPoint(newBlock, insertionPoint);
                     auto helperOp = builder.create<arith::ConstantOp>(cmpOp.getLoc(), builder.getIndexAttr(0));

                     mlir::relalg::detail::inlineOpIntoBlock(keyVal.getDefiningOp(), keyVal.getDefiningOp()->getParentOp(), newBlock->getParentOp(), newBlock, mapping, helperOp);
                     helperOp->remove();
                     helperOp->destroy();
                  }
                  keys.push_back(mapping.lookupOrNull(keyVal));
               }
            }
         } else {
            mlir::relalg::Attributes attributes;
            for (auto operand : op->getOperands()) {
               if (required.count(operand)) {
                  attributes.insert(required[operand]);
               }
            }
            for (auto result : op->getResults()) {
               required.insert({result, attributes});
            }
         }
      });
      return keys;
   }
};

class HashJoinTranslator : public mlir::relalg::JoinTranslator {
   public:
   mlir::Location loc;
   mlir::relalg::Attributes leftKeys, rightKeys;
   mlir::relalg::OrderedAttributes orderedKeys;
   mlir::relalg::OrderedAttributes orderedValues;
   mlir::TupleType keyTupleType, valTupleType, entryType;
   size_t builderId;
   mlir::relalg::PipelineDependency joinHt;

   HashJoinTranslator(std::shared_ptr<JoinImpl> impl) : JoinTranslator(impl), loc(joinOp.getLoc()) {}

   public:
   virtual void setInfo(mlir::relalg::Translator* consumer, mlir::relalg::Attributes requiredAttributes) override;
   virtual void produce(mlir::relalg::TranslatorContext& context, mlir::OpBuilder& builder) override;

   void unpackValues(TranslatorContext::AttributeResolverScope& scope, OpBuilder& builder, Value packed, TranslatorContext& context, Value& marker);
   void unpackKeys(TranslatorContext::AttributeResolverScope& scope, OpBuilder& builder, Value packed, TranslatorContext& context);

   virtual void scanHT(TranslatorContext& context, mlir::OpBuilder& builder) override;
   virtual void consume(mlir::relalg::Translator* child, mlir::OpBuilder& builder, mlir::relalg::TranslatorContext& context) override;

   virtual ~HashJoinTranslator() {}
};
} // end namespace mlir::relalg

#endif // MLIR_CONVERSION_RELALGTODB_HASHJOINTRANSLATOR_H
