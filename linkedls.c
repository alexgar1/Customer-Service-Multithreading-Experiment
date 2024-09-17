#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linkedls.h"


Node* newNode(Node* head, int nid, int nclass, int narrivet, int nsvct) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Malloc failed");
        return head;
    }

    new_node->id = nid;
    new_node->class = nclass;
    new_node->svct = nsvct;
    new_node->arrivet = narrivet;
    new_node->next = NULL;

    if (head == NULL) {
        head = new_node;
    } else {
        Node *tmp = head;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new_node;
    }

    return head;
}


Node* deleteNode(Node *head, int id) {
    Node *tmp = head;
    Node *prev = NULL;

    if (tmp == NULL) {
        return head;  // Empty list, nothing to do
    }

    if (tmp->id == id) {  // The head is the node to delete
        head = tmp->next;
        free(tmp);
        return head;
    }

    while (tmp != NULL && tmp->id != id) {
        prev = tmp;
        tmp = tmp->next;
    }

    if (tmp == NULL) {
        return head;  // Node not found
    }

    prev->next = tmp->next;  // Unlink the node from the list
    free(tmp);

    return head;
}

void printQueue(Node *head) {
    if (head == NULL) {
        //printf("The queue is empty.\n");
        return;
    }

    int count = 0;
    Node *current = head;
    while (current != NULL) {
        if (current->next != NULL) {
            printf("%d -> ", current->id);
        } else {
            printf("%d\n", current->id);
        }
        current = current->next;
        count++;
    }
    //printf("Total customers in queue: %d\n", count);
}



