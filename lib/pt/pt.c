#include "pt.h"
#include <string.h>
#include <assert.h>

static const struct {
    struct detect_ctx_desc desc;
    enum pt_parser_tokentype start_tok;
} pt_ctxs[] = {
    // clang-format off
    [PT_CTX_INJECTION] = {
        .desc = {.name = {CSTR_LEN("inj")}},
        .start_tok = TOK_START_PT_INJ,
    },
};
// clang-format on

static struct detect *
detect_pt_open(struct detect_parser *parser)
{
    struct detect *detect;
    unsigned i;

    detect = malloc(sizeof(*detect));
    detect_instance_init(detect, parser);
    detect->nctx = 1;
    detect->ctxs = malloc(detect->nctx * sizeof(*detect->ctxs));

    for (i = 0; i < detect->nctx; i++) {
        struct pt_detect_ctx *ctx;

        ctx = calloc(1, sizeof(*ctx));
        ctx->base.desc = (struct detect_ctx_desc *)&pt_ctxs[i].desc;
        ctx->base.res = &ctx->res;
        detect_ctx_result_init(ctx->base.res);
        ctx->type = i;
        ctx->ctxnum = i;
        ctx->detect = detect;
        detect->ctxs[i] = (void *)ctx;
    }
    return (detect);
}

static int
detect_pt_close(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct pt_detect_ctx *ctx = (void *)detect->ctxs[i];

        detect_ctx_result_deinit(ctx->base.res);
        if (ctx->pstate != NULL)
            pt_parser_pstate_delete(ctx->pstate);
        free(ctx);
    }
    if (detect->ctxs != NULL)
        free(detect->ctxs);
    free(detect);

    return (0);
}

static void
pt_lexer_init(struct pt_detect_lexer_ctx *lexer)
{
    memset(lexer, 0, sizeof(*lexer));
    detect_re2c_init(&lexer->re2c);
    lexer->state = -1;
}

static void
pt_lexer_deinit(struct pt_detect_lexer_ctx *lexer)
{
    detect_buf_deinit(&lexer->buf);
    detect_re2c_deinit(&lexer->re2c);
}

void
pt_token_data_destructor(void *token)
{
    struct pt_token_arg_data *data = token;

    if (!!(data->flags & PT_VALUE_NEEDFREE) && data->value.str != NULL)
        free(data->value.str);
}

static int
detect_pt_push_token(struct pt_detect_ctx *ctx, int tok, void *tok_val)
{
    int rv;

    if (ctx->res.finished)
        return (0);

    rv = pt_parser_push_parse(ctx->pstate, tok, tok_val, ctx);
    if (rv == YYPUSH_MORE)
        rv = 0;
    else {
        ctx->res.finished = true;
        ctx->detect->nctx_finished++;
        /*
         * On parse error, ctx->res.parse_error is already set by
         * pt_parser_error().
         */
        if (ctx->detect->finish_cb != NULL)
            rv = ctx->detect->finish_cb(
                ctx->detect, ctx->ctxnum, ctx->detect->nctx - ctx->detect->nctx_finished,
                ctx->detect->finish_cb_arg);
        else
            rv = 0;
    }
    return (rv);
}

static int
detect_pt_start(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct pt_detect_ctx *ctx = (void *)detect->ctxs[i];

        if (ctx->res.finished)
            continue;
        ctx->pstate = pt_parser_pstate_new();
        pt_lexer_init(&ctx->lexer);
        if (detect_pt_push_token(ctx, pt_ctxs[ctx->type].start_tok, NULL) != 0)
            break;
    }
    return (0);
}

static int
detect_pt_stop(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct pt_detect_ctx *ctx = (void *)detect->ctxs[i];

        if (ctx->pstate == NULL)
            continue;

        /* We have to finish bison state machine or memleaks will occur*/
        if (!ctx->res.finished)
            detect_pt_push_token(ctx, 0, NULL);

        pt_parser_pstate_delete(ctx->pstate);
        ctx->pstate = NULL;
        pt_lexer_deinit(&ctx->lexer);
        detect_ctx_result_deinit(&ctx->res);
    }
    return (0);
}

static int
pt_lexer_add_data(struct pt_detect_ctx *ctx, const void *data, size_t siz, bool fin)
{
    return (detect_re2c_add_data(&ctx->lexer.re2c, data, siz, fin));
}

static int
detect_pt_add_data(struct detect *detect, const void *data, size_t siz, bool fin)
{
    unsigned i;
    union PT_PARSER_STYPE token_arg;
    int rv = 0;

    for (i = 0; i < detect->nctx; i++) {
        struct pt_detect_ctx *ctx = (void *)detect->ctxs[i];
        int token;

        if (ctx->res.finished)
            continue;
        pt_lexer_add_data(ctx, data, siz, fin);
        do {
            memset(&token_arg, 0, sizeof(token_arg));
            token = pt_get_token(ctx, &token_arg);

            if (token >= 0) {
                if ((rv = detect_pt_push_token(ctx, token, &token_arg)) != 0)
                    goto done;
            } else if (token != -EAGAIN) {
                /* fatal error */
                rv = -token;
                goto done;
            }
        } while (!ctx->res.finished && token != -EAGAIN);
    }

done:
    return (rv);
}

static int
pt_store_key(struct pt_detect_ctx *ctx, struct pt_token_arg_data *info)
{
    const static struct {
        struct detect_str name;
        uint32_t flag;
    } flagnames[] = {
        {.flag = PT_KEY_INSTR, .name = {CSTR_LEN("INSTR")}},
    };
    unsigned i;

    for (i = 0; i < sizeof(flagnames) / sizeof(flagnames[0]); i++) {
        if (!(info->flags & flagnames[i].flag))
            continue;
        switch (info->tok) {
        case TOK_SEP:
            detect_ctx_result_store_token(
                &ctx->res, &flagnames[i].name, &(struct detect_str){CSTR_LEN("sep")});
            break;
        case TOK_TRAV:
            detect_ctx_result_store_token(
                &ctx->res, &flagnames[i].name, &(struct detect_str){CSTR_LEN("trav")});
            break;
        default:
            break;
        }
    }
    pt_token_data_destructor(info);
    return (0);
}

int
pt_store_data(struct pt_detect_ctx *ctx, struct pt_token_arg_data *info)
{
    switch (info->tok) {
    case TOK_NAME:
        assert(!!(info->flags & PT_VALUE_NEEDFREE));
        detect_ctx_result_store_data(
            &ctx->res, &(struct detect_str){CSTR_LEN("NAME")}, &info->value);
        return (0);
    case TOK_ROOT: {
        char *copy;

        assert(!(info->flags & PT_VALUE_NEEDFREE));

        copy = malloc(info->value.len);
        memcpy(copy, info->value.str, info->value.len);
        info->value.str = copy;

        detect_ctx_result_store_data(
            &ctx->res, &(struct detect_str){CSTR_LEN("NAME")}, &info->value);
        return (0);
    }
    default:
        return (pt_store_key(ctx, info));
    }
}

struct detect_parser detect_parser_pt = {
    .name = {CSTR_LEN("pt")},
    .open = detect_pt_open,
    .close = detect_pt_close,
    .start = detect_pt_start,
    .stop = detect_pt_stop,
    .add_data = detect_pt_add_data,
};
