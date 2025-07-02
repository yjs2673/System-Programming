#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "hash.h"
#include "bitmap.h"

#define CMD_LEN 256
#define ARG_LEN 32
#define ARG_MAX 10
#define BIT_MAX 10

struct list list_data;
struct bitmap *list_bitmap[BIT_MAX] = {0,};

/***** Get line to get command *****/
void cmd_par (char cmd[CMD_LEN], char arg[][ARG_LEN], int *arc) {
	int i = 0, j = 0;
	*arc = 0;

	while (j < CMD_LEN && i < CMD_LEN) {
		while (cmd[j] != '\0' && cmd[j] != '\n' && cmd[j] != ' ') j = j + 1;
		int length = j - i;
		memcpy (arg[*arc], cmd + i, length);
		arg[(*arc)][length] = '\0';
		(*arc)++;

		if (cmd[j] == ' ') i = ++j;
		else return;
	}
}

/***** Convert string to int *****/
int toInt(char arg[ARG_LEN]) {
	int i, result = 0;
	if (arg[0] == '-') i = 1;
	else i = 0;

	while (i < strlen(arg)) result = (int)(arg[i++] - '0') + result * 10;

	if (arg[0] == '-') return -result;
	else return result;
}

/***** Processing by command *****/
void instruction() {
	while(1) {
		int arc = 0;
		char cmd[CMD_LEN], arg[ARG_MAX][ARG_LEN];
		fgets(cmd, CMD_LEN, stdin);
		cmd_par(cmd, arg, &arc);

		/***** Whole command *****/
		if (strcmp("quit", arg[0]) == 0) break;
		if (strcmp("create", arg[0]) == 0) {
			if (strcmp("list", arg[1]) == 0) {
				struct list *n_list  = (struct list*)malloc(sizeof(struct list));
				struct list_item *n_list_item = (struct list_item*)malloc(sizeof(struct list_item));
				list_init(n_list);
				n_list_item->type = LIST;
				n_list_item->data.list = n_list;
				strcpy(n_list_item->data.list->name, arg[2]);
				list_push_back(&list_data, &(n_list_item->elem));
			}
			if (strcmp("hashtable", arg[1]) == 0) {
				struct hash *n_hash = (struct hash*)malloc(sizeof(struct hash));
				struct list_item *n_list_item = (struct list_item*)malloc(sizeof(struct list_item));
				hash_init(n_hash, hash_hash_int, &hash_less, NULL);
				n_list_item->type = HASH;
				n_list_item->data.hash = n_hash;
				strcpy(n_list_item->data.hash->name, arg[2]);
				list_push_back(&list_data, &(n_list_item->elem));
				// if (list_empty(&list_data)) printf("empty\n");
				// else printf("exits\n");
			}
			if (strcmp("bitmap", arg[1]) == 0) {
				int length = strlen(arg[2]) - 1;
				int index = atoi(&arg[2][length]);
				int count = toInt(arg[3]);
				if (list_bitmap[index] != NULL) printf("input error\n");
				else list_bitmap[index] = bitmap_create((size_t)count);
			}
		}
		if (strcmp("delete", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			if (strncmp(arg[1], "list", length) == 0) {
				struct list_elem *list_elem = find_list_elem(&list_data, arg[1]);
				if (list_elem == NULL) printf("no data\n");
				else {
					struct list_item *list_item = list_entry(list_elem, struct list_item, elem);
					/*if (list_item->type == HASH) {
						struct hash *hash = find_hash(&list_data, arg[1]);
						if (hash == NULL) printf("no hash\n");
						else {
							hash_destroy(hash, &hash_action);
							free(hash);
						}
					}*/
					//else if (list_item->type == LIST) {
						list_remove(list_elem);
						list_delete(list_item->data.list);
						free(list_item);
					//}
				}
			}
			else if (strncmp(arg[1], "hash", length) == 0) {
				struct hash *hash = find_hash(&list_data, arg[1]);
				if (!hash_empty(hash)) {
					hash_destroy(hash, &hash_action);
					free(hash);
				}
			}
			else if (strncmp(arg[1], "bm", length) == 0) {
				int index = atoi(&arg[1][length]);
                struct bitmap *bitmap = list_bitmap[index];
				bitmap_destroy(bitmap);
				list_bitmap[index] = NULL;
			}
		}
		if (strcmp("dumpdata", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			if (strncmp(arg[1], "list", length) == 0) {
				struct list_elem *list_elem = find_list_elem(&list_data, arg[1]);
				//if (list_elem != NULL) {
					struct list_item *list_item = list_entry(list_elem, struct list_item, elem);
					/*if (list_item->type == HASH) {
						struct hash *hash = list_item->data.hash;
						if (!hash_empty(hash)) {
							struct hash_iterator iter;
							hash_first(&iter, hash);
							while (hash_next(&iter)) printf("%d ", hash_cur(&iter)->key);
							printf("\n");
						}
					}*/
					//else if (list_item->type == LIST) {
						struct list *list = list_item->data.list;
						if (!list_empty(list)) {
							for (struct list_elem *c = list_begin(list); c != list_end(list); c = c->next) {
								printf("%d ", list_entry(c, struct list_item, elem)->data.val);
							}
							printf("\n");
						}
					//}
				//}
			}
			else if (strncmp(arg[1], "hash", length) == 0) {
				struct hash *hash = find_hash(&list_data, arg[1]);
				if (!hash_empty(hash)) {
					struct hash_iterator iter;
					hash_first(&iter, hash);
					while (hash_next(&iter)) printf("%d ", hash_cur(&iter)->key);
					printf("\n");
				}
			}
			else if (strncmp(arg[1], "bm", length) == 0) {
				int index = atoi(&arg[1][length]);
				struct bitmap *bitmap = list_bitmap[index];
				size_t s = bitmap_size(bitmap);
				if ((s = bitmap_size(bitmap) > 0)) {
					for (size_t i = 0; i < bitmap_size(bitmap); i++) {
						if (bitmap_test(bitmap, i)) printf("1");
						else printf("0");
					}
					printf("\n");
				}
			}
		}

		/***** List command *****/
		if (strcmp("list_push_back", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list == NULL) printf("input error\n");
			else {
				int key = toInt(arg[2]);
				struct list_item *n_list_item = (struct list_item*)malloc(sizeof(struct list_item));
				n_list_item->data.val = key;
				list_push_back(list, &(n_list_item->elem));
			}
		}
		if (strcmp("list_push_front", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list == NULL) printf("input error\n");
			else {
				int key = toInt(arg[2]);
				struct list_item *n_list_item = (struct list_item*)malloc(sizeof(struct list_item));
				n_list_item->data.val = key;
				list_push_front(list, &(n_list_item->elem));
			}
		}
		if (strcmp("list_back", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if(list == NULL) puts("input error\n");
			else {
				if(list_rbegin(list) != list_rend(list))
					printf("%d\n", list_entry(list_back(list), struct list_item, elem)->data.val);
			}
		}
		if (strcmp("list_front", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if(list == NULL) puts("input error\n");
			else {
				if(list_begin(list) != list_end(list))
					printf("%d\n", list_entry(list_begin(list), struct list_item, elem)->data.val);
			}
		}
		if (strcmp("list_pop_back", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if(list == NULL) puts("input error\n");
			else {
				struct list_elem *p = list_pop_back(list);
				free(list_entry(p, struct list_item, elem));
			}
		}
		if (strcmp("list_pop_front", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if(list == NULL) puts("input error\n");
			else {
				struct list_elem *p = list_pop_front(list);
				free(list_entry(p, struct list_item, elem));
			}
		}
		if (strcmp("list_insert", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				int idx = toInt(arg[2]);
				int key = toInt(arg[3]);
				struct list_elem *b = get_elem(list, idx);
				struct list_item *n_list_item = (struct list_item*)malloc(sizeof(struct list_item));
				n_list_item->data.val = key;
				list_insert(b, &(n_list_item->elem));
			}
		}
		if (strcmp("list_insert_ordered", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				int key = toInt(arg[2]);
				struct list_item *n_list_item = (struct list_item*)malloc(sizeof(struct list_item));
				n_list_item->data.val = key;
				list_insert_ordered(list, &(n_list_item->elem), &list_less, NULL);
			}
		}
		if (strcmp("list_empty", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				if (list_empty(list)) printf("true\n");
				else printf("false\n");
			}
		}
		if (strcmp("list_remove", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				int idx = toInt(arg[2]);
				struct list_elem *t = get_elem(list, idx);
				list_remove(t);
				free(t);
			}
		}
		if (strcmp("list_min", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				struct list_elem *min = list_min(list, &list_less, NULL);
				printf("%d\n", list_entry(min, struct list_item, elem)->data.val);
			}
		}
		if (strcmp("list_max", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				struct list_elem *max = list_max(list, &list_less, NULL);
				printf("%d\n", list_entry(max, struct list_item, elem)->data.val);
			}
		}
		if (strcmp("list_size", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) printf("%zu\n", list_size(list));
		}
		if (strcmp("list_sort", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) list_sort(list, &list_less, NULL);
		}
		if (strcmp("list_splice", arg[0]) == 0) {
			struct list *source = find_list(&list_data, arg[1]);
			struct list *dest = find_list(&list_data, arg[3]);
			if (dest != NULL && source != NULL) {
				struct list_elem *a, *b, *c;
				a = get_elem(source, toInt(arg[2]));
				b = get_elem(dest, toInt(arg[4]));
				c = get_elem(dest, toInt(arg[5]));
				list_splice(a, b, c);
			}
		}
		if (strcmp("list_reverse", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) list_reverse(list);
		}
		if (strcmp("list_unique", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			struct list *d = NULL;
			if (arc == 3) d = find_list(&list_data, arg[2]);
			list_unique(list, d, &list_less, NULL);
		}
		if (strcmp("list_swap", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) {
				int x = toInt(arg[2]), y = toInt(arg[3]);
				struct list_elem *Ex, *Ey;
				Ey = get_elem(list, y);
				Ex = get_elem(list, x);
				list_swap(Ex, Ey);
			}
		}
		if (strcmp("list_shuffle", arg[0]) == 0) {
			struct list *list = find_list(&list_data, arg[1]);
			if (list != NULL) list_shuffle(list);
		}

		/***** Hash command *****/
		if (strcmp("hash_insert", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) {
				int key = toInt(arg[2]);
				struct hash_elem *n_hash_elem = (struct hash_elem*)malloc(sizeof(struct hash_elem));
				n_hash_elem->key = key;
				struct hash_elem *temp = hash_insert(hash, n_hash_elem);
			}
			else printf("input error\n");
		}
		if (strcmp("hash_apply", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) {
				if (strcmp("triple", arg[2]) == 0) hash_apply(hash, &hash_cubed);
				else if (strcmp("square", arg[2]) == 0) hash_apply(hash, &hash_squared);
				else continue;
			}
			else printf("input error\n");
		}
		if (strcmp("hash_delete", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) {
				int key = toInt(arg[2]);
				struct hash_elem source, *t;
				source.key = key;
				t = hash_find(hash, &source);
				if (t != NULL) {
					hash_delete(hash, t);
					free(t);
				}
			}
			else printf("input error\n");
		}
		if (strcmp("hash_size", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) printf("%zu\n", hash_size(hash));
			else printf("input error\n");
		}
		if (strcmp("hash_clear", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) hash_clear(hash, &hash_action);
			else printf("input error\n");
		}
		if (strcmp("hash_empty", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) {
				if (hash_empty(hash)) printf("true\n");
				else printf("false\n");
			}
			else printf("input error\n");
		}
		if (strcmp("hash_find", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) {
				int key = toInt(arg[2]);
				struct hash_elem source, *d;
				source.key = key;
				d = hash_find(hash, &source);
				if (d != NULL) printf("%d\n", d->key);
			}
			else printf("input error\n");
		}
		if (strcmp("hash_replace", arg[0]) == 0) {
			struct hash *hash = find_hash(&list_data, arg[1]);
			if (hash != NULL) {
				int key = toInt(arg[2]);
				struct hash_elem *n_hash_elem = (struct hash_elem*)malloc(sizeof(struct hash_elem));
				n_hash_elem->key = key;
				struct hash_elem *o = hash_replace(hash, n_hash_elem);
				if (o != NULL) free(o);
			}
			else printf("input error\n");
		}

		/***** Bitmap command *****/
		if (strcmp("bitmap_mark", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			int key = toInt(arg[2]);
			bitmap_mark(b, (size_t)key);
		}
		if (strcmp("bitmap_reset", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			int key = toInt(arg[2]);
			bitmap_reset(b, (size_t)key);
		}
		if (strcmp("bitmap_all", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if (bitmap_all(b, start, count)) printf("true\n");
			else printf("false\n");
		}
		if (strcmp("bitmap_any", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if (bitmap_any(b, start, count)) printf("true\n");
			else printf("false\n");
		}
		if (strcmp("bitmap_contains", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if (strcmp("true", arg[4]) == 0) {
				if (bitmap_contains(b, start, count, true)) printf("true\n");
				else printf("false\n");
			}
			else if (strcmp("false", arg[4]) == 0) {
				if (bitmap_contains(b, start, count, false)) printf("true\n");
				else printf("false\n");
			}
		}
		if (strcmp("bitmap_none", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if (bitmap_none(b, start, count)) printf("true\n");
			else printf("false\n");
		}
		if (strcmp("bitmap_set", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			int idx = toInt(arg[2]);
			if (!strcmp(arg[3], "true")) bitmap_set(b, idx, true);
			else if (!strcmp(arg[3], "false")) bitmap_set(b, idx, false);
		}
		if (strcmp("bitmap_set_all", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			if (!strcmp(arg[2], "true")) bitmap_set_all(b, true);
			else if (!strcmp(arg[2], "false")) bitmap_set_all(b, false);
		}
		if (strcmp("bitmap_set_multiple", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if (strcmp("true", arg[4]) == 0) bitmap_set_multiple(b, start, count, true);
			else if (strcmp("false", arg[4]) == 0) bitmap_set_multiple(b, start, count, false);
		}
		if (strcmp("bitmap_flip", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			int key = toInt(arg[2]);
			bitmap_flip(b, (size_t)key);
		}
		if (strcmp("bitmap_count", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if(!strcmp(arg[4], "true")) printf("%zu\n", bitmap_count(b, start, count, true));
			else if(!strcmp(arg[4], "false")) printf("%zu\n", bitmap_count(b, start, count, false));
		}
		if (strcmp("bitmap_dump", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			bitmap_dump(b);
		}
		if (strcmp("bitmap_scan", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if(!strcmp(arg[4], "true")) printf("%zu\n", bitmap_scan(b, start, count, true));
			else if(!strcmp(arg[4], "false")) printf("%zu\n", bitmap_scan(b, start, count, false));
		}
		if (strcmp("bitmap_scan_and_flip", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			size_t start = (size_t)toInt(arg[2]);
			size_t count = (size_t)toInt(arg[3]);
			if(!strcmp(arg[4], "true")) printf("%zu\n", bitmap_scan_and_flip(b, start, count, true));
			else if(!strcmp(arg[4], "false")) printf("%zu\n", bitmap_scan_and_flip(b, start, count, false));
		}
		if (strcmp("bitmap_size", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			printf("%zu\n", bitmap_size(b));
		}
		if (strcmp("bitmap_test", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			int idx = toInt(arg[2]);
			if (bitmap_test(b, idx)) printf("true\n");
			else printf("false\n");
		}
		if (strcmp("bitmap_expand", arg[0]) == 0) {
			int length = strlen(arg[1]) - 1;
			int index = atoi(&arg[1][length]);
			struct bitmap *b = list_bitmap[index];
			int size = toInt(arg[2]);
			bitmap_expand(b, size);
		}
	}
}

int main() {
	list_init(&list_data);
	instruction();
	return 0;
}
