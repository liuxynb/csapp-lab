/*这段文本是C++中的预处理指令，
用于防止头文件重复包含。
`#ifndef`是预处理指令的一种，
意思是“如果没有定义过__CACHE__，
则执行下面的代码”，
而`#define`则是定义一个宏，
即将__CACHE__定义为一个空值。
这样，在后续的代码中，
如果再次遇到`#ifndef __CACHE__`，
由于__CACHE__已经被定义过了，
所以就不会执行下面的代码了，
从而避免了头文件的重复包含。
这种技术被称为“头文件保护”或“宏定义保护”。*/
#ifndef __CACHE__
#define __CACHE__
#include <stdio.h>
#include "csapp.h"

// 双向链表的节点
typedef struct Node
{
    char *url;
    char *data;
    struct Node *prev;
    struct Node *next;
    int size;
} Node;

typedef struct Cache
{
    int max_cache_size;
    int max_object_size;
    int used_size;
    struct Node *head;
    struct Node *tail;
} Cache;

/* The fuction for Node*/
void init_node(Node *node);

void init_node_with_data(Node *node, char *url, char *data, int n);

void free_node(Node *node);

void link_node(Node *node1, Node *node2);

void change_link(Node *node);

void insert_node(Node *node1, Node *node2);

void print_node(Node *node);

/*The function for Cache*/
void init_cache(Cache *cache, int max_cache_size, int max_object_size);

Node *find_cache(Cache *cache, char *url);

void free_cache_block(Cache *cache);

void free_cache(Cache *cache);

int insert_cache(Cache *cache, char *url, char *data);

void print_cache(Cache *cache);

int reader(Cache *cache, char *url, int fd);

void writer(Cache *cache, char *uil, char *data);

#endif