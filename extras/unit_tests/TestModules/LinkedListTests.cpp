/*
File:   LinkedListTests.cpp
Author: J. Ian Lindsay
Date:   2021.10.08

Copyright 2021 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This program runs tests against the classes that are essentially linked lists.
That includes LinkedList, PriorityQueue, and possibly others.
*/

/*******************************************************************************
* PriorityQueue test routines
*******************************************************************************/
// Tests for:
//   int insert(T);
//   T get(void);
//   T get(int position);
//   bool contains(T);
//   bool hasNext(void);
//   int clear(void);
int test_PriorityQueue0(StringBuilder* log) {
  int return_value = -1;
  PriorityQueue<uint32_t*> queue0;
  uint32_t vals[16] = { randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32() };
  int q_size = queue0.size();
  if (0 == q_size) {
    if (!queue0.contains(&vals[5])) {  // Futile search for non-existant value.
      // Populate the queue...
      for (int i = 0; i < (int) sizeof(vals); i++) {
        int q_pos = queue0.insert(&vals[i]);
        if (i != q_pos) {
          log->concatf("Returned index from queue insertion didn't match the natural order. %d verus %d.\n", i, q_pos);
          return -1;
        }
      }
      q_size = queue0.size();
      if (sizeof(vals) == q_size) {
        if (queue0.hasNext()) {
          bool contains_all_elements = true;
          bool contains_all_elements_in_order = true;
          for (int i = 0; i < q_size; i++) {
            contains_all_elements &= queue0.contains(&vals[i]);
            contains_all_elements_in_order &= (*queue0.get(i) == vals[i]);
          }
          if (contains_all_elements & contains_all_elements_in_order) {
            if (*queue0.get(0) == vals[0]) {
              int q_clear_val = queue0.clear();
              if (q_size == q_clear_val) {
                if (0 == queue0.size()) {
                  if (!queue0.hasNext()) {
                    return_value = 0;
                  }
                  else log->concat("hasNext() reports true, when it ought to report false.\n");
                }
                else log->concat("The queue's size ought to be zero, but it isn't.\n");
              }
              else log->concatf("clear() ought to have cleared %d value. But it reports %d.\n", q_size, q_clear_val);
            }
            else log->concat("The queue's first element return didn't match the first element.\n");
          }
          else log->concat("Queue didn't contain all elements in their natural order.\n");
        }
        else log->concat("hasNext() reports false, when it ought to report true.\n");
      }
      else log->concatf("Queue didn't take all elements. Expected %u, but got %d.\n", sizeof(vals), q_size);
    }
    else log->concat("Queue claims to have a value it does not.\n");
  }
  else log->concat("Empty queue reports a non-zero size.\n");
  return return_value;
}


// Tests for:
//   int insertIfAbsent(T);
//   bool remove(T);
//   bool remove(int position);
//   int getPosition(T);
int test_PriorityQueue1(StringBuilder* log) {
  int return_value = -1;
  PriorityQueue<uint32_t*> queue0;
  uint32_t vals[16] = { 234, 734, 733, 7456, 819, 943, 223, 936,
                        134, 634, 633, 6456, 719, 843, 123, 836 };
  int vals_accepted = 0;
  int vals_rejected = 0;
  for (int n = 0; n < 2; n++) {
    for (unsigned int i = 0; i < sizeof(vals); i++) {
      if (-1 != queue0.insertIfAbsent(&vals[i])) {
        vals_accepted++;
      }
      else {
        vals_rejected++;
      }
    }
  }
  int q_size = queue0.size();
  if (vals_accepted == q_size) {
    if (vals_rejected == sizeof(vals)) {
      // Try some removal...
      if (!queue0.remove((int) sizeof(vals))) {  // This ought to fail.
        if (!queue0.remove((int) -1)) {   // This is not a PHP array. Negative indicies are disallowed.
          if (vals_accepted == q_size) {  // Is the size unchanged?
            if (queue0.remove((int) vals_accepted - 1)) {  // Remove the last element.
              if (queue0.remove((int) 1)) {                // Remove element at position 1.
                if (queue0.remove(&vals[4])) {             // Remove the value 819.
                  if (234 == *queue0.get()) {              // Does not change queue.
                    if (234 == *queue0.dequeue()) {        // Removes the first element.
                      q_size = queue0.size();
                      if (q_size == (vals_accepted - 4)) { // Four removals have happened.
                        if (2 == queue0.getPosition(&vals[5])) {
                          if (-1 == queue0.getPosition(&vals[4])) {
                            return_value = 0;
                          }
                          else log->concat("A previously removed element was found.\n");
                        }
                        else log->concat("Known element is not at the position it is expected to be.\n");
                      }
                      else log->concat("The queue is not the expected size following removals.\n");
                    }
                    else log->concat("dequeue(): First element is wrong.\n");
                  }
                  else log->concat("get(): First element is wrong.\n");
                }
                else log->concat("Queue remove() returned failure when it ought not to have (named value).\n");
              }
              else log->concat("Queue remove() returned failure when it ought not to have (intermediary index).\n");
            }
            else log->concat("Queue remove() returned failure when it ought not to have (last index).\n");
          }
          else log->concat("Queue operation that ought not to have changed the size have done so anyhow.\n");
        }
        else log->concat("Queue remove() returned success when it ought not to have (negative index).\n");
      }
      else log->concat("Queue remove() returned success when it ought not to have (out-of-bounds index).\n");
    }
    else log->concatf("vals_rejected=%d, but should have been %d.\n", vals_rejected, sizeof(vals));
  }
  else log->concatf("Queue acceptance mismatch. q_size=%d   vals_accepted=%d   vals_rejected=%d\n", q_size, vals_accepted, vals_rejected);
  return return_value;
}


