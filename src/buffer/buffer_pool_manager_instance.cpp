//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/macros.h"

namespace bustub {

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager)
    : BufferPoolManagerInstance(pool_size, 1, 0, disk_manager, log_manager) {}

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, uint32_t num_instances, uint32_t instance_index,
                                                     DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size),
      num_instances_(num_instances),
      instance_index_(instance_index),
      next_page_id_(instance_index),
      disk_manager_(disk_manager),
      log_manager_(log_manager) {
  BUSTUB_ASSERT(num_instances > 0, "If BPI is not part of a pool, then the pool size should just be 1");
  BUSTUB_ASSERT(
      instance_index < num_instances,
      "BPI index cannot be greater than the number of BPIs in the pool. In non-parallel case, index should just be 1.");
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new LRUReplacer(pool_size);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete replacer_;
}

auto BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) -> bool {
  // Make sure you call DiskManager::WritePage!
  std::scoped_lock guard(latch_);
  if (!FindPg(page_id)) {
    return false;
  }
  auto frame_id = page_table_[page_id];
  auto page = &pages_[frame_id];
  FlushPg(page);
  return true;
}

bool BufferPoolManagerInstance::FindPg(page_id_t page_id) { return page_table_.count(page_id) != 0; }

void BufferPoolManagerInstance::FlushPg(Page *page) {
  if (page->IsDirty()) {
    disk_manager_->WritePage(page->GetPageId(), page->GetData());
    page->is_dirty_ = false;
  }
}

void BufferPoolManagerInstance::FlushAllPgsImp() {
  // You can do it!
  std::scoped_lock guard(latch_);
  for (const auto &e : page_table_) {
    auto page = &pages_[e.second];
    FlushPg(page);
  }
}

auto BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) -> Page * {
  // 0.   Make sure you call AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  std::scoped_lock guard(latch_);
  if (AllPgsPinned()) {
    return nullptr;
  }

  frame_id_t frame_id;
  if (!GetFrameId(&frame_id)) {
    return nullptr;
  }

  auto new_page_id = AllocatePage();
  auto page = &pages_[frame_id];
  ResetPg(page, new_page_id, 1);
  replacer_->Pin(frame_id);
  page_table_[new_page_id] = frame_id;
  *page_id = new_page_id;
  return page;
}

bool BufferPoolManagerInstance::AllPgsPinned() {
  if (page_table_.size() < pool_size_) {
    return false;
  }

  return std::all_of(page_table_.begin(), page_table_.end(),
                     [&](const auto &e) { return pages_[e.second].GetPinCount() > 0; });
}

void BufferPoolManagerInstance::ResetPg(Page *page, page_id_t page_id, int pin_count) {
  page->page_id_ = page_id;
  page->pin_count_ = pin_count;
  page->is_dirty_ = false;
  page->ResetMemory();
}

auto BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) -> Page * {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  std::scoped_lock guard(latch_);
  if (FindPg(page_id)) {
    auto frame_id = page_table_[page_id];
    auto page = &pages_[frame_id];
    page->pin_count_++;
    replacer_->Pin(frame_id);
    return page;
  }
  frame_id_t frame_id;
  if (!GetFrameId(&frame_id)) {
    return nullptr;
  }

  page_table_[page_id] = frame_id;
  auto page = &pages_[frame_id];
  ResetPg(page, page_id, 1);
  replacer_->Pin(frame_id);
  disk_manager_->ReadPage(page_id, page->GetData());
  return page;
}

bool BufferPoolManagerInstance::GetFrameId(frame_id_t *frame_id) {
  if (!free_list_.empty()) {
    *frame_id = free_list_.front();
    free_list_.pop_front();
    return true;
  }
  if (!replacer_->Victim(frame_id)) {
    return false;
  }
  auto page = &pages_[*frame_id];
  FlushPg(page);
  page_table_.erase(page->GetPageId());
  return true;
}

auto BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) -> bool {
  // 0.   Make sure you call DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  std::scoped_lock guard(latch_);
  if (!FindPg(page_id)) {
    return true;
  }

  auto frame_id = page_table_[page_id];
  auto page = &pages_[frame_id];
  if (page->GetPinCount() > 0) {
    return false;
  }

  FlushPg(page);
  page_table_.erase(page_id);
  ResetPg(page);
  free_list_.emplace_back(frame_id);

  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) -> bool {
  std::scoped_lock guard(latch_);
  if (!FindPg(page_id)) {
    return false;
  }
  auto frame_id = page_table_[page_id];
  auto page = &pages_[frame_id];
  if (page->GetPinCount() <= 0) {
    return false;
  }
  if (is_dirty) {
    page->is_dirty_ = true;
  }
  page->pin_count_--;
  if (page->GetPinCount() == 0) {
    replacer_->Unpin(frame_id);
  }
  return true;
}

auto BufferPoolManagerInstance::AllocatePage() -> page_id_t {
  const page_id_t next_page_id = next_page_id_;
  next_page_id_ += num_instances_;
  ValidatePageId(next_page_id);
  return next_page_id;
}

void BufferPoolManagerInstance::ValidatePageId(const page_id_t page_id) const {
  assert(page_id % num_instances_ == instance_index_);  // allocated pages mod back to this BPI
}

}  // namespace bustub
