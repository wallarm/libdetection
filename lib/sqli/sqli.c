#include "sqli.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

static const struct {
    struct detect_ctx_desc desc;
    enum sqli_parser_tokentype start_tok;
    bool var_start_with_num;
} sqli_ctxs[] = {
    [SQLI_CTX_DATA] = {
        .desc = {.name = {CSTR_LEN("data")}},
        .start_tok = TOK_START_DATA,
        .var_start_with_num = false,
    },
    [SQLI_CTX_IN_STRING] = {
        .desc = {.name = {CSTR_LEN("str")}},
        .start_tok = TOK_START_STRING,
        .var_start_with_num = false,
    },
    [SQLI_CTX_RCE] = {
        .desc = {.name = {CSTR_LEN("rce")}, .rce = true},
        .start_tok = TOK_START_RCE,
        .var_start_with_num = false,
    },
    [SQLI_CTX_DATA_VAR_START_WITH_NUM] = {
        .desc = {.name = {CSTR_LEN("data_num")}},
        .start_tok = TOK_START_DATA,
        .var_start_with_num = true,
    },
    [SQLI_CTX_IN_STRING_VAR_START_WITH_NUM] = {
        .desc = {.name = {CSTR_LEN("str_num")}},
        .start_tok = TOK_START_STRING,
        .var_start_with_num = true,
    },
    [SQLI_CTX_RCE_VAR_START_WITH_NUM] = {
        .desc = {.name = {CSTR_LEN("rce_num")}, .rce = true},
        .start_tok = TOK_START_RCE,
        .var_start_with_num = true,
    },
};

static struct detect *
detect_sqli_open(struct detect_parser *parser)
{
    struct detect *detect;
    unsigned i;

    detect = malloc(sizeof(*detect));
    detect_instance_init(detect, parser);

    detect->nctx = SQLI_CTX_LAST;
    detect->ctxs = malloc(detect->nctx * sizeof(*detect->ctxs));
    for (i = 0; i < detect->nctx; i++) {
        struct sqli_detect_ctx *ctx;

        ctx = calloc(1, sizeof(*ctx));
        ctx->base.desc = (struct detect_ctx_desc *)&sqli_ctxs[i].desc;
        ctx->base.res = &ctx->res;
        detect_ctx_result_init(ctx->base.res);
        ctx->type = i;
        ctx->ctxnum = i;
        ctx->detect = detect;
        ctx->var_start_with_num = sqli_ctxs[i].var_start_with_num;
        detect->ctxs[i] = (void *)ctx;
    }

    return (detect);
}

static int
detect_sqli_close(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct sqli_detect_ctx *ctx = (void *)detect->ctxs[i];

        detect_ctx_result_deinit(ctx->base.res);
        if (ctx->pstate != NULL)
            sqli_parser_pstate_delete(ctx->pstate);
        free(ctx);
    }
    if (detect->ctxs != NULL)
        free(detect->ctxs);
    free(detect);

    return (0);
}

static int
detect_sqli_push_token(struct sqli_detect_ctx *ctx, int tok, void *tok_val)
{
    int rv;

    if (ctx->res.finished)
        return (0);

    rv = sqli_parser_push_parse(ctx->pstate, tok, tok_val, ctx);
    if (rv == YYPUSH_MORE)
        rv = 0;
    else {
        ctx->res.finished = true;
        ctx->detect->nctx_finished++;
        /*
         * On parse error, ctx->res.parse_error is already set by
         * sqli_parser_error().
         */
        if (ctx->detect->finish_cb != NULL)
            rv = ctx->detect->finish_cb(
                ctx->detect, ctx->ctxnum,
                ctx->detect->nctx - ctx->detect->nctx_finished,
                ctx->detect->finish_cb_arg);
        else
            rv = 0;
    }
    return (rv);
}

static void
sqli_lexer_init(struct sqli_detect_lexer_ctx *lexer)
{
    memset(lexer, 0, sizeof(*lexer));
    detect_re2c_init(&lexer->re2c);
    lexer->state = -1;
}

static void
sqli_lexer_deinit(struct sqli_detect_lexer_ctx *lexer)
{
    detect_buf_deinit(&lexer->buf);
    while (1) {
        struct sqli_pending_token *token =
            lexer->pending + lexer->pending_first;

        if (token->tok <= 0)
            break;
        if (token->destructor != 0)
            token->destructor(token);
        token->tok = 0;
        SQLI_PENDING_SHIFT(lexer->pending_first);
    }
    lexer->pending_last = lexer->pending_first;
    detect_re2c_deinit(&lexer->re2c);
}

void
sqli_token_data_destructor(void *token)
{
    struct sqli_token_arg_data *data = token;

    if (!!(data->flags & SQLI_VALUE_NEEDFREE) && data->value.str != NULL)
        free(data->value.str);
    data->value.str = NULL;
    data->value.len = 0;
}

