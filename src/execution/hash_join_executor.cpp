//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include "execution/expressions/abstract_expression.h"

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_executor,
                                   std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)) {}

void HashJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();

  // 用left_executor_建hash表
  Tuple left_tuple;
  RID left_rid;
  while (left_executor_->Next(&left_tuple, &left_rid)) {
    HashJoinKey key;
    key.val_ = plan_->LeftJoinKeyExpression()->Evaluate(&left_tuple, left_executor_->GetOutputSchema());
    ht_[key].emplace_back(left_tuple);  // 支持key值重复
  }
}

auto HashJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (!result_.empty()) {
    *tuple = result_.front();
    result_.pop_front();
    *rid = tuple->GetRid();
    return true;
  }

  Tuple right_tuple;
  RID right_rid;
  if (!right_executor_->Next(&right_tuple, &right_rid)) {
    return false;
  }

  HashJoinKey key;
  key.val_ = plan_->RightJoinKeyExpression()->Evaluate(&right_tuple, right_executor_->GetOutputSchema());
  if (ht_.count(key) > 0) {
    for (auto &left_tuple : ht_[key]) {
      // 提取output_schema指定字段
      std::vector<Value> vals;
      for (auto &col : GetOutputSchema()->GetColumns()) {
        vals.emplace_back(col.GetExpr()->EvaluateJoin(&left_tuple, left_executor_->GetOutputSchema(), &right_tuple,
                                                      right_executor_->GetOutputSchema()));
      }
      result_.emplace_back(Tuple(vals, GetOutputSchema()));
    }
  }

  return Next(tuple, rid);
}

}  // namespace bustub
