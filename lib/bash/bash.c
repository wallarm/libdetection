#include "bash.h"
#include <string.h>
#include <assert.h>

static const struct {
    struct detect_ctx_desc desc;
    enum bash_parser_tokentype start_tok;
} bash_ctxs[] = {
    [BASH_CTX_RCE] = {
        .desc = {.name = {CSTR_LEN("rce")}},
        .start_tok = TOK_START_RCE,
    },
    [BASH_CTX_IN_WORD] = {
        .desc = {.name = {CSTR_LEN("word")}},
        .start_tok = TOK_START_WORD,
    },
};

static struct detect *
detect_bash_open(struct detect_parser *parser)
{
    struct detect *detect;
    unsigned i;

    detect = malloc(sizeof(*detect));
    detect_instance_init(detect, parser);
    detect->nctx = BASH_CTX_LAST;
    detect->ctxs = malloc(detect->nctx * sizeof(*detect->ctxs));

    for (i = 0; i < detect->nctx; i++) {
        struct bash_detect_ctx *ctx;

        ctx = calloc(1, sizeof(*ctx));
        ctx->base.desc = (struct detect_ctx_desc *)&bash_ctxs[i].desc;
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
detect_bash_close(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct bash_detect_ctx *ctx = (void *)detect->ctxs[i];

        detect_ctx_result_deinit(ctx->base.res);
        if (ctx->pstate != NULL)
            bash_parser_pstate_delete(ctx->pstate);
        free(ctx);
    }
    if (detect->ctxs != NULL)
        free(detect->ctxs);
    free(detect);

    return (0);
}

static void
bash_lexer_init(struct bash_detect_lexer_ctx *lexer)
{
    memset(lexer, 0, sizeof(*lexer));
    detect_re2c_init(&lexer->re2c);
    lexer->state = -1;
}

static void
bash_lexer_deinit(struct bash_detect_lexer_ctx *lexer)
{
    detect_buf_deinit(&lexer->buf);
    detect_re2c_deinit(&lexer->re2c);
}

void
bash_token_data_destructor(void *token)
{
    struct bash_token_arg_data *data = token;

    if (!!(data->flags & BASH_VALUE_NEEDFREE) && data->value.str != NULL)
        free(data->value.str);
}

static int
detect_bash_push_token(struct bash_detect_ctx *ctx, int tok, void *tok_val)
{
    int rv;

    if (ctx->res.finished)
        return (0);

    rv = bash_parser_push_parse(ctx->pstate, tok, tok_val, ctx);
    if (rv == YYPUSH_MORE)
        rv = 0;
    else {
        ctx->res.finished = true;
        ctx->detect->nctx_finished++;
        /*
         * On parse error, ctx->res.parse_error is already set by
         * bash_parser_error().
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

static int
detect_bash_start(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct bash_detect_ctx *ctx = (void *)detect->ctxs[i];

        if (ctx->res.finished)
            continue;
        ctx->pstate = bash_parser_pstate_new();
        ctx->last_read_token = '\n';
        ctx->token_before_that = 0;
        bash_lexer_init(&ctx->lexer);
        if (detect_bash_push_token(
                ctx, bash_ctxs[ctx->type].start_tok, NULL) != 0)
            break;
    }
    return (0);
}

static int
detect_bash_stop(struct detect *detect)
{
    unsigned i;

    for (i = 0; i < detect->nctx; i++) {
        struct bash_detect_ctx *ctx = (void *)detect->ctxs[i];

        if (ctx->pstate == NULL)
            continue;

        /* We have to finish bison state machine or memleaks will occur*/
        if (!ctx->res.finished)
            detect_bash_push_token(ctx, 0, NULL);

        bash_parser_pstate_delete(ctx->pstate);
        ctx->pstate = NULL;
        bash_lexer_deinit(&ctx->lexer);
        detect_ctx_result_deinit(&ctx->res);
    }
    return (0);
}

static int
bash_lexer_add_data(
    struct bash_detect_ctx *ctx, const void *data, size_t siz, bool fin)
{
    return (detect_re2c_add_data(&ctx->lexer.re2c, data, siz, fin));
}

static int
detect_bash_add_data(
    struct detect *detect, const void *data, size_t siz, bool fin)
{
    unsigned i;
    union BASH_PARSER_STYPE token_arg;
    int rv = 0;

    for (i = 0; i < detect->nctx; i++) {
        struct bash_detect_ctx *ctx = (void *)detect->ctxs[i];
        int token;

        if (ctx->res.finished)
            continue;
        bash_lexer_add_data(ctx, data, siz, fin);
        do {
            memset(&token_arg, 0, sizeof(token_arg));
            token = bash_get_token(ctx, &token_arg);

            if (token >= 0) {
                if ((rv = detect_bash_push_token(ctx, token, &token_arg)) != 0)
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
bash_store_key(
    struct bash_detect_ctx *ctx,
    struct bash_token_arg_data *info)
{
    const static struct {
        struct detect_str name;
        uint32_t flag;
    } flagnames[] = {
        {.flag = BASH_KEY_INSTR, .name = {CSTR_LEN("INSTR")}},
    };
    unsigned i;

    for (i = 0; i < sizeof(flagnames) / sizeof(flagnames[0]); i++) {
        if (!(info->flags & flagnames[i].flag))
            continue;
        detect_ctx_result_store_token(
            &ctx->res, &flagnames[i].name, &info->value);
    }
    bash_token_data_destructor(info);
    return (0);
}

int
bash_store_data(
    struct bash_detect_ctx *ctx,
    struct bash_token_arg_data *info)
{
    switch (info->tok) {
    default:
        return (bash_store_key(ctx, info));
    }
}

struct detect_parser detect_parser_bash = {
    .name = {CSTR_LEN("bash")},
    .open = detect_bash_open,
    .close = detect_bash_close,
    .start = detect_bash_start,
    .stop = detect_bash_stop,
    .add_data = detect_bash_add_data,
};
