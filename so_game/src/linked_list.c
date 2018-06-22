#include "linked_list.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

void List_init(ListHead* head) {
  head->first=0;
  head->last=0;
  head->size=0;
}

ListItem* List_find(ListHead* head, ListItem* item) {
  // linear scanning of list
  ListItem* aux=head->first;
  while(aux){
    if (aux==item)
      return item;
    aux=aux->next;
  }
  return 0;
}

ListItem* List_insert(ListHead* head, ListItem* prev, ListItem* item) {
  if (item->next || item->prev)
    return 0;
  
#ifdef _LIST_DEBUG_
  // we check that the element is not in the list
  ListItem* instance=List_find(head, item);
  assert(!instance);

  // we check that the previous is inthe list

  if (prev) {
    ListItem* prev_instance=List_find(head, prev);
    assert(prev_instance);
  }
  // we check that the previous is inthe list
#endif

  ListItem* next= prev ? prev->next : head->first;
  if (prev) {
    item->prev=prev;
    prev->next=item;
  }
  if (next) {
    item->next=next;
    next->prev=item;
  }
  if (!prev)
    head->first=item;
  if(!next)
    head->last=item;
  ++head->size;
  return item;
}

ListItem* List_detach(ListHead* head, ListItem* item) {

#ifdef _LIST_DEBUG_
  // we check that the element is in the list
  ListItem* instance=List_find(head, item);
  assert(instance);
#endif

  ListItem* prev=item->prev;
  ListItem* next=item->next;
  if (prev){
    prev->next=next;
  }
  if(next){
    next->prev=prev;
  }
  if (item==head->first)
    head->first=next;
  if (item==head->last)
    head->last=prev;
  head->size--;
  item->next=item->prev=0;
  return item;
}

// creates a list item with a socket
ListItem* ListItem_init(int sock){
  ListItem* item = (ListItem*)malloc(sizeof(ListItem));
  item->info = sock;
  item->prev = NULL;
  item->next = NULL;
  return item; // new
}

// add a list item to a list (head)
int insertSockFD(ListHead* head, int sock){
  ListItem* item = ListItem_init(sock);
  ListItem* result = List_insert(head, head->last, item);
  return result->info;
}


// returns the list item containing the specified socket
ListItem* getSockFD(ListHead* head, int sock){
  ListItem* item = head->first;
  while(item){
    if(item->info == sock) return item;
    item=item->next;
  }
  return NULL;
}

void removeSockFD(ListHead* head, int sock){
  ListItem* to_remove = getSockFD(head,sock);
  List_detach(head, to_remove);
}

