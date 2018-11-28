// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "butil/threading/thread_collision_warner.h"

#include "butil/logging.h"
#include "butil/threading/platform_thread.h"

namespace butil {

void DCheckAsserter::warn() {
  NOTREACHED() << "Thread Collision";
}

static int32_t CurrentThread() {
  const PlatformThreadId current_thread_id = PlatformThread::CurrentId();
  // We need to get the thread id into an atomic data type. This might be a
  // truncating conversion, but any loss-of-information just increases the
  // chance of a fault negative, not a false positive.
  const int32_t atomic_thread_id =
      static_cast<int32_t>(current_thread_id);

  return atomic_thread_id;
}

void ThreadCollisionWarner::EnterSelf() {
  // If the active thread is 0 then I'll write the current thread ID
  // if two or more threads arrive here only one will succeed to
  // write on valid_thread_id_ the current thread ID.
  int32_t current_thread_id = CurrentThread();

  int32_t previous_value = 0;
  valid_thread_id_.compare_exchange_strong(previous_value, current_thread_id,
							std::memory_order_relaxed,
							std::memory_order_relaxed);
  if (previous_value != 0 && previous_value != current_thread_id) {
    // gotcha! a thread is trying to use the same class and that is
    // not current thread.
    asserter_->warn();
  }

  counter_.fetch_add(1, std::memory_order_relaxed);
}

void ThreadCollisionWarner::Enter() {
  int32_t current_thread_id = CurrentThread();

  int32_t previous_value = 0;
  if (valid_thread_id_.compare_exchange_strong(
                                       previous_value,
                                       current_thread_id,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed)) {
    // gotcha! another thread is trying to use the same class.
    asserter_->warn();
  }

  counter_.fetch_add(1, std::memory_order_relaxed);
}

void ThreadCollisionWarner::Leave() {
  if (counter_.fetch_sub(1, std::memory_order_acq_rel) == 0) {
    valid_thread_id_.store(0, std::memory_order_relaxed);
  }
}

}  // namespace butil
