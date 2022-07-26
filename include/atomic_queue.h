///
/// Created by Anonymous275 on 7/26/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <queue>
#include <semaphore>

template<class T, size_t Size>
class atomic_queue {
   public:
   bool try_pop(T& val) {
      lock_guard guard(semaphore);
      if (queue.empty()) return false;
      val = queue.front();
      queue.pop();
      full.release();
      return true;
   }

   void push(const T& val) {
      check_full();
      lock_guard guard(semaphore);
      queue.push(val);
   }

   size_t size() {
      lock_guard guard(semaphore);
      return queue.size();
   }

   bool empty() {
      lock_guard guard(semaphore);
      return queue.empty();
   }

   private:
   void check_full() {
      if (size() >= Size) {
         full.acquire();
      }
   }

   private:
   struct lock_guard {
      explicit lock_guard(std::binary_semaphore& lock) : lock(lock) {
         lock.acquire();
      }
      ~lock_guard() { lock.release(); }

      private:
      std::binary_semaphore& lock;
   };
   std::binary_semaphore semaphore{1}, full{0};
   std::queue<T> queue{};
};
