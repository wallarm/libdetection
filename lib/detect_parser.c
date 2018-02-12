#include "detect_int.h"
#include <errno.h>
#include <stdlib.h>

struct detect_parser_info {
    struct detect_parser *parser;
    struct detect_str name;
    RB_ENTRY(detect_parser_info) link;
};

RB_HEAD(detect_parser_tree, detect_parser_info);
#define DETECT_PARSER_INFO2KEY(info) (&(info)->name)
WRB_GENERATE_STATIC(
    detect_parser_tree, detect_parser_info, struct detect_str *,
    link, detect_str_cmp, DETECT_PARSER_INFO2KEY);

static struct detect_parser_tree detect_parsers;

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
    if (WRB_FIND(detect_parser_tree, &detect_parsers, &parser->name) != NULL)
        return (EINVAL);
    if (parser->init != NULL) {
        int rc;

        if (!!(rc = parser->init()))
            return (rc);
    }
    pi = calloc(1, sizeof(*pi));
    pi->parser = parser;
    pi->name = parser->name;
    RB_INSERT(detect_parser_tree, &detect_parsers, pi);
    return (0);
}

int
detect_parser_init(void)
{
    int rc = 0;

    RB_INIT(&detect_parsers);

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
    struct detect_parser_info *pi, *pi_tmp;

    WRB_FOREACH_PDFS(pi, detect_parser_tree, &detect_parsers, pi_tmp) {
        if (pi->parser->deinit != NULL)
            pi->parser->deinit();
        free(pi);
    }
    return (0);
}

struct detect_parser *
detect_parser_find(struct detect_str *name)
{
    struct detect_parser_info *pi;

    if ((pi = WRB_FIND(detect_parser_tree, &detect_parsers, name)) == NULL)
        return (NULL);
    return (pi->parser);
}

void *
detect_parser_list(const struct detect_str **name)
{
    struct detect_parser_info *pi;

    pi = RB_MIN(detect_parser_tree, &detect_parsers);
    if (pi == NULL)
        return (NULL);
    *name = &pi->name;
    return (pi);
}

void *
detect_parser_list_next(void *ctx, const struct detect_str **name)
{
    struct detect_parser_info *pi = ctx;

    pi = RB_NEXT(detect_parser_tree, &detect_parsers, pi);
    if (pi == NULL)
        return (NULL);
    *name = &pi->name;
    return (pi);
}
