/*
File:   LightLinkedList.h
Author: J. Ian Lindsay
Date:   2014.03.28

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


TODO: This entire file should eventually contain all the linked-list fundamental
  datastructs, and be renamed to C3PLinkedList.h.

Some functions are #pragma'd to stop the compiler from complaining about nullptr being
  interpreted as a non-pointer. This is to enable the use of this template to carry
  (doubles, floats, ints, etc) without polluting the build log.
*/

#ifndef __C3P_DAGS_H
#define __C3P_DAGS_H

#include "CppPotpourri.h"

/* This is a linked-list element with a slot for data. */
template <class T> class Node{
  public:
    Node<T>* next;
    T data;
};

/* This is the class that holds a datum with a priority slot. */
template <class T> class PriorityNode{
  public:
    PriorityNode<T>* next;
    T data;
    int priority;
};



/*******************************************************************************
* Template for a linked list interface.
* TODO: LinkedList and PriorityQueue should both extend this.
*******************************************************************************/
//template <class T> class C3P_LL {
//  public:
//    C3P_LL();
//    ~C3P_LL();
//
//    int  count();                   // Returns the number of elements in this list.
//    int  insert(T);                 // Returns the index of the data, or -1 on failure.
//    T    get(int position = 0);     // Returns the data from the element at the given position.
//    int  remove(T);                 // Removes any elements with this data. Returns the number of elements removed.
//    T    remove(int position = 0);  // Removes the element at the given position of the list. Return true on success.
//    bool hasNext();                 // Returns false if this list is empty. True otherwise.
//    bool contains(T);               // Returns true if this list contains the given data. False if not.
//    int  clear();                   // Returns the number of elements purged from the list.
//
//  protected:
//    int _element_count;             // Cached element count to speed access.
//    int _count_elements();          // Counts the elements in the list without reliance on stored value. Slower...
//
//    Node<T>* _allocate_node(T) =0;  // Type-specific allocation of node objects.
//
//  private:
//    // This is a reference to the general list node. It will be type-punned into
//    //   a more nuanced version of the basse class by classes that extend the
//    //   basic linked-list.
//    Node<T>* _root;
//};


/*******************************************************************************
* Template for an instantiable linked list.
*******************************************************************************/
template <class T> class LinkedList {
  public:
    LinkedList(void);
    ~LinkedList(void);

    int size(void);                  // Returns the number of elements in this list.
    int insert(T);                   // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
    T get(void);                     // Returns the data from the first element.
    T get(int position);             // Returns the data from the element at the given position.
    T remove(void);                  // Removes the first element of the list. Return true on success.
    bool remove(T);                  // Removes any elements with this data.
    T remove(int position);          // Removes the element at the given position of the list. Return true on success.
    bool hasNext(void);              // Returns false if this list is empty. True otherwise.
    bool contains(T);                // Returns true if this list contains the given data. False if not.
    int clear(void);                 // Returns the number of elements purged from the list.

    int insertAtHead(T);             // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
    //bool move(int old_position, int new_position);


  private:
    Node<T>* root;
    Node<T>* getLast(void);          // Returns the last element in the list. Returns the WHOLE element, not just the data it holds.
    int element_count;               // Call this member if you are pressed for time and haven't changed the list recently.
    int count(void);                 // Counts the elements in the list without reliance on stored value. Slower...
};


/**
* Constructor.
*/
template <class T> LinkedList<T>::LinkedList() {
  root = nullptr;
  element_count = 0;
}


/**
* Destructor. Empties the list.
*/
template <class T> LinkedList<T>::~LinkedList() {
  while (root != nullptr) {
    remove();
  }
}


/**
* Empties the list.
*
* @return the number of elements removed by the call.
*/
template <class T> int LinkedList<T>::clear(void) {
  int return_value = 0;
  while (root != nullptr) {
    if (remove()) return_value++;
  }
  return return_value;
};


/**
* Inserts the given dataum into the linked list.
*
* @param  d   The data to copy and insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int LinkedList<T>::insert(T d) {
  Node<T> *current;
  if (root == nullptr) {
    root = (Node<T>*) malloc(sizeof(Node<T>));
    current = root;
  }
  else {
    current = getLast();
    current->next = (Node<T>*) malloc(sizeof(Node<T>));
    current = current->next;
  }
  current->next = nullptr;
  current->data = d;
  element_count++;
  return 1;
}


/**
* Inserts the given dataum into the linked list at the head position.
* Useful for treating the list like a stack.
*
* @param  d   The data to copy and insert.
* @return the position in the list that the data was inserted, or -1 on failure.
*/
template <class T> int LinkedList<T>::insertAtHead(T d) {
  Node<T> *current = (Node<T>*) malloc(sizeof(Node<T>));
  if (current) {
    current->data = d;
    current->next = root;
    root = current;
    element_count++;
    return 0;
  }
  return -1;
}


