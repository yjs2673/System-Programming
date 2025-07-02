/*
 * echoclient_threadpool.c - An echo server with thread pool
 */
#include "csapp.h"

#define NWORKERS 4
#define QUEUE_SIZE 16

typedef struct item {
    int ID, left_stock, price, readcnt;
    sem_t mutex, w;
    struct item *left, *right;
} item;

item *arr[20];
int arr_idx = 0;
item *root = NULL;
sem_t file_mutex;

int conn_queue[QUEUE_SIZE];
int queue_front = 0, queue_rear = 0;
sem_t mutex_queue, slots, items;

void queue_init() {
    queue_front = queue_rear = 0;
    Sem_init(&mutex_queue, 0, 1);
    Sem_init(&slots, 0, QUEUE_SIZE);
    Sem_init(&items, 0, 0);
}

void enqueue(int connfd) {
    P(&slots);
    P(&mutex_queue);
    conn_queue[queue_rear] = connfd;
    queue_rear = (queue_rear + 1) % QUEUE_SIZE;
    V(&mutex_queue);
    V(&items);
}

int dequeue() {
    int connfd;
    P(&items);
    P(&mutex_queue);
    connfd = conn_queue[queue_front];
    queue_front = (queue_front + 1) % QUEUE_SIZE;
    V(&mutex_queue);
    V(&slots);
    return connfd;
}

item* new_item(int ID, int left_stock, int price) {
    item *node = (item*)Malloc(sizeof(item));
    node->ID = ID;
    node->left_stock = left_stock;
    node->price = price;
    node->readcnt = 0;
    node->left = node->right = NULL;
    Sem_init(&node->mutex, 0, 1);
    Sem_init(&node->w, 0, 1);
    return node;
}

item* insert_item(item *node, int ID, int left_stock, int price) {
    if (node == NULL) {
        item *NEW = new_item(ID, left_stock, price);
        arr[arr_idx++] = NEW;
        return NEW;
    }
    if (ID < node->ID)
        node->left  = insert_item(node->left,  ID, left_stock, price);
    else if (ID > node->ID)
        node->right = insert_item(node->right, ID, left_stock, price);
    return node;
}

void load_stock(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    int id, left, price;
    while (fscanf(fp, "%d %d %d", &id, &left, &price) != EOF) {
        root = insert_item(root, id, left, price);
    }
    fclose(fp);
}

void write_stock(char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    for (int i = 0; i < arr_idx; i++) {
        item *it = arr[i];

        // Writer lock
        P(&it->w);

        // 쓰기 작업
        fprintf(fp, "%d %d %d\n", it->ID, it->left_stock, it->price);

        // Unlock
        V(&it->w);
    }
    fclose(fp);
}

void show_stock(int connfd, char *buf) {
    for (int i = 0; i < arr_idx; i++) {
        item *it = arr[i];

        // Start reader
        P(&it->mutex);
        it->readcnt++;
        if (it->readcnt == 1)
            P(&it->w); // 첫 번째 reader는 writer 차단
        V(&it->mutex);

        // 읽기 작업
        char line[MAXLINE];
        sprintf(line, "%d %d %d\n", it->ID, it->left_stock, it->price);
        strcat(buf, line);

        // End reader
        P(&it->mutex);
        it->readcnt--;
        if (it->readcnt == 0)
            V(&it->w); // 마지막 reader가 나가면 writer 허용
        V(&it->mutex);
    }
}

item* search_item(item *node, int ID) {
    if (!node) return NULL;
    if (ID < node->ID) return search_item(node->left,  ID);
    if (ID > node->ID) return search_item(node->right, ID);
    return node;
}

void echo(int connfd, char *cmd) {
    char buf[MAXLINE] = "";
    if (!strncmp(cmd, "show", 4)) show_stock(connfd, buf);
    else if (!strncmp(cmd, "buy", 3)) {
        int id, amount;
        sscanf(cmd + 4, "%d %d", &id, &amount);
        item *target = search_item(root, id);
        if (target) {
            P(&target->mutex);
            if (target->left_stock >= amount) {
                target->left_stock -= amount;
                sprintf(buf, "[buy] success\n");
            } 
			else sprintf(buf, "Not enough left stock\n");
            V(&target->mutex);
        } 
		else sprintf(buf, "[buy] fail: stock ID not found\n");
    }
    else if (!strncmp(cmd, "sell", 4)) {
        int id, amount;
        sscanf(cmd + 5, "%d %d", &id, &amount);
        item *target = search_item(root, id);
        if (target) {
            P(&target->mutex);
            target->left_stock += amount;
            sprintf(buf, "[sell] success\n");
            V(&target->mutex);
        } 
		else sprintf(buf, "[sell] fail: stock ID not found\n");
    }
    Rio_writen(connfd, buf, MAXLINE);
}

void *worker(void *vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = dequeue();
        char buf[MAXLINE];
        rio_t rio;
        int n;
        Rio_readinitb(&rio, connfd);
        while ((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
            printf("server received %d bytes\n", n);
            fflush(stdout);
            if (!strcmp(buf, "exit\n")) break;
            echo(connfd, buf);
        }
        write_stock("stock.txt");
        Close(connfd);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int listenfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    Sem_init(&file_mutex, 0, 1);
    load_stock("stock.txt");

    queue_init();

    for (int i = 0; i < NWORKERS; i++) {
        Pthread_create(&tid, NULL, worker, NULL);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen,
                    client_hostname, MAXLINE,
                    client_port,     MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        enqueue(connfd);
    }

    return 0;
}
