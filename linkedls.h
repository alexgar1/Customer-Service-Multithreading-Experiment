#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

typedef struct Node Node;

typedef struct Node {
    int id;         // Customer ID
    int class;      // Customer class (0 for Economy, 1 for Business)
    int svct;       // Service time in tenths of a second
    int arrivet;    // Arrival time in tenths of a second
    struct Node* next;
} Node;


Node* newNode(Node* head, int id, int class, int svct, int arrivet);
Node* deleteNode(Node *head, int id);
void printQueue(Node *head);



#endif