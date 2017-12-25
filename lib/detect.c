#include "detect_int.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static bool detect_initialized = false;

static int
s_detect_deinit(void)
{
    detect_parser_deinit();
    return (0);
}

int
detect_init(void)
{
    int rc;

    if (detect_initialized)
        return (0);

    if (!!(rc = detect_parser_init())) {
        s_detect_deinit();
        return (rc);
    }
    detect_initialized = true;
    return (0);
}

int
detect_deinit(void)
{
    if (!detect_initialized)
        return (0);
    s_detect_deinit();
    detect_initialized = false;
    return (0);
}

struct detect *
detect_open(const char *parser_name)
{
    struct detect_str name = {
        .str = (char *)parser_name,
        .len = strlen(parser_name)
    };
    struct detect_parser *parser;

    if ((parser = detect_parser_find(&name)) == NULL)
        return (NULL);

    return (parser->open(parser));
}

const struct detect_str *
detect_name(struct detect *detect)
{
    return (&detect->parser->name);
}

int
detect_close(struct detect *detect)
{
    if (detect->started)
        detect_stop(detect);
    return (detect->parser->close(detect));
}

int
detect_set_options(struct detect *detect, const char *options)
{
    if (detect->started)
        return (EINVAL);

    if (detect->parser->set_options != NULL)
        return (detect->parser->set_options(detect, options));
    else
        return (0);
}

int
detect_set_finish_cb(struct detect *detect, detect_finish_cb cb, void *arg)
{
    detect->finish_cb = cb;
    detect->finish_cb_arg = arg;

    return (0);
}

int
detect_start(struct detect *detect)
{
    unsigned i;
    int rc;

    if (detect->started)
        return (EINVAL);

    for (i = 0; i < detect->nctx; i++)
        detect->ctxs[i]->res->finished = detect->ctxs[i]->res->disabled;
    if (!!(rc = detect->parser->start(detect)))
        return (rc);

    detect->started = true;
    return (0);
}

int
detect_stop(struct detect *detect)
{
    int rc;

    if (!detect->started)
        return (EINVAL);

    if (!!(rc = detect->parser->stop(detect)))
        return (rc);

    detect->started = false;
    return (0);
}

unsigned
detect_get_nctx(struct detect *detect)
{
    return (detect->nctx);
}

const struct detect_ctx_desc *
detect_ctx_get_desc(struct detect *detect, unsigned ctxnum)
{
    if (ctxnum >= detect->nctx) {
        errno = EINVAL;
        return (NULL);
    }
    return (detect->ctxs[ctxnum]->desc);
}

int
detect_ctx_disable(struct detect *detect, unsigned ctxnum)
{
    if (detect->started && ctxnum >= detect->nctx)
        return (EINVAL);
    detect->ctxs[ctxnum]->res->disabled = true;
    return (0);
}

const struct detect_ctx_result *
detect_ctx_get_result(struct detect *detect, unsigned ctxnum)
{
    if (ctxnum >= detect->nctx) {
        errno = EINVAL;
        return (NULL);
    }
    return (detect->ctxs[ctxnum]->res);
}

int
detect_add_data(struct detect *detect, const void *data, size_t siz, bool fin)
{
    if (!detect->started)
        return (EINVAL);
    if (!siz && !fin)
        return (0);
    return (detect->parser->add_data(detect, data, siz, fin));
}

int
detect_instance_init(struct detect *detect, struct detect_parser *parser)
{
    memset(detect, 0, sizeof(*detect));
    detect->parser = parser;

    return (0);
}

static void
s_detect_flag_stat_free(void *p, void *user)
{
    struct detect_flag_stat *fs = container_of(p, typeof(*fs), flag_name);

    avl_tree_purge(&fs->stat_by_tokens);
    free(fs);
}

static void
s_detect_flag_token_free(void *p, void *user)
{
    struct detect_token_stat *ts = container_of(p, typeof(*ts), token_name);

    free(ts);
}

