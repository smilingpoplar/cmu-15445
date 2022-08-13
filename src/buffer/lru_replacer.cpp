//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_replacer.h"

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) { num_pages_ = num_pages; }

LRUReplacer::~LRUReplacer() = default;

auto LRUReplacer::Victim(frame_id_t *frame_id) -> bool {
  std::scoped_lock guard(latch_);
  if (recent_.empty()) {
    return false;
  }
  *frame_id = GetVictim();
  return true;
}

frame_id_t LRUReplacer::GetVictim() {
  int frame_id = recent_.back();
  recent_.pop_back();
  m_.erase(frame_id);
  return frame_id;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  // frame被引用，从lru_replacer删除
  std::scoped_lock guard(latch_);
  if (m_.count(frame_id) == 0) {
    return;
  }
  recent_.erase(m_[frame_id]);
  m_.erase(frame_id);
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  // frame没被引用，加入lru_replacer等待被换出
  std::scoped_lock guard(latch_);
  if (m_.count(frame_id) != 0) {
    return;
  }
  if (static_cast<int>(recent_.size()) == num_pages_) {
    GetVictim();
  }
  recent_.push_front(frame_id);
  m_[frame_id] = recent_.begin();
}

auto LRUReplacer::Size() -> size_t {
  std::scoped_lock guard(latch_);
  return recent_.size();
}

}  // namespace bustub
