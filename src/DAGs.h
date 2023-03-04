/*
File:   DAGs.h
Author: J. Ian Lindsay
Date:   2022.06.14

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Templates and types for a small collection of directed acyclic graphs (trees).
*/

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <inttypes.h>

#if defined(ARDUINO)
  #include <Arduino.h>
#elif defined(__MANUVR_LINUX)
  #include <stdlib.h>
  #include <pthread.h>
#elif defined(__BUILD_HAS_FREERTOS)
  #include "freertos/FreeRTOS.h"
#else
  #include <stdlib.h>
#endif


/*
* An ordered n-tree. Heap-driven.
* Always copies the underlying type by value, and does no memory management on
*   the templated type.
*/
template <class T> class C3PnTree {
  public:
    T data;

    C3PnTree(T d) : data(d) {};
    ~C3PnTree() {
      if (branches) {
        for (uint i = 0; i < _weight_allocated; i++) {
          if (nullptr != branches[i]) {
            _weight_occupied--;
            delete branches[i];
            branches[i] = nullptr;
          }
        }
        _weight_allocated = 0;
        free(branches);
        branches = nullptr;
      }
    };

    int add(T);                  // Returns the new weight of this node, or -1 on failure.
    int weight();                // Returns the number of nodes downstrem (including us).

    // TODO: Need variadic route arguments for some of these.
    /* Functions for removing downstream nodes. All return the number of nodes removed. */
    int remove(int position);    // Removes the distal node at the given index. Any branches are promoted.
    int remove(T);               // Removes any distal nodes with this data. Any branches are promoted.
    int prune(int position);     // Removes the distal node at the given index. Distal branches are dropped.
    int prune(T);                // Removes any distal nodes with this data. Distal branches are dropped.
    int clear();                 // Purges the entire tree. Returns the number of things purged or -1 on failure.

    T get();                      // Returns the data from this element.
    T get(int branch_idx);        // Returns the data from the element at the given branch.
    int getBranchIdx(T);          // Returns the branch index for the given element.
    bool hasBranch();             // Returns true if this node has branches.
    bool contains(T);             // Returns true if the given data appears in this (or distal) nodes.


  private:
    TreeNode<T>* branches = nullptr;
    int _weight_occupied  = 0;
    int _weight_allocated = 0;

    #if defined(__MANUVR_LINUX)
      // If we are on linux, we control for concurrency with a mutex...
      pthread_mutex_t _mutex;
    #elif defined(__BUILD_HAS_FREERTOS)
//      SemaphoreHandle_t _mutex;
    #endif

    TreeNode<T>* get_node_by_element(T);    // Return a node by the data it contains.
    int insert(TreeNode<T>*);    // Returns the index of the new branch, or -1 on failure.
    void _garbage_collect();     // If the tree underwent a large re-arrangement, free any slack space.
};




