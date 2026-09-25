// 单向链表
#include <stdio.h>
#include <stdlib.h>
struct Node
{
    int value;
    struct Node *next;
};
typedef struct Node Node;
typedef struct
{
    Node dummy;
} List;
void list_init(List *list)
{
    list->dummy.value = 0;
    list->dummy.next = NULL;
}
void list_free(List *list)
{
    Node *node = list->dummy.next;
    while (node)
    {
        Node *next = node->next;
        free(node);
        node = next;
    }
    list->dummy.next = NULL;
}
Node *list_before_begin(List *list)
{
    return &list->dummy;
}
Node *list_begin(List *list)
{
    return list->dummy.next;
}
Node *list_insert_after(Node *pos, int value)
{
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL)
    {
        perror("分配内存失败");
        exit(EXIT_FAILURE);
    }
    *new_node = (Node){.value = value, .next = pos->next};
    pos->next = new_node;
    return new_node;
}
void list_reverse(List *list)
{
    Node *prev = NULL;
    Node *cur = list->dummy.next;
    while (cur)
    {
        Node *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    list->dummy.next = prev;
}
void list_print(List *list)
{
    Node *node = list->dummy.next;
    while (node)
    {
        printf("%d ", node->value);
        node = node->next;
    }
    printf("\n");
}
List list_construct(int values[], size_t size)
{
    List list;
    list_init(&list);
    Node *node = &list.dummy;
    for (size_t i = 0; i < size; i++)
        node = list_insert_after(node, values[i]);
    return list;
}
Node *list_at(List *list, size_t index)
{
    Node *node = list->dummy.next;
    for (size_t i = 0; i < index && node; i++)
        node = node->next;
    return node;
}
int list_find(List *list, int value)
{
    Node *node = list->dummy.next;
    int index = 0;
    while (node != NULL)
    {
        if (node->value == value)
            return index;
        index++;
        node = node->next;
    }
    return -1;
}
int list_find_pos(List *list, int value, size_t pos)
{
    Node *node = list_at(list, pos);
    if (node == NULL)
        return -1;
    while (node != NULL)
    {
        if (node->value == value)
            return pos;
        pos++;
        node = node->next;
    }
    return -1;
}
int main()
{
    int values[] = {1, 2, 3, 4, 5, 3, 5};
    List list = list_construct(values, sizeof(values) / sizeof(int));
    list_print(&list);
    Node *ptr = list_at(&list, 4);
    if (ptr != NULL)
        printf("%d\n", ptr->value);
    int pos = list_find(&list, 5);
    printf("%d\n", pos);
    if (pos != -1)
        printf("%d\n", list_find_pos(&list, 5, pos + 1));
    list_reverse(&list);
    list_print(&list);
    list_free(&list);
    return 0;
}