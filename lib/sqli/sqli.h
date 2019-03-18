#ifndef SQLI_H
#define SQLI_H

#include <detect/detect_parser.h>
#include <detect/detect_re2c.h>
#include <stdint.h>

struct sqli_detect_ctx;

enum SQLI_CTX {
    SQLI_CTX_DATA = 0,
    SQLI_CTX_IN_STRING,
    SQLI_CTX_RCE,
    SQLI_CTX_DATA_VAR_START_WITH_NUM,
    SQLI_CTX_IN_STRING_VAR_START_WITH_NUM,
    SQLI_CTX_RCE_VAR_START_WITH_NUM,
    SQLI_CTX_LAST,
};

struct sqli_token_arg_data {
    struct detect_str value;
#define SQLI_KEY_READ       (1<<0)
#define SQLI_KEY_WRITE      (1<<1)
#define SQLI_KEY_INSTR      (1<<2)
#define SQLI_DATA_NOSTART   (1<<3)
#define SQLI_DATA_NOEND     (1<<4)
#define SQLI_VALUE_NEEDFREE (1<<5)
    uint32_t flags;
    int tok;
};

#include "sqli_parser.h"

#define MAX_PENDING_TOKENS 1
#define SQLI_PENDING_NEXT(idx) \
    (((idx) == (MAX_PENDING_TOKENS - 1)) ? 0 : (idx) + 1)
#define SQLI_PENDING_SHIFT(idx) ({idx = SQLI_PENDING_NEXT(idx);})

struct sqli_pending_token {
    int tok;
    union SQLI_PARSER_STYPE arg;
    void (*destructor)(void *token);
};

DETECT_HIDDEN void sqli_token_data_destructor(void *token);

struct sqli_detect_lexer_ctx {
    unsigned instring:1;

    struct detect_re2c re2c;

    struct sqli_pending_token pending[MAX_PENDING_TOKENS];
    unsigned pending_first;
    unsigned pending_last;
    int state;
    int condition;
    struct detect_buf buf;
};

struct sqli_detect_ctx {
    struct detect_ctx base;
    enum SQLI_CTX type;
    unsigned ctxnum;
    bool has_any_tokens;
    struct detect *detect;
    sqli_parser_pstate *pstate;
    struct sqli_detect_lexer_ctx lexer;
    struct detect_ctx_result res;
    bool var_start_with_num;
};

DETECT_HIDDEN int sqli_get_token(
    struct sqli_detect_ctx *ctx, union SQLI_PARSER_STYPE *arg);

DETECT_HIDDEN int sqli_store_data(
    struct sqli_detect_ctx *ctx, struct sqli_token_arg_data *info);

#endif /* SQLI_H */
