//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)) {}

void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();
}

auto NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple right_tuple;
  RID right_rid;
  while (Advance(&right_tuple, &right_rid)) {
    auto predicate = plan_->Predicate();
    if (predicate == nullptr || predicate
                                    ->EvaluateJoin(&left_tuple_, left_executor_->GetOutputSchema(), &right_tuple,
                                                   right_executor_->GetOutputSchema())
                                    .GetAs<bool>()) {
      // 提取output_schema指定字段
      std::vector<Value> vals;
      for (auto &col : GetOutputSchema()->GetColumns()) {
        vals.emplace_back(col.GetExpr()->EvaluateJoin(&left_tuple_, left_executor_->GetOutputSchema(), &right_tuple,
                                                      right_executor_->GetOutputSchema()));
      }
      *tuple = Tuple(vals, GetOutputSchema());
      *rid = tuple->GetRid();
      return true;
    }
  }

  return false;
}

auto NestedLoopJoinExecutor::Advance(Tuple *tuple, RID *rid) -> bool {
  if (left_tuple_.GetLength() == 0) {
    RID left_rid;
    if (!left_executor_->Next(&left_tuple_, &left_rid)) {
      return false;
    }
  }

  if (!right_executor_->Next(tuple, rid)) {
    RID left_rid;
    if (!left_executor_->Next(&left_tuple_, &left_rid)) {
      return false;
    }
    right_executor_->Init();
    return right_executor_->Next(tuple, rid);
  }
  return true;
}

}  // namespace bustub
