//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// distinct_executor.cpp
//
// Identification: src/execution/distinct_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/distinct_executor.h"
#include "execution/expressions/abstract_expression.h"

namespace bustub {

DistinctExecutor::DistinctExecutor(ExecutorContext *exec_ctx, const DistinctPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DistinctExecutor::Init() {
  child_executor_->Init();

  Tuple tuple;
  RID rid;
  while (child_executor_->Next(&tuple, &rid)) {
    // 提取output_schema指定字段
    auto count = GetOutputSchema()->GetColumnCount();
    DistinctKey key;
    key.vals_.reserve(count);
    for (size_t i = 0; i < count; i++) {
      auto val = tuple.GetValue(GetOutputSchema(), i);
      key.vals_.emplace_back(val);
    }
    if (ht_.count(key) == 0) {
      ht_[key] = tuple;
    }
  }

  iter_ = begin(ht_);
}

auto DistinctExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (iter_ == end(ht_)) {
    return false;
  }
  *tuple = iter_->second;
  *rid = tuple->GetRid();
  iter_++;
  return true;
}
}  // namespace bustub
