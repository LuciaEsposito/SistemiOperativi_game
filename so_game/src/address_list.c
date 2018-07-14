#include "address_list.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

void addressList_init(addressListHead* head) {
    head->first=0;
    head->last=0;
    head->size=0;
}

addressListItem* addressList_find(addressListHead* head, addressListItem* item) {
    // linear scanning of list
    addressListItem* aux=head->first;
    while(aux){
        if (aux==item)
            return item;
        aux=aux->next;
    }
    return 0;
}

addressListItem* addressList_insert(addressListHead* head, addressListItem* prev, addressListItem* item) {
    if (item->next || item->prev)
        return 0;

#ifdef _LIST_DEBUG_
    // we check that the element is not in the list
  addressListItem* instance=addressList_find(head, item);
  assert(!instance);

  // we check that the previous is inthe list

  if (prev) {
    addressListItem* prev_instance=addressList_find(head, prev);
    assert(prev_instance);
  }
  // we check that the previous is inthe list
#endif

    addressListItem* next= prev ? prev->next : head->first;
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

addressListItem* addressList_detach(addressListHead* head, addressListItem* item) {

#ifdef _LIST_DEBUG_
    // we check that the element is in the list
  addressListItem* instance=addressList_find(head, item);
  assert(instance);
#endif

    addressListItem* prev=item->prev;
    addressListItem* next=item->next;
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
addressListItem* addressListItem_init(sockaddr address){
    addressListItem* item = (addressListItem*)malloc(sizeof(addressListItem));
    item->address = address;
    item->prev = NULL;
    item->next = NULL;
    return item; // new
}

// add an address to the list head
sockaddr insertAddress(addressListHead* head, sockaddr address){
    addressListItem* item = addressListItem_init(address);
    addressListItem* result = addressList_insert(head, head->last, item);
    return result->address;
}


// returns the list item containing the specified address
addressListItem* getAddress(addressListHead* head, sockaddr address){
    addressListItem* item = head->first;
    start:
    while(item){
        sockaddr current_address = item->address;
        int i;
        for( i = 0; i < 14; i++){
            if(current_address.sa_data[i] != address.sa_data[i]) {
                item = item->next;
                goto start;
            }
        }
        return item;
    }
    return NULL;
}

// removes an address
void removeAddress(addressListHead* head, sockaddr address){
    addressListItem* to_remove = getAddress(head,address);
    addressList_detach(head, to_remove);
}