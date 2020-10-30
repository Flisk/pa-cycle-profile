#include <stdlib.h>

struct strlist {
        struct strlist_node *head;
        struct strlist_node *tail;
};

struct strlist_node {
        const char		*s;
        struct strlist_node	*next;
};

static int strlist_append(struct strlist *list, const char *s)
{
        if (list->tail == NULL) {
                list->head = malloc(sizeof(struct strlist_node));

                if (list->head == NULL) {
                        return -1;
                }

                list->tail = list->head;
                list->head->s = s;

                return 0;
        } else {
                struct strlist_node *tail = list->tail;

                tail->next = malloc(sizeof(struct strlist_node));

                if (tail->next == NULL) {
                        return -1;
                }

                list->tail = tail->next;
                tail->next->s = s;

                return 0;
        }
}
