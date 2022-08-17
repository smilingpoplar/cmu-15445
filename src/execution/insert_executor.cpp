//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->TableOid());
}

void InsertExecutor::Init() {
  if (child_executor_) {
    child_executor_->Init();
  }
  raw_values_idx_ = 0;
}

auto InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  Tuple tup;  // 待插入元组
  if (plan_->IsRawInsert()) {
    if (raw_values_idx_ >= plan_->RawValues().size()) {
      return false;
    }
    tup = Tuple(plan_->RawValuesAt(raw_values_idx_), &table_info_->schema_);
    raw_values_idx_++;
  } else {
    RID tmp_rid;
    if (!child_executor_->Next(&tup, &tmp_rid)) {
      return false;
    }
  }
  // 插入数据库
  auto txn = exec_ctx_->GetTransaction();
  if (!table_info_->table_->InsertTuple(tup, rid, txn)) {
    return false;
  }
  // 更新索引
  auto index_infos = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
  for (auto index_info : index_infos) {
    auto key = tup.KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
    index_info->index_->InsertEntry(key, *rid, txn);
  }
  return true;
}

}  // namespace bustub
