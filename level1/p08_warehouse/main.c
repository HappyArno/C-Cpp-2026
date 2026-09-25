#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct
{
    char *name;
    char *model;
    size_t quantity;
} Item;
void item_free(Item *item)
{
    free(item->name);
    free(item->model);
}
#define INVENTORY_DEFAULT_CAPACITY 8
typedef struct
{
    size_t size;
    size_t capacity;
    Item *data;
} Inventory;
void inventory_init(Inventory *inventory)
{
    inventory->size = 0;
    inventory->capacity = 0;
    inventory->data = NULL;
}
void inventory_free(Inventory *inventory)
{
    for (size_t i = 0; i < inventory->size; i++)
        item_free(&(inventory->data[i]));
    free(inventory->data); // NULL is OK
    inventory->data = NULL;
    inventory->capacity = 0;
    inventory->size = 0;
}
void inventory_reserve(Inventory *inventory, size_t capacity)
{
    if (capacity <= inventory->capacity)
        return;
    Item *new_data = realloc(inventory->data, sizeof(Item) * capacity); // When data is NULL, realloc will act as if malloc
    if (!new_data)
    {
        perror("分配内存失败");
        exit(EXIT_FAILURE);
    }
    inventory->data = new_data;
    inventory->capacity = capacity;
}
void inventory_shrink_to_fit(Inventory *inventory)
{
    if (inventory->size >= inventory->capacity)
        return;
    if (inventory->size == 0)
    {
        free(inventory->data);
        inventory->data = NULL;
        inventory->capacity = 0;
        return;
    }
    Item *new_data = realloc(inventory->data, sizeof(Item) * inventory->size);
    if (!new_data)
    {
        perror("分配内存失败");
        exit(EXIT_FAILURE);
    }
    inventory->data = new_data;
    inventory->capacity = inventory->size;
}
void inventory_push_back(Inventory *inventory, Item item)
{
    if (inventory->size >= inventory->capacity)
    {
        size_t new_capacity;
        if (inventory->capacity == 0)
            new_capacity = INVENTORY_DEFAULT_CAPACITY;
        else if (inventory->capacity > SIZE_MAX / 2)
        {
            fprintf(stderr, "Inventory容量增长时发生溢出\n");
            exit(EXIT_FAILURE);
        }
        else
            new_capacity = inventory->capacity * 2;
        inventory_reserve(inventory, new_capacity);
    }
    inventory->data[inventory->size] = item;
    inventory->size++;
}
void inventory_erase(Inventory *inventory, size_t index)
{
    if (index >= inventory->size)
        return;
    item_free(&(inventory->data[index]));
    for (size_t i = index + 1; i < inventory->size; i++)
        inventory->data[i - 1] = inventory->data[i];
    inventory->size--;
}
Item *inventory_at(Inventory *inventory, size_t index)
{
    if (index < inventory->size)
        return &(inventory->data[index]);
    else
        return NULL;
}
Inventory load_inventory(FILE *fp)
{
    Inventory inventory;
    inventory_init(&inventory);
    for (;;)
    {
        Item item;
        int count = fscanf(fp, "%ms %ms %zu", &item.name, &item.model, &item.quantity);
        if (count != 3)
        {
            if (count >= 1)
                free(item.name);
            if (count >= 2)
                free(item.model);
            break;
        }
        inventory_push_back(&inventory, item);
    }
    return inventory;
}
void save_inventory(FILE *fp, Inventory *inventory)
{
    for (size_t i = 0; i < inventory->size; i++)
        fprintf(fp, "%s %s %zu\n", inventory->data[i].name, inventory->data[i].model, inventory->data[i].quantity);
}
void stock_in(Inventory *inventory, Item item)
{
    for (size_t i = 0; i < inventory->size; i++)
        if (strcmp(inventory->data[i].name, item.name) == 0 && strcmp(inventory->data[i].model, item.model) == 0)
        {
            inventory->data[i].quantity += item.quantity;
            item_free(&item);
            return;
        }
    inventory_push_back(inventory, item);
}
void stock_out(Inventory *inventory, Item item)
{
    for (size_t i = 0; i < inventory->size; i++)
        if (strcmp(inventory->data[i].name, item.name) == 0 && strcmp(inventory->data[i].model, item.model) == 0)
        {
            if (inventory->data[i].quantity == item.quantity)
                inventory_erase(inventory, i);
            else if (inventory->data[i].quantity >= item.quantity)
                inventory->data[i].quantity -= item.quantity;
            else
                printf("出库失败: 该货物数量不足\n");
            item_free(&item);
            return;
        }
    item_free(&item);
    printf("出库失败: 未找到该货物\n");
}
void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}
void menu(Inventory *inventory)
{
    printf("[1] 显示存货列表\n");
    printf("[2] 入库\n");
    printf("[3] 出库\n");
    printf("[4] 退出程序\n");
    for (;;)
    {
        int choice;
        printf("请输入对应数字选择功能: ");
        if (scanf("%d", &choice) != 1)
        {
            clear_input_buffer();
            printf("输入非法\n");
            continue;
        }
        Item item;
        int count;
        switch (choice)
        {
        case 1:
            printf("%-15s %-15s %-15s\n", "Name", "Model", "Quantity");
            for (size_t i = 0; i < inventory->size; i++)
                printf("%-15s %-15s %-15zu\n", inventory->data[i].name, inventory->data[i].model, inventory->data[i].quantity);
            break;
        case 2:
            printf("请输入名称，型号和数量: ");
            count = scanf("%ms %ms %zu", &item.name, &item.model, &item.quantity);
            if (count != 3)
            {
                clear_input_buffer();
                printf("输入非法\n");
                if (count >= 1)
                    free(item.name);
                if (count >= 2)
                    free(item.model);
                continue;
            }
            stock_in(inventory, item);
            break;
        case 3:
            printf("请输入名称，型号和数量: ");
            count = scanf("%ms %ms %zu", &item.name, &item.model, &item.quantity);
            if (count != 3)
            {
                clear_input_buffer();
                printf("输入非法\n");
                if (count >= 1)
                    free(item.name);
                if (count >= 2)
                    free(item.model);
                continue;
            }
            stock_out(inventory, item);
            break;
        case 4:
            return;
            break;
        default:
            printf("输入的数字没有对应的功能\n");
            break;
        }
    }
}
int main()
{
    FILE *fp = fopen("inventory.txt", "r");
    if (fp == NULL)
    {
        perror("打开库存文件失败");
        exit(EXIT_FAILURE);
    }
    Inventory inventory = load_inventory(fp);
    if (fclose(fp) == EOF)
    {
        perror("关闭库存文件失败");
        exit(EXIT_FAILURE);
    }
    menu(&inventory);
    fp = fopen("inventory.txt", "w");
    if (fp == NULL)
    {
        perror("打开库存文件失败");
        exit(EXIT_FAILURE);
    }
    save_inventory(fp, &inventory);
    if (fclose(fp) == EOF)
    {
        perror("关闭库存文件失败");
        exit(EXIT_FAILURE);
    }
    inventory_free(&inventory);
    return 0;
}