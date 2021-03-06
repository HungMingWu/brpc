// Copyright (c) 2016 Baidu, Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
 
// Author: Ge,Jun (gejun@baidu.com)
// Date: Thu Dec 15 14:37:39 CST 2016

#ifndef BUTIL_MEMORY_SINGLETON_ON_PTHREAD_ONCE_H
#define BUTIL_MEMORY_SINGLETON_ON_PTHREAD_ONCE_H

#include <atomic>
#include <mutex>

namespace butil {

template <typename T> class GetLeakySingleton {
public:
    static std::atomic_intptr_t g_leaky_singleton_untyped;
    static std::once_flag g_create_leaky_singleton_once;
    static void create_leaky_singleton();
};
template <typename T>
std::atomic_intptr_t GetLeakySingleton<T>::g_leaky_singleton_untyped { 0 };

template <typename T>
std::once_flag GetLeakySingleton<T>::g_create_leaky_singleton_once;

template <typename T>
void GetLeakySingleton<T>::create_leaky_singleton() {
    T* obj = new T;
    g_leaky_singleton_untyped.store(
        reinterpret_cast<intptr_t>(obj), std::memory_order_release);
}

// To get a never-deleted singleton of a type T, just call get_leaky_singleton<T>().
// Most daemon threads or objects that need to be always-on can be created by
// this function.
// This function can be called safely before main() w/o initialization issues of
// global variables.
template <typename T>
inline T* get_leaky_singleton() {
    const intptr_t value =
        GetLeakySingleton<T>::g_leaky_singleton_untyped.load(std::memory_order_acquire);
    if (value) {
        return reinterpret_cast<T*>(value);
    }
    std::call_once(GetLeakySingleton<T>::g_create_leaky_singleton_once,
                 GetLeakySingleton<T>::create_leaky_singleton);
    return reinterpret_cast<T*>(
        GetLeakySingleton<T>::g_leaky_singleton_untyped.load());
}

// True(non-NULL) if the singleton is created.
// The returned object (if not NULL) can be used directly.
template <typename T>
inline T* has_leaky_singleton() {
    return reinterpret_cast<T*>(
            GetLeakySingleton<T>::g_leaky_singleton_untyped.load(std::memory_order_acquire));
}

} // namespace butil

#endif // BUTIL_MEMORY_SINGLETON_ON_PTHREAD_ONCE_H