/**
* How many elements in the list? Uses the cached value rather than traversing
*   the list for each call.
*
* @return the number of elements in the linked list.
*/
template <class T> int LinkedList<T>::size() {
  return element_count;
}


/**
* How many elements in the list? Actually traverses the list and updates the
*   cached count value.
*
* @return the number of elements in the linked list.
*/
template <class T> int LinkedList<T>::count() {
  Node<T>* current = root;
  int return_value = 0;
  while (current != nullptr) {
    current = current->next;
    return_value++;
  }
  element_count = return_value;
  return return_value;
}


/**
* Convenience function for finding the tail of the list.
*
* @return a pointer to the last node in this linked list.
*/
template <class T> Node<T>* LinkedList<T>::getLast() {
  Node<T>* return_value = root;
  while ((return_value != nullptr) && (return_value->next != nullptr)) {
    return_value = return_value->next;
  }
  return return_value;
}


/**
* Removes the element at the head of the list.
*
* @return the thing removed, if anything. nullptr otherwise.
*/
template <class T> T LinkedList<T>::remove() {
  T return_value = nullptr;
  Node<T>* current = root;
  if (current != nullptr) {
    return_value = current->data;
    root = current->next;
    free(current);
    element_count--;
  }
  return return_value;
}


/**
* Removes the element at the given position within the list.
*
* @param  pos The position to remove.
* @return the thing removed, if anything. nullptr otherwise.
*/
template <class T> T LinkedList<T>::remove(int pos) {
  int i = 0;
  T return_value   = nullptr;
  Node<T>* prior   = nullptr;
  Node<T>* current = root;
  while (current != nullptr) {
    if (i == pos) {
      if (prior != nullptr) {
        prior->next = current->next;
      }
      else {
        root = current->next;
      }
      return_value = current->data;
      free(current);
      element_count--;
      return return_value;
    }
    i++;
    prior = current;
    current = current->next;
  }
  return return_value;
}


/**
* Removes the element with the given data.
*
* @param  test_data  The data to remove.
* @return true if something was removed. False otherwise.
*/
template <class T> bool LinkedList<T>::remove(T test_data) {
  Node<T>* prior   = nullptr;
  Node<T>* current = root;
  bool return_value = false;
  while (current != nullptr) {
    if ((current->data != nullptr) && (current->data == test_data)) {
      if (prior != nullptr) {
        prior->next = current->next;
      }
      else {
        root = current->next;
      }
      free(current);
      element_count--;
      return_value = true;
      current = (nullptr == prior) ? nullptr : prior->next;
    }
    else {
      prior = current;
      current = current->next;
    }
  }
  return return_value;
}


/**
* Get the first piece of data.
*
* @return The data from the element at the head of the list, or nullptr on empty list.
*/
template <class T> T LinkedList<T>::get() {
  Node<T>* current = root;
  if (current != nullptr) {
    return current->data;
  }
  return nullptr;
}


/**
* Get the piece of data at the given position in the list.
*
* @param  pos The position to fetch.
* @return The data from the requested element, or nullptr if the list isn't that large.
*/
template <class T> T LinkedList<T>::get(int pos) {
  int i = 0;
  Node<T>* current = root;
  while (current != nullptr) {
    if (i == pos) {
      return current->data;
    }
    i++;
    current = current->next;
  }
  return nullptr;
}


/**
* Does this list have more elements in it?
*
* @return true if the list is not empty. False if it is.
*/
template <class T> bool LinkedList<T>::hasNext() {
  return (root != nullptr);
}


/**
* Does this list contain the given data?
*
* @param  test_data The data to search for.
* @return true if the list has the given data. False otherwise.
*/
template <class T> bool LinkedList<T>::contains(T test_data) {
  Node<T>* current = root;
  while (current != nullptr) {
    if ((current->data != nullptr) && (current->data == test_data)) {
      return true;
    }
    current = current->next;
  }
  return false;
}


/*******************************************************************************
* Template for a priority queue implemented on top of a linked-list.
* Highest-priority nodes are stored closest to the beginning of the list.
*******************************************************************************/


#endif  // __C3P_DAGS_H