// /**
// * Constructor.
// */
// template <class T> PriorityQueue<T>::PriorityQueue() {
//   root = nullptr;
//   element_count = 0;
//   #ifdef __MANUVR_LINUX
//     #if defined (PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP)
//     _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
//     #else
//     _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
//     #endif
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    _mutex = xSemaphoreCreateRecursiveMutex();
//   #endif
// }
//
// /**
// * Destructor. Empties the queue.
// */
// template <class T> PriorityQueue<T>::~PriorityQueue() {
//   while (root != nullptr) {
//     dequeue();
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_destroy(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    vSemaphoreDelete(&_mutex);
//   #endif
// }
//
//
// /**
// * Inserts the given dataum into the queue.
// *
// * @param  d   The data to insert.
// * @return the position in the list that the data was inserted, or -1 on failure.
// */
// template <class T> int PriorityQueue<T>::insert(T d) {
//   return insert(d, 0);
// }
//
//
// /**
// * Inserts the given dataum into the queue.
// *
// * @param  d       The data to insert.
// * @param  nu_pri  The priority of the node.
// * @return the position in the list that the data was inserted, or -1 on failure.
// */
// template <class T> int PriorityQueue<T>::insertIfAbsent(T d, int nu_pri) {
//   // Note that we are going to replicate alot of code here for speed reasons.
//   // We could just as well have done with....
//   // return (contains(d)) ? false : insert(d, 0);
//
//   if (nullptr == root) {
//     // If root is null, we just insert and call it done. That way we
//     // don't have to worry about it later.
//     return insert(d, nu_pri);
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_lock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    if (pdTRUE != xSemaphoreTakeRecursive(&_mutex, 0)) {
// //      Serial.println("insertIfAbsent(T, int)");
// //      while(true);
// //    };
//   #endif
//
//   int return_value = -1;
//   PriorityNode<T>* insert_pointer = nullptr;
//   PriorityNode<T>* current = root;
//   while (nullptr != current) {
//     if (current->data == d) {
//       #ifdef __MANUVR_LINUX
//         pthread_mutex_unlock(&_mutex);
//       #elif defined(__BUILD_HAS_FREERTOS)
// //        xSemaphoreGiveRecursive(&_mutex);
//       #endif
//       return -1;
//     }
//     if (current->priority >= nu_pri) {
//       insert_pointer = current;
//       return_value++;
//     }
//     current = current->next;
//   }
//
//   PriorityNode<T> *nu = (PriorityNode<T>*) malloc(sizeof(PriorityNode<T>));
//   if (nullptr == nu) {
//     return_value = -1;      // Failed to allocate memory.
//   }
//   else {
//     nu->data     = d;
//     nu->priority = nu_pri;
//
//     if (nullptr == insert_pointer) {
//       nu->next = root;
//       root = nu;
//       return_value = 0;
//     }
//     else {
//       nu->next = insert_pointer->next;
//       insert_pointer->next = nu;
//     }
//     element_count++;
//   }
//
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_unlock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    xSemaphoreGiveRecursive(&_mutex);
//   #endif
//   return return_value;
// }
//
//
// /**
// * Inserts the given dataum into the queue.
// *
// * @param  d   The data to insert.
// * @return the position in the list that the data was inserted, or -1 on failure.
// */
// template <class T> int PriorityQueue<T>::insertIfAbsent(T d) {
//   return insertIfAbsent(d, 0);
// }
//
//
// /*
// * Returns the position in the list that the data was inserted, or -1 on failure.
// * Inserts at the first position that has a lower priority than the one given.
// */
// template <class T> int PriorityQueue<T>::insert(T d, int nu_pri) {
//   int return_value = -1;
//   PriorityNode<T> *nu = (PriorityNode<T>*) malloc(sizeof(PriorityNode<T>));
//   if (nu == nullptr) {
//     return return_value;      // Failed to allocate memory.
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_lock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    if (pdTRUE != xSemaphoreTakeRecursive(&_mutex, 0)) {
// //      Serial.println("insert(T, int)");
// //      while(true);
// //    };
//   #endif
//   nu->data     = d;
//   nu->priority = nu_pri;
//
//   return_value = insert(nu);
//   element_count++;
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_unlock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    xSemaphoreGiveRecursive(&_mutex);
//   #endif
//   return return_value;
// }
//
//
// /*
// * Returns the position in the list that the data was inserted, or -1 on failure.
// * Inserts at the first position that has a lower priority than the one given.
// */
// template <class T> int PriorityQueue<T>::insert(PriorityNode<T>* nu) {
//   if (nu == nullptr) {
//     return -1;
//   }
//
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_lock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    if (pdTRUE != xSemaphoreTakeRecursive(&_mutex, 0)) {
// //      Serial.println("insert(PriorityNode<T>*)");
// //      while(true);
// //    };
//   #endif
//   int idx = 0;
//   PriorityNode<T> *current = getLastWithPriority(nu->priority, &idx);
//
//   if (current == nullptr) {
//     nu->next = root;
//     root     = nu;
//   }
//   else {
//     nu->next      = current->next;
//     current->next = nu;
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_unlock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    xSemaphoreGiveRecursive(&_mutex);
//   #endif
//   return idx;
// }
//
//
//
// template <class T> int PriorityQueue<T>::size() {
//   return element_count;
// }
//
//
// template <class T> int PriorityQueue<T>::count() {
//   PriorityNode<T>* current = root;
//   int return_value = 0;
//   while (current != nullptr) {
//     current = current->next;
//     return_value++;
//   }
//   element_count = return_value;
//   return return_value;
// }
//
//
// /*
// * Returns a pointer to the last PriorityNode in this linked list.
// */
// template <class T> PriorityNode<T>* PriorityQueue<T>::getLast() {
//   PriorityNode<T>* return_value = root;
//   while ((return_value != nullptr) && (return_value->next != nullptr)) {
//     return_value = return_value->next;
//   }
//   return return_value;
// }
//
//
// /*
// * Returns a pointer to the first PriorityNode in this linked list with a matching element.
// */
// template <class T> PriorityNode<T>* PriorityQueue<T>::get_node_by_element(T test_data) {
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (current->data == test_data) {
//       return current;
//     }
//     current = current->next;
//   }
//   return nullptr;
// }
//
//
// /*
// * Returns a pointer to the last PriorityNode in this linked list.
// * Returns nullptr if there is not a node that has a priority at-least matching what we provided.
// */
// template <class T> PriorityNode<T>* PriorityQueue<T>::getLastWithPriority(int nu_pri, int* idx) {
//   PriorityNode<T>* current      = root;
//   PriorityNode<T>* last         = nullptr;
//   while ((current != nullptr) && (nu_pri <= current->priority)) {
//     last    = current;
//     current = current->next;
//     *idx = *idx + 1;
//   }
//   return last;
// }
//
//
// template <class T> void PriorityQueue<T>::enforce_priorities(void) {
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_lock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    if (pdTRUE != xSemaphoreTakeRecursive(&_mutex, 0)) {
// //      Serial.println("enforce_priorities()");
// //      while(true);
// //    };
//   #endif
//   PriorityNode<T>* current  = root;
//   PriorityNode<T>* prior    = nullptr;
//   while (current != nullptr) {
//     if (prior == nullptr) {              // If current is the root node...
//       if (current->next != nullptr) {  // ...and there are at least two items in the queue...
//         if (current->next->priority > current->priority) {  // ...and they are out of order...
//           root = current->next;         // Swap them.
//           current->next = root->next;
//           root->next = current;
//
//           current = root;               // Restart the test.
//         }
//       }
//     }
//     else {
//       if (prior->priority < current->priority) {
//         // We need to promote current to come before prior in the queue.
//         // We won't bother juggling this here. Instead, we'll simply remove
//         // current and re-insert it.
//         prior->next = current->next;   // Effectively removes current.
//         insert(current);
//
//         // Restart the test.
//         current = root;
//       }
//     }
//
//     prior = current;
//     current = current->next;
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_unlock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    xSemaphoreGiveRecursive(&_mutex);
//   #endif
// }
//
//
// template <class T> bool PriorityQueue<T>::incrementPriority(T test_data) {
//   PriorityNode<T>* current = get_node_by_element(test_data);
//   if (current != nullptr) {
//     current->priority++;
//     enforce_priorities();
//     return true;
//   }
//   return false;
// }
//
// template <class T> bool PriorityQueue<T>::decrementPriority(T test_data) {
//   PriorityNode<T>* current = get_node_by_element(test_data);
//   if (current != nullptr) {
//     current->priority--;
//     enforce_priorities();
//     return true;
//   }
//   return false;
// }
//
//
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic push
// #endif
// #pragma GCC diagnostic ignored "-Wconversion-null"
// template <class T> T PriorityQueue<T>::dequeue() {
//   T return_value = nullptr;
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_lock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    if (pdTRUE != xSemaphoreTakeRecursive(&_mutex, 0)) {
// //      Serial.println("dequeue()");
// //      while(true);
// //    };
//   #endif
//   PriorityNode<T>* current = root;
//   if (current != nullptr) {
//     // This is safe because we only store references. Not the actual data.
//     return_value = current->data;
//     root = current->next;
//     free(current);
//     element_count--;
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_unlock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    xSemaphoreGiveRecursive(&_mutex);
//   #endif
//   return return_value;
// }
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic pop
// #endif
//
//
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic push
// #endif
// #pragma GCC diagnostic ignored "-Wconversion-null"
// template <class T> T PriorityQueue<T>::recycle() {
//   T return_value = nullptr;
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_lock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    if (pdTRUE != xSemaphoreTakeRecursive(&_mutex, 0)) {
// //      Serial.println("recycle()");
// //      while(true);
// //    };
//   #endif
//   PriorityNode<T>* current = root;
//   if (current != nullptr) {
//     return_value = current->data;
//
//     if (element_count > 1) {
//       root = current->next;
//       current->next = nullptr;
//
//       PriorityNode<T>* temp = getLast();
//       current->priority = temp->priority;  // Demote to the rear of the queue.
//       temp->next = current;
//     }
//   }
//   #ifdef __MANUVR_LINUX
//     pthread_mutex_unlock(&_mutex);
//   #elif defined(__BUILD_HAS_FREERTOS)
// //    xSemaphoreGiveRecursive(&_mutex);
//   #endif
//   return return_value;
// }
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic pop
// #endif
//
//
// template <class T> int PriorityQueue<T>::clear(void) {
//   int return_value = 0;
//   while (remove(0)) {
//     return_value++;
//   }
//   return return_value;
// }
//
//
// template <class T> bool PriorityQueue<T>::remove(int pos) {
//   int i = 0;
//   PriorityNode<T>* prior   = nullptr;
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (i == pos) {
//       if (prior != nullptr) {
//         prior->next = current->next;
//       }
//       else {
//         root = current->next;
//       }
//       free(current);
//       element_count--;
//       return true;
//     }
//     i++;
//     prior = current;
//     current = current->next;
//   }
//   return false;
// }
//
//
// template <class T> bool PriorityQueue<T>::remove(T test_data) {
//   PriorityNode<T>* prior   = nullptr;
//   PriorityNode<T>* current = root;
//   bool return_value = false;
//   while (current != nullptr) {
//     if ((current->data != nullptr) && (current->data == test_data)) {
//       if (prior != nullptr) {
//         prior->next = current->next;
//       }
//       else {
//         root = current->next;
//       }
//       free(current);
//       element_count--;
//       return_value = true;
//       current = (nullptr == prior) ? nullptr : prior->next;
//     }
//     else {
//       prior = current;
//       current = current->next;
//     }
//   }
//   return return_value;
// }
//
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic push
// #endif
// #pragma GCC diagnostic ignored "-Wconversion-null"
// template <class T> T PriorityQueue<T>::get() {
//   PriorityNode<T>* current = root;
//   if (current != nullptr) {
//     return current->data;
//   }
//   return nullptr;
// }
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic pop
// #endif
//
//
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic push
// #endif
// #pragma GCC diagnostic ignored "-Wconversion-null"
// template <class T> T PriorityQueue<T>::get(int pos) {
//   int i = 0;
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (i == pos) {
//       return current->data;
//     }
//     i++;
//     current = current->next;
//   }
//   return nullptr;
// }
// #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
// #pragma GCC diagnostic pop
// #endif
//
//
// template <class T> bool PriorityQueue<T>::contains(T test_data) {
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (current->data == test_data) {
//       return true;
//     }
//     current = current->next;
//   }
//   return false;
// }
//
//
// template <class T> int PriorityQueue<T>::getPosition(T test_data) {
//   int i = 0;
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (current->data == test_data) {
//       return i;
//     }
//     i++;
//     current = current->next;
//   }
//   return -1;
// }
//
//
// template <class T> int PriorityQueue<T>::getPriority(T test_data) {
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (current->data == test_data) {
//       return current->priority;
//     }
//     current = current->next;
//   }
//   return -1;
// }
//
//
// template <class T> int PriorityQueue<T>::getPriority(int position) {
//   int i = 0;
//   PriorityNode<T>* current = root;
//   while (current != nullptr) {
//     if (position == i++) return current->priority;
//     current = current->next;
//   }
//   return -1;
// }
//
//
// template <class T> bool PriorityQueue<T>::hasNext() {
//   return (root != nullptr);
// }


#endif
