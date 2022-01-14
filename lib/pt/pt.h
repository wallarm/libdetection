#ifndef PATHTRAVERSAL_H
#define PATHTRAVERSAL_H

#include <detect/detect_parser.h>
#include <detect/detect_re2c.h>

struct pt_detect_ctx;

enum PT_CTX {
    PT_CTX_INJECTION = 0,
};

struct pt_token_arg_data {
    struct detect_str value;
#define PT_KEY_INSTR      (1 << 0)
#define PT_VALUE_NEEDFREE (1 << 1)
    uint32_t flags;
    int tok;
};

struct pt_detect_lexer_ctx {
    struct detect_re2c re2c;
    int state;
    int condition;
    struct detect_buf buf;
};

#include "pt_parser.h"

struct pt_detect_ctx {
    struct detect_ctx base;
    enum PT_CTX type;
    unsigned ctxnum;
#define PT_PENDING_SEP 1
#define PT_PENDING_END 2
    unsigned pending;
    struct detect *detect;
    pt_parser_pstate *pstate;
    struct pt_detect_lexer_ctx lexer;
    struct detect_ctx_result res;
};

DETECT_HIDDEN int
pt_get_token(struct pt_detect_ctx *ctx, union PT_PARSER_STYPE *arg);
DETECT_HIDDEN void
pt_token_data_destructor(void *token);
DETECT_HIDDEN int
pt_store_data(struct pt_detect_ctx *ctx, struct pt_token_arg_data *info);

#endif /* PATHTRAVERSAL_H */
