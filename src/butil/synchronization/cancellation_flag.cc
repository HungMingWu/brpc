// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "butil/synchronization/cancellation_flag.h"

#include "butil/logging.h"

namespace butil {

void CancellationFlag::Set() {
#if !defined(NDEBUG)
  DCHECK_EQ(set_on_, PlatformThread::CurrentId());
#endif
  flag_.store(1, std::memory_order_release);
}

bool CancellationFlag::IsSet() const {
  return flag_.load(std::memory_order_acquire) != 0;
}

}  // namespace butil
