#pragma once
#include <stdint.h>

// represent the list element
typedef struct ListItem {
  struct ListItem* prev;
  struct ListItem* next;
  int info;
} ListItem;

// represent the whole list
typedef struct ListHead {
  ListItem* first;
  ListItem* last;
  int size;
} ListHead;

void List_init(ListHead* head);
ListItem* List_find(ListHead* head, ListItem* item);
ListItem* List_insert(ListHead* head, ListItem* previous, ListItem* item);
ListItem* List_detach(ListHead* head, ListItem* item);
ListItem* ListItem_init(int sock);
int insertSockFD(ListHead* l, int sock);
ListItem* getSockFD(ListHead* l, int sock);
void removeSockFD(ListHead* l, int sock);