//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_hash_table.cpp
//
// Identification: src/container/hash/extendible_hash_table.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"
#include "container/hash/extendible_hash_table.h"

namespace bustub {

template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_TYPE::ExtendibleHashTable(const std::string &name, BufferPoolManager *buffer_pool_manager,
                                     const KeyComparator &comparator, HashFunction<KeyType> hash_fn)
    : buffer_pool_manager_(buffer_pool_manager), comparator_(comparator), hash_fn_(std::move(hash_fn)) {
  //  implement me!
  table_latch_.WLock();
  auto dir_page = reinterpret_cast<HashTableDirectoryPage *>(buffer_pool_manager_->NewPage(&directory_page_id_));
  page_id_t bucket_page_id;
  auto page = buffer_pool_manager_->NewPage(&bucket_page_id);
  page->WLatch();

  dir_page->SetLocalDepth(0, 0);
  dir_page->SetBucketPageId(0, bucket_page_id);

  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, true));
  page->WUnlatch();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, true));
  table_latch_.WUnlock();
}

/*****************************************************************************
 * HELPERS
 *****************************************************************************/
/**
 * Hash - simple helper to downcast MurmurHash's 64-bit hash to 32-bit
 * for extendible hashing.
 *
 * @param key the key to hash
 * @return the downcasted 32-bit hash
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Hash(KeyType key) -> uint32_t {
  return static_cast<uint32_t>(hash_fn_.GetHash(key));
}

template <typename KeyType, typename ValueType, typename KeyComparator>
inline auto HASH_TABLE_TYPE::KeyToDirectoryIndex(KeyType key, HashTableDirectoryPage *dir_page) -> uint32_t {
  return Hash(key) & dir_page->GetGlobalDepthMask();
}

template <typename KeyType, typename ValueType, typename KeyComparator>
inline auto HASH_TABLE_TYPE::KeyToPageId(KeyType key, HashTableDirectoryPage *dir_page) -> uint32_t {
  auto bucket_idx = KeyToDirectoryIndex(key, dir_page);
  return dir_page->GetBucketPageId(bucket_idx);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::FetchDirectoryPage() -> HashTableDirectoryPage * {
  return reinterpret_cast<HashTableDirectoryPage *>(buffer_pool_manager_->FetchPage(directory_page_id_));
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::FetchBucketPage(page_id_t bucket_page_id) -> HASH_TABLE_BUCKET_TYPE * {
  return reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(buffer_pool_manager_->FetchPage(bucket_page_id));
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::GetValue(Transaction *transaction, const KeyType &key, std::vector<ValueType> *result) -> bool {
  table_latch_.RLock();
  auto dir_page = FetchDirectoryPage();
  auto bucket_page_id = KeyToPageId(key, dir_page);
  auto bucket_page = FetchBucketPage(bucket_page_id);
  auto page = reinterpret_cast<Page *>(bucket_page);
  page->RLatch();

  auto success = bucket_page->GetValue(key, comparator_, result);

  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false));
  page->RUnlatch();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false));
  table_latch_.RUnlock();
  return success;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Insert(Transaction *transaction, const KeyType &key, const ValueType &value) -> bool {
  table_latch_.RLock();
  auto dir_page = FetchDirectoryPage();
  auto bucket_page_id = KeyToPageId(key, dir_page);
  auto bucket_page = FetchBucketPage(bucket_page_id);
  auto page = reinterpret_cast<Page *>(bucket_page);
  page->WLatch();

  auto success = bucket_page->Insert(key, value, comparator_);
  auto need_split = !success && bucket_page->IsFull();

  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, success));
  page->WUnlatch();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false));
  table_latch_.RUnlock();

  if (need_split) {
    return SplitInsert(transaction, key, value);
  }
  return success;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::SplitInsert(Transaction *transaction, const KeyType &key, const ValueType &value) -> bool {
  table_latch_.WLock();
  auto dir_page = FetchDirectoryPage();
  auto bucket_idx = KeyToDirectoryIndex(key, dir_page);
  auto bucket_page_id = dir_page->GetBucketPageId(bucket_idx);
  auto bucket_page = FetchBucketPage(bucket_page_id);
  auto page = reinterpret_cast<Page *>(bucket_page);
  page->WLatch();

  if (bucket_page->IsFull()) {
    // 获取原桶的所有值
    auto vals = bucket_page->StealKVs();
    // 按需将表项扩展一倍
    if (dir_page->GetLocalDepth(bucket_idx) == dir_page->GetGlobalDepth()) {
      dir_page->IncrGlobalDepth();
      bucket_idx = KeyToDirectoryIndex(key, dir_page);
    }
    // 创建新桶
    page_id_t new_bucket_page_id;
    auto new_page = buffer_pool_manager_->NewPage(&new_bucket_page_id);
    // 把一半指向原桶的表项指向新桶
    auto common_bits = bucket_idx & dir_page->GetLocalDepthMask(bucket_idx);
    auto dir_size = dir_page->Size();
    auto higher_bit = 1 << dir_page->GetLocalDepth(bucket_idx);
    for (auto i = common_bits; i < dir_size; i += higher_bit) {
      if ((i & higher_bit) != (bucket_idx & higher_bit)) {  // split out
        dir_page->SetBucketPageId(i, new_bucket_page_id);
      }
      dir_page->IncrLocalDepth(i);
    }
    // 迁移旧桶值
    auto new_local_depth_mask = dir_page->GetLocalDepthMask(bucket_idx);
    auto new_bucket_page = reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(new_page);
    HASH_TABLE_BUCKET_TYPE *b = nullptr;
    new_page->WLatch();

    for (const auto &e : vals) {
      auto idx = Hash(e.first) & new_local_depth_mask;
      if (idx == bucket_idx) {
        b = bucket_page;
      } else {
        b = new_bucket_page;
      }
      assert(b->Insert(e.first, e.second, comparator_));
    }

    assert(buffer_pool_manager_->UnpinPage(new_bucket_page_id, true));
    new_page->WUnlatch();
  }

  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, true));
  page->WUnlatch();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, true));
  table_latch_.WUnlock();

  return Insert(transaction, key, value);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::Remove(Transaction *transaction, const KeyType &key, const ValueType &value) -> bool {
  table_latch_.RLock();
  auto dir_page = FetchDirectoryPage();
  auto bucket_idx = KeyToDirectoryIndex(key, dir_page);
  auto bucket_page_id = dir_page->GetBucketPageId(bucket_idx);
  auto bucket_page = FetchBucketPage(bucket_page_id);
  auto page = reinterpret_cast<Page *>(bucket_page);
  page->WLatch();

  auto success = bucket_page->Remove(key, value, comparator_);
  auto need_merge = NeedMerge(bucket_idx, bucket_page, dir_page);

  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, success));
  page->WUnlatch();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false));
  table_latch_.RUnlock();

  if (need_merge) {
    Merge(transaction, key, value);
  }
  return success;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::NeedMerge(uint32_t bucket_idx, HASH_TABLE_BUCKET_TYPE *bucket_page,
                                HashTableDirectoryPage *dir_page) {
  if (bucket_page->IsEmpty()) {
    auto split_image_idx = dir_page->GetSplitImageIndex(bucket_idx);
    auto local_depth = dir_page->GetLocalDepth(bucket_idx);
    if (local_depth > 0 && dir_page->GetLocalDepth(split_image_idx) == local_depth) {
      return true;
    }
  }
  return false;
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::Merge(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.WLock();
  auto dir_page = FetchDirectoryPage();

  while (true) {
    auto bucket_idx = KeyToDirectoryIndex(key, dir_page);
    auto bucket_page_id = dir_page->GetBucketPageId(bucket_idx);
    auto bucket_page = FetchBucketPage(bucket_page_id);
    auto page = reinterpret_cast<Page *>(bucket_page);

    page->RLatch();
    auto need_merge = NeedMerge(bucket_idx, bucket_page, dir_page);
    assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false));
    page->RUnlatch();
    if (!need_merge) {
      break;
    }

    auto split_image_idx = dir_page->GetSplitImageIndex(bucket_idx);
    auto split_image_page_id = dir_page->GetBucketPageId(split_image_idx);
    // 所有指向原bucket和split_image表项，指向split_image、local_depth--；
    auto local_depth_mask = dir_page->GetLocalDepthMask(bucket_idx);
    uint32_t idx_start = bucket_idx & local_depth_mask & split_image_idx;
    uint32_t idx_size = dir_page->Size();
    uint32_t idx_diff = dir_page->GetLocalHighBit(bucket_idx);
    for (auto i = idx_start; i < idx_size; i += idx_diff) {
      dir_page->SetBucketPageId(i, split_image_page_id);
      dir_page->DecrLocalDepth(i);
    }

    if (dir_page->CanShrink()) {
      dir_page->DecrGlobalDepth();
    }
  }

  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, true));
  table_latch_.WUnlock();
}

/*****************************************************************************
 * GETGLOBALDEPTH - DO NOT TOUCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
auto HASH_TABLE_TYPE::GetGlobalDepth() -> uint32_t {
  table_latch_.RLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  uint32_t global_depth = dir_page->GetGlobalDepth();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr));
  table_latch_.RUnlock();
  return global_depth;
}

/*****************************************************************************
 * VERIFY INTEGRITY - DO NOT TOUCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::VerifyIntegrity() {
  table_latch_.RLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  dir_page->VerifyIntegrity();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr));
  table_latch_.RUnlock();
}

/*****************************************************************************
 * TEMPLATE DEFINITIONS - DO NOT TOUCH
 *****************************************************************************/
template class ExtendibleHashTable<int, int, IntComparator>;

template class ExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
