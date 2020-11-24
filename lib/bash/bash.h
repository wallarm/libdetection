#ifndef BASH_H
#define BASH_H

#include <detect/detect_parser.h>
#include <detect/detect_re2c.h>

struct bash_detect_ctx;

enum BASH_CTX {
    BASH_CTX_RCE = 0,
    BASH_CTX_IN_WORD = 1,
    BASH_CTX_LAST,
};

struct bash_token_arg_data {
    struct detect_str value;
#define BASH_KEY_INSTR      (1<<0)
#define BASH_VALUE_NEEDFREE (1<<1)
#define BASH_DATA_NOSTART   (1<<2)
#define BASH_DATA_NOEND     (1<<3)
    uint32_t flags;
    int tok;
};

struct var {
    struct detect_str name;
    struct detect_str val;

    RB_ENTRY(var) link;
};

RB_HEAD(vars_tree, var);
WRB_PROTOTYPE(vars_tree, var, struct detect_str *);

struct bash_detect_lexer_ctx {
    unsigned inword:1;

    struct detect_re2c re2c;
    int state;
    int condition;
    struct detect_buf buf;
    struct detect_buf var_name;
    struct vars_tree vars;
};

#include "bash_parser.h"

struct bash_detect_ctx {
    struct detect_ctx base;
    enum BASH_CTX type;
    unsigned ctxnum;
    struct detect *detect;
    bash_parser_pstate *pstate;
    struct bash_detect_lexer_ctx lexer;
    struct detect_ctx_result res;
    int last_read_token;
    int token_before_that;
};

DETECT_HIDDEN int bash_get_token(
    struct bash_detect_ctx *ctx, union BASH_PARSER_STYPE *arg);
DETECT_HIDDEN void bash_token_data_destructor(void *token);
DETECT_HIDDEN int bash_store_data(
    struct bash_detect_ctx *ctx, struct bash_token_arg_data *info);

#endif /* BASH_H */
