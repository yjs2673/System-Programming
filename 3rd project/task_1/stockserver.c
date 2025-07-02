/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"
#include <sys/select.h>

typedef struct item {
    int ID, left_stock, price;
    struct item *left, *right;
} item;

item *arr[100];
int arr_idx = 0;
item *root = NULL;

typedef struct pool {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready, maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;

int byte_cnt = 0;

item* new_item(int ID, int left_stock, int price) {
    item *node = (item*)Malloc(sizeof(item));
    node->ID = ID;
    node->left_stock = left_stock;
    node->price = price;
    node->left = node->right = NULL;
    return node;
}

item* insert_item(item *node, int ID, int left_stock, int price) {
    if (node == NULL) {
		item *NEW = new_item(ID, left_stock, price);
		arr[arr_idx++] = NEW;
		return NEW;
	}
	if (ID < node->ID) node->left = insert_item(node->left, ID, left_stock, price);
    else if (ID > node->ID) node->right = insert_item(node->right, ID, left_stock, price);
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
	for (int i = 0; i < arr_idx; i++) fprintf(fp, "%d %d %d\n", arr[i]->ID, arr[i]->left_stock, arr[i]->price);
	fclose(fp);
}

void show_stock(int connfd, char *buf) {
	for (int i = 0; i < arr_idx; i++) {
		char line[MAXLINE];
		sprintf(line, "%d %d %d\n", arr[i]->ID, arr[i]->left_stock, arr[i]->price);
		strcat(buf, line);
	}
}

item* search_item(item *node, int ID) {
    if (!node) return NULL;
	if (ID < node->ID) return search_item(node->left, ID);
	else if (ID > node->ID) return search_item(node->right, ID);
	return node;
}

void echo(int connfd, char *cmd) {
	char buf[MAXLINE];
    strcpy(buf, "");
    if (!strncmp(cmd, "show", 4)) show_stock(connfd, buf);
    else if (!strncmp(cmd, "buy", 3)) {
        int id, amount;
        sscanf(cmd + 4, "%d %d", &id, &amount);
        item *target = search_item(root, id);
        if (target) {
            if (target->left_stock >= amount) {
                target->left_stock -= amount;
                sprintf(buf, "[buy] success\n");
            } 
			else {
				sprintf(buf, "Not enough left stock\n");
			}
        }
		else {
			sprintf(buf, "[buy] fail: stock ID not found\n");
		}
	}
    else if (!strncmp(cmd, "sell", 4)) {
        int id, amount;
        sscanf(cmd + 5, "%d %d", &id, &amount);
        item *target = search_item(root, id);
        if (target) {
            target->left_stock += amount;
            sprintf(buf, "[sell] success\n");
        } 
		else {
			sprintf(buf, "[sell] fail: stock ID not found\n");
		}
	}
	Rio_writen(connfd, buf, MAXLINE);
}

void init_pool(int listenfd, pool *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) p->clientfd[i] = -1;
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p) {
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd) p->maxfd = connfd;
            if (i > p->maxi) p->maxi = i;
            break;
        }
    }
    if (i == FD_SETSIZE) app_error("add_client error: Too many clients");
}

void check_clients(pool *p) {
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("server received %d bytes\n", n);
				fflush(stdout);
				if (!strcmp(buf, "exit\n")) {
					Close(connfd);
					FD_CLR(connfd, &p->read_set);
					p->clientfd[i] = -1;
					continue;
				}
                echo(connfd, buf);
            }
            else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
	write_stock("stock.txt");
}


int main(int argc, char **argv) {
    int listenfd, *connfdp, maxfd, nready;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
	char client_hostname[MAXLINE], client_port[MAXLINE];
	static pool pool;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(0);
    }

    load_stock("stock.txt");

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
			connfdp = Malloc(sizeof(int));
            *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
			Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
			printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(*connfdp, &pool);
        }
        check_clients(&pool);
    }
}