int test_PriorityQueue() {
  StringBuilder log("===< PriorityQueue >====================================\n");
  int return_value = -1;
  if (0 == test_PriorityQueue0(&log)) {
    if (0 == test_PriorityQueue1(&log)) {
      return_value = 0;
    }
  }

  //T dequeue(void);              // Removes the first element of the list. Return the node's data on success, or nullptr on empty queue.
  //T recycle(void);              // Reycle this element. Return the node's data on success, or nullptr on empty queue.

  //int insert(T, int priority);  // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
  //int insertIfAbsent(T, int);   // Same as above, but also specifies the priority if successful.
  //int getPriority(T);           // Returns the priority in the queue for the given element.
  //int getPriority(int position);           // Returns the priority in the queue for the given element.
  //bool incrementPriority(T);    // Finds the given T and increments its priority by one.
  //bool decrementPriority(T);    // Finds the given T and decrements its priority by one.
  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


void print_types_linked_lists() {
  printf("\tLinkedList<uint8_t>      %u\t%u\n", sizeof(LinkedList<uint8_t>),  alignof(LinkedList<uint8_t>));
  printf("\tLinkedList<void*>        %u\t%u\n", sizeof(LinkedList<void*>),    alignof(LinkedList<void*>));
  printf("\tPriorityQueue<uint8_t>   %u\t%u\n", sizeof(PriorityQueue<uint8_t>),  alignof(PriorityQueue<uint8_t>));
  printf("\tPriorityQueue<void*>     %u\t%u\n", sizeof(PriorityQueue<void*>),    alignof(PriorityQueue<void*>));
}


/*******************************************************************************
* LinkedList test routines
*******************************************************************************/

int test_LinkedList() {
  int return_value = -1;
  StringBuilder log("===< LinkedList >=======================================\n");
  const int TEST_SIZE = 18;
  LinkedList<uint32_t*> a;
  uint32_t ref_vals[TEST_SIZE] = {0, };
  for (unsigned int i = 0; i < TEST_SIZE; i++) {
    ref_vals[i] = randomUInt32();
    if (0 > a.insert(&ref_vals[i])) {
      log.concat("\nFailed to insert.\n");
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    log.concatf(" (%u: %08x)", a.size(), ref_vals[i]);
  }
  if (TEST_SIZE == a.size()) {
    log.concat("\n\tGetting:  ");
    for (unsigned int i = 0; i < TEST_SIZE/2; i++) {
      uint32_t* val = a.get(i);
      log.concatf(" (%u: %08x)", i, *val);
      if (*val != ref_vals[i]) {
        log.concatf("Value mismatch at index %u.\n", i);
        printf("%s\n\n", (const char*) log.string());
        return -2;
      }
    }
    if (TEST_SIZE == a.size()) {
      log.concat("\n\tRemoving:  ");
      for (unsigned int i = 0; i < TEST_SIZE; i++) {
        uint32_t* val = a.remove();
        log.concatf(" (%u: %08x)", i, *val);
        if (*val != ref_vals[i]) {
          log.concatf("Value mismatch at index %u.\n", i);
          printf("%s\n\n", (const char*) log.string());
          return -3;
        }
      }
      if (0 == a.size()) {
        if (nullptr == a.remove()) {
          a.insert(&ref_vals[0]);
          a.insert(&ref_vals[1]);
          if (nullptr == a.remove(15)) {
            if (nullptr == a.get(15)) {
              a.clear();
              if (0 == a.size()) {
                a.insert(&ref_vals[2]);
                return_value = 0;
              }
              else log.concatf("Count should be 0, but is %u\n", a.size());
            }
            else log.concatf("Sadly worked. Count is %u\n", a.size());
          }
          else log.concatf("Sadly worked. Count is %u\n", a.size());
        }
        else log.concatf("Sadly worked. Count is %u\n", a.size());
      }
      else log.concatf("Count should have been 0 but is %u\n", a.size());
    }
    else log.concatf("It appears get() removed elements. The count says %u.\n", a.size());
  }
  else log.concatf("Fairly certain we inserted %u elements, but the count says %u.\n", TEST_SIZE, a.size());

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}
