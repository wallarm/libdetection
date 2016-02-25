#include "detect_int.h"
#include <errno.h>
#include <stdlib.h>

static avl_tree_t detect_parsers;

struct detect_parser_info {
    struct detect_parser *parser;
    struct detect_str name;
    avl_node_t link;
};

#define TRYLOAD(rc, parser)                             \
    do {                                                \
        extern struct detect_parser parser;             \
                                                        \
        if (!!(rc = s_detect_parser_load(&parser)))     \
            goto done;                                  \
    } while (0)

static int
s_detect_parser_load(struct detect_parser *parser)
{
    struct detect_parser_info *pi;

    if (parser->name.str == NULL)
        return (EINVAL);
    if (avl_search(&detect_parsers, &parser->name) != NULL)
        return (EINVAL);
    if (parser->init != NULL) {
        int rc;

        if (!!(rc = parser->init()))
            return (rc);
    }
    pi = calloc(1, sizeof(*pi));
    pi->parser = parser;
    pi->name = parser->name;
    avl_node_init(&pi->link, &pi->name);
    avl_insert(&detect_parsers, &pi->link);
    return (0);
}

static void
s_detect_parser_free(void *p, void *user)
{
    struct detect_parser_info *pi = container_of(p, typeof(*pi), name);

    if (pi->parser->deinit != NULL)
        pi->parser->deinit();
    free(pi);
}

int
detect_parser_init(void)
{
    int rc = 0;

    avl_tree_init(&detect_parsers, detect_str_cmp_fast,
                  s_detect_parser_free);
    detect_parsers.allocator = (avl_allocator_t *)&avl_allocator_0;

    TRYLOAD(rc, detect_parser_sqli);
    TRYLOAD(rc, detect_parser_pt);
  done:
    if (rc) {
        detect_parser_deinit();
    }
    return (rc);
}

int
detect_parser_deinit(void)
{
    avl_tree_purge(&detect_parsers);
    return (0);
}

struct detect_parser *
detect_parser_find(struct detect_str *name)
{
    avl_node_t *link;

    if ((link = avl_search(&detect_parsers, name)) == NULL)
        return (NULL);
    return (container_of(link, struct detect_parser_info, link)->parser);
}

void *
detect_parser_list(const struct detect_str **name)
{
    struct detect_parser_info *pi;

    if (detect_parsers.head == NULL)
        return (NULL);
    pi = container_of(detect_parsers.head->item, typeof(*pi), name);
    *name = &pi->name;
    return (pi);
}

void *
detect_parser_list_next(void *ctx, const struct detect_str **name)
{
    struct detect_parser_info *pi = ctx;

    if (pi->link.next == NULL)
        return (NULL);
    pi = container_of(pi->link.next->item, typeof(*pi), name);
    *name = &pi->name;
    return (pi);
}