int
detect_ctx_result_init(struct detect_ctx_result *res)
{
    memset(res, 0, sizeof(*res));
    avl_tree_init(&res->stat_by_flags, detect_str_cmp, s_detect_flag_stat_free);
    res->stat_by_flags.allocator =
        (typeof(res->stat_by_flags.allocator))&avl_allocator_0;
    STAILQ_INIT(&res->datas);
    return (0);
}

int
detect_ctx_result_deinit(struct detect_ctx_result *res)
{
    struct detect_data *data;

    avl_tree_purge(&res->stat_by_flags);
    while ((data = STAILQ_FIRST(&res->datas)) != NULL) {
        STAILQ_REMOVE_HEAD(&res->datas, link);
        free(data->value.str);
        free(data);
    }
    res->finished = 0;
    res->parse_error = 0;
    return (0);
}

int
detect_ctx_result_store_token(
    struct detect_ctx_result *res, const struct detect_str *flag,
    const struct detect_str *name)
{
    avl_node_t *link;
    struct detect_flag_stat *fs;
    struct detect_token_stat *ts;

    if ((link = avl_search(&res->stat_by_flags, flag)) != NULL) {
        fs = container_of(link, typeof(*fs), link);
        fs->count++;
    } else {
        fs = calloc(1, sizeof(*fs) + flag->len + 1);

        fs->flag_name.str = (void *)(fs + 1);
        memcpy(fs->flag_name.str, flag->str, flag->len);
        fs->flag_name.len = flag->len;
        fs->flag_name.str[fs->flag_name.len] = 0;
        fs->count = 1;
        avl_tree_init(&fs->stat_by_tokens, detect_str_cmp,
                      s_detect_flag_token_free);
        fs->stat_by_tokens.allocator =
            (typeof(fs->stat_by_tokens.allocator))&avl_allocator_0;
        avl_node_init(&fs->link, &fs->flag_name);
        avl_insert(&res->stat_by_flags, &fs->link);
    }
    if ((link = avl_search(&fs->stat_by_tokens, name)) != NULL) {
        ts = container_of(link, typeof(*ts), link);
        ts->count++;
    } else {
        ts = calloc(1, sizeof(*ts) + name->len + 1);

        ts->token_name.str = (void *)(ts + 1);
        memcpy(ts->token_name.str, name->str, name->len);
        ts->token_name.len = name->len;
        ts->token_name.str[ts->token_name.len] = 0;
        ts->count++;
        avl_node_init(&ts->link, &ts->token_name);
        avl_insert(&fs->stat_by_tokens, &ts->link);
    }
    return (0);
}

int
detect_ctx_result_store_data(
    struct detect_ctx_result *res, const struct detect_str *kind,
    struct detect_str *value)
{
    struct detect_data *dd;

    dd = malloc(sizeof(*dd) + kind->len + 1);
    dd->kind.str = (void *)(dd + 1);
    memcpy(dd->kind.str, kind->str, kind->len);
    dd->kind.len = kind->len;
    dd->kind.str[dd->kind.len] = 0;
    dd->value = *value;

    STAILQ_INSERT_TAIL(&res->datas, dd, link);

    return (0);
}

bool
detect_ctx_has_attack(struct detect *detect, unsigned ctxnum)
{
    struct detect_ctx *ctx;

    if (ctxnum >= detect->nctx)
        return (false);
    ctx = detect->ctxs[ctxnum];
    if (ctx->res->disabled || !ctx->res->finished)
        return (false);
    if (ctx->res->parse_error)
        return (false);
    if (ctx->desc->rce)
        return (true);
    /*
     * Injection. At least one instruction should present.
     * Just test if instruction tree is not empty.
     */
    if (ctx->res->stat_by_flags.head != NULL)
        return (true);
    return (false);
}

bool
detect_has_attack(struct detect *detect, uint32_t *attack_types)
{
    unsigned i;

    *attack_types = 0;
    for (i = 0; i < detect->nctx; i++) {
        struct detect_ctx *ctx;

        if (!detect_ctx_has_attack(detect, i))
            continue;

        ctx = detect->ctxs[i];
        if (ctx->desc->rce)
            (*attack_types) |= DETECT_ATTACK_RCE;
        else
            (*attack_types) |= DETECT_ATTACK_INJ;
    }

    return (!!(*attack_types));
}
