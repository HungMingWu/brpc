// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <thread>
#include "butil/at_exit.h"
#include "butil/atomic_sequence_num.h"
#include "butil/lazy_instance.h"
#include <gtest/gtest.h>

namespace {

butil::StaticAtomicSequenceNumber constructed_seq_;
butil::StaticAtomicSequenceNumber destructed_seq_;

class ConstructAndDestructLogger {
 public:
  ConstructAndDestructLogger() {
    constructed_seq_.GetNext();
  }
  ~ConstructAndDestructLogger() {
    destructed_seq_.GetNext();
  }
};

class SlowConstructor {
 public:
  SlowConstructor() : some_int_(0) {
    // Sleep for 1 second to try to cause a race.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ++constructed;
    some_int_ = 12;
  }
  int some_int() const { return some_int_; }

  static int constructed;
 private:
  int some_int_;
};

int SlowConstructor::constructed = 0;

}  // namespace

static butil::LazyInstance<ConstructAndDestructLogger> lazy_logger;

TEST(LazyInstanceTest, Basic) {
  {
    butil::ShadowingAtExitManager shadow;

    EXPECT_EQ(0, constructed_seq_.GetNext());
    EXPECT_EQ(0, destructed_seq_.GetNext());

    lazy_logger.Get();
    EXPECT_EQ(2, constructed_seq_.GetNext());
    EXPECT_EQ(1, destructed_seq_.GetNext());

    lazy_logger.Pointer();
    EXPECT_EQ(3, constructed_seq_.GetNext());
    EXPECT_EQ(2, destructed_seq_.GetNext());
  }
  EXPECT_EQ(4, constructed_seq_.GetNext());
  EXPECT_EQ(4, destructed_seq_.GetNext());
}

static butil::LazyInstance<SlowConstructor> lazy_slow;

namespace {

// DeleteLogger is an object which sets a flag when it's destroyed.
// It accepts a bool* and sets the bool to true when the dtor runs.
class DeleteLogger {
 public:
  DeleteLogger() : deleted_(NULL) {}
  ~DeleteLogger() { *deleted_ = true; }

  void SetDeletedPtr(bool* deleted) {
    deleted_ = deleted;
  }

 private:
  bool* deleted_;
};

}  // anonymous namespace

TEST(LazyInstanceTest, LeakyLazyInstance) {
  // Check that using a plain LazyInstance causes the dtor to run
  // when the AtExitManager finishes.
  bool deleted1 = false;
  {
    butil::ShadowingAtExitManager shadow;
    static butil::LazyInstance<DeleteLogger> test;
    test.Get().SetDeletedPtr(&deleted1);
  }
  EXPECT_TRUE(deleted1);

  // Check that using a *leaky* LazyInstance makes the dtor not run
  // when the AtExitManager finishes.
  bool deleted2 = false;
  {
    butil::ShadowingAtExitManager shadow;
    static butil::LazyInstance<DeleteLogger>::Leaky test;
    test.Get().SetDeletedPtr(&deleted2);
  }
  EXPECT_FALSE(deleted2);
}
