#pragma once
#include <stdint.h>
#include <netinet/in.h>

typedef struct sockaddr sockaddr;

// represent the list element
typedef struct addressListItem {
    struct addressListItem* prev;
    struct addressListItem* next;
    sockaddr address;
} addressListItem;

// represent the whole list
typedef struct addressListHead {
    addressListItem* first;
    addressListItem* last;
    int size;
} addressListHead;

void addressList_init(addressListHead* head);
addressListItem* addressList_find(addressListHead* head, addressListItem* item);
addressListItem* addressList_insert(addressListHead* head, addressListItem* previous, addressListItem* item);
addressListItem* addressList_detach(addressListHead* head, addressListItem* item);
addressListItem* addressListItem_init(sockaddr address);
sockaddr insertAddress(addressListHead* l, sockaddr address);
addressListItem* getAddress(addressListHead* l,sockaddr address);
void removeAddress(addressListHead* l, sockaddr address);
