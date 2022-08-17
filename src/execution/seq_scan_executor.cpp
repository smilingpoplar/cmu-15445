//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void SeqScanExecutor::Init() {
  table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
  table_iter_ = std::make_unique<TableIterator>(table_info_->table_->Begin(exec_ctx_->GetTransaction()));
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (*table_iter_ != table_info_->table_->End()) {
    auto tup = **table_iter_;
    (*table_iter_)++;
    auto predicate = plan_->GetPredicate();
    if (predicate != nullptr && !predicate->Evaluate(&tup, &table_info_->schema_).GetAs<bool>()) {
      continue;
    }
    // 提取output_schema指定字段
    std::vector<Value> vals;
    for (auto &col : GetOutputSchema()->GetColumns()) {
      vals.emplace_back(col.GetExpr()->Evaluate(&tup, &table_info_->schema_));
    }
    *tuple = Tuple(vals, GetOutputSchema());
    *rid = tup.GetRid();
    return true;
  }

  return false;
}

}  // namespace bustub
