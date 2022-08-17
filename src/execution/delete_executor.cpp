//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  table_info_ = exec_ctx_->GetCatalog()->GetTable(plan_->TableOid());
}

void DeleteExecutor::Init() { child_executor_->Init(); }

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  Tuple tup;  // 待删除元组
  if (!child_executor_->Next(&tup, rid)) {
    return false;
  }
  // 更新数据库
  auto txn = exec_ctx_->GetTransaction();
  if (!table_info_->table_->MarkDelete(*rid, txn)) {
    return false;
  }
  // 更新索引
  auto index_infos = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
  for (auto index_info : index_infos) {
    auto key = tup.KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
    index_info->index_->DeleteEntry(key, *rid, txn);
  }

  return true;
}

}  // namespace bustub
