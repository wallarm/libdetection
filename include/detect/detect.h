#ifndef DETECT_H
#define DETECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <wallarm/tree.h>
#include <sys/queue.h>

int detect_init(void);
int detect_deinit(void);

struct detect;

#define CSTR_LEN(cstr) .str = cstr, .len = sizeof(cstr) - 1
#define DETECT_CSTR(cstr) {CSTR_LEN(cstr)}
struct detect_str {
    char *str;
    size_t len;
};

struct detect_token_stat {
    struct detect_str token_name;
    size_t count;

    RB_ENTRY(detect_token_stat) link;
};

RB_HEAD(detect_token_stat_tree, detect_token_stat);
WRB_PROTOTYPE(detect_token_stat_tree, detect_token_stat, struct detect_str *);

struct detect_flag_stat {
    struct detect_str flag_name;
    size_t count;
    struct detect_token_stat_tree stat_by_tokens;

    RB_ENTRY(detect_flag_stat) link;
};

struct detect_data {
    struct detect_str kind;
    struct detect_str value;

    STAILQ_ENTRY(detect_data) link;
};

struct detect_ctx_desc {
    struct detect_str name;
    unsigned rce:1;
};

RB_HEAD(detect_flag_stat_tree, detect_flag_stat);
WRB_PROTOTYPE(detect_flag_stat_tree, detect_flag_stat, struct detect_str *);

struct detect_ctx_result {
    struct detect_flag_stat_tree stat_by_flags;
    STAILQ_HEAD(, detect_data) datas;
    unsigned finished:1;
    unsigned disabled:1;
    unsigned parse_error:1;
};

typedef int (*detect_finish_cb)(
    struct detect *detect, unsigned ctxnum, unsigned n_unfinished, void *arg);

void *detect_parser_list(const struct detect_str **name);
void *detect_parser_list_next(void *ctx, const struct detect_str **name);

struct detect *detect_open(const char *parser);
const struct detect_str *detect_name(struct detect *detect);
int detect_close(struct detect *detect);

int detect_set_options(struct detect *detect, const char *options);
int detect_set_finish_cb(struct detect *detect, detect_finish_cb cb, void *arg);

int detect_start(struct detect *detect);
int detect_stop(struct detect *detect);
unsigned detect_get_nctx(struct detect *detect);
#define DETECT_ATTACK_RCE (1<<0)
#define DETECT_ATTACK_INJ (1<<1)
bool detect_has_attack(struct detect *detect, uint32_t *attack_types);
int detect_ctx_disable(struct detect *detect, unsigned ctxnum);
bool detect_ctx_has_attack(struct detect *detect, unsigned ctxnum);
const struct detect_ctx_desc *detect_ctx_get_desc(
    struct detect *detect, unsigned ctxnum);
const struct detect_ctx_result *detect_ctx_get_result(
    struct detect *detect, unsigned ctxnum);

int detect_add_data(
    struct detect *detect, const void *data, size_t siz, bool fin);

#endif /* DETECT_H */