static int
detect_sqli_start(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct sqli_detect_ctx *ctx = (void *)detect->ctxs[i];

        if (ctx->res.finished)
            continue;

        ctx->pstate = sqli_parser_pstate_new();
        sqli_lexer_init(&ctx->lexer);
        if (detect_sqli_push_token(
                ctx, sqli_ctxs[ctx->type].start_tok, NULL) != 0)
            break;
    }
    return (0);
}

static int
detect_sqli_stop(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct sqli_detect_ctx *ctx = (void *)detect->ctxs[i];

        if (ctx->pstate == NULL)
            continue;

        if (!ctx->res.finished)
            detect_sqli_push_token(ctx, 0, NULL);
        sqli_parser_pstate_delete(ctx->pstate);
        ctx->pstate = NULL;
        ctx->has_any_tokens = false;
        sqli_lexer_deinit(&ctx->lexer);
        detect_ctx_result_deinit(&ctx->res);
    }
    return (0);
}

static int
sqli_lexer_add_data(
    struct sqli_detect_ctx *ctx, const void *data, size_t siz, bool fin)
{
    return (detect_re2c_add_data(&ctx->lexer.re2c, data, siz, fin));
}

static int
detect_sqli_add_data(
    struct detect *detect, const void *data, size_t siz, bool fin)
{
    unsigned i;
    union SQLI_PARSER_STYPE token_arg;
    int rv = 0;

    for (i = 0; i < detect->nctx; i++) {
        struct sqli_detect_ctx *ctx = (void *)detect->ctxs[i];
        int token;

        if (ctx->res.finished)
            continue;
        sqli_lexer_add_data(ctx, data, siz, fin);
        do {
            memset(&token_arg, 0, sizeof(token_arg));
            token = sqli_get_token(ctx, &token_arg);

            if (token > 0) {
                if (token != TOK_ERROR) {
                    ctx->has_any_tokens = true;
                    if ((rv = detect_sqli_push_token(ctx, token, &token_arg)) != 0)
                        goto done;
                }
            } else if (token < 0 && token != -EAGAIN) {
                /* fatal error */
                rv = -token;
                goto done;
            }

            /* We stop parsing with success on end of data */
            if (!ctx->res.finished) {
                if (token == 0 || token == TOK_ERROR ||
                    (token == -EAGAIN && fin)) {

                    /* We push $end to the parser */
                    detect_sqli_push_token(ctx, 0, NULL);
                    ctx->res.finished = true;
                    /*
                     * We may clear error here to:
                     * - ignore unfinished syntax
                     * - ignore results of reduce/reduce conflicts
                     */
                    ctx->res.parse_error =
                        (token == TOK_ERROR ||
                         RB_EMPTY(&ctx->res.stat_by_flags)
                         /* || !ctx->has_any_tokens */);
                }
            }
        } while (!ctx->res.finished && token != -EAGAIN);
    }

  done:
    return (rv);
}

static int
sqli_store_key(
    struct sqli_detect_ctx *ctx,
    struct sqli_token_arg_data *info)
{
    const static struct {
        struct detect_str name;
        uint32_t flag;
    } flagnames[] = {
        {.flag = SQLI_KEY_READ, .name = {CSTR_LEN("READ")}},
        {.flag = SQLI_KEY_WRITE, .name = {CSTR_LEN("WRITE")}},
        {.flag = SQLI_KEY_INSTR, .name = {CSTR_LEN("INSTR")}},
    };
    unsigned i;

    for (i = 0; i < sizeof(flagnames) / sizeof(flagnames[0]); i++) {
        if (!(info->flags & flagnames[i].flag))
            continue;
        detect_ctx_result_store_token(
            &ctx->res, &flagnames[i].name, &info->value);
    }
    sqli_token_data_destructor(info);
    return (0);
}

int
sqli_store_data(
    struct sqli_detect_ctx *ctx,
    struct sqli_token_arg_data *info)
{
    switch (info->tok) {
    case TOK_DATA:
        detect_ctx_result_store_data(
            &ctx->res, &(struct detect_str){CSTR_LEN("DATA")}, &info->value);
        return (0);
    case TOK_NAME:
        detect_ctx_result_store_data(
            &ctx->res, &(struct detect_str){CSTR_LEN("NAME")}, &info->value);
        return (0);
    default:
        return (sqli_store_key(ctx, info));
    }
}

struct detect_parser detect_parser_sqli = {
    .name = {CSTR_LEN("sqli")},
    .open = detect_sqli_open,
    .close = detect_sqli_close,
    .start = detect_sqli_start,
    .stop = detect_sqli_stop,
    .add_data = detect_sqli_add_data,
};
