#ifndef DETECT_PARSER_H
#define DETECT_PARSER_H

#include <detect.h>
#include "detect_buf.h"

#define DETECT_HIDDEN __attribute__ ((visibility ("hidden")))

struct detect_parser;

typedef int (*detect_parser_init_func)(void);
typedef int (*detect_parser_deinit_func)(void);
typedef struct detect *(*detect_parser_open_func)(struct detect_parser *parser);
typedef int (*detect_parser_close_func)(struct detect *detect);
typedef int (*detect_parser_set_options_func)(
    struct detect *detect, const char *options);
typedef int (*detect_parser_start_func)(struct detect *detect);
typedef int (*detect_parser_stop_func)(struct detect *detect);
typedef int (*detect_parser_add_data_func)(
    struct detect *detect, const void *data, size_t siz, bool fin);

struct detect_parser {
    struct detect_str name;

    detect_parser_init_func init;
    detect_parser_deinit_func deinit;

    detect_parser_open_func open;
    detect_parser_close_func close;
    detect_parser_set_options_func set_options;

    detect_parser_start_func start;
    detect_parser_stop_func stop;
    detect_parser_add_data_func add_data;
};

struct detect_ctx {
    struct detect_ctx_desc *desc;
    struct detect_ctx_result *res;
};

struct detect {
    struct detect_parser *parser;

    detect_finish_cb finish_cb;
    void *finish_cb_arg;

    struct detect_ctx **ctxs;
    unsigned nctx;
    unsigned nctx_finished;

    unsigned started:1;
};

/* Alphanumeric comparison */
int detect_str_cmp(const void *a, const void *b, void *user);

/* Fast but non-alphanumeric comparison */
int detect_str_cmp_fast(const void *a, const void *b, void *user);

struct detect_parser *detect_parser_find(struct detect_str *name);

int detect_instance_init(struct detect *detect, struct detect_parser *parser);

int detect_ctx_result_init(struct detect_ctx_result *res);
int detect_ctx_result_deinit(struct detect_ctx_result *res);

int detect_ctx_result_store_token(
    struct detect_ctx_result *res, const struct detect_str *flag,
    const struct detect_str *name);

int detect_ctx_result_store_data(
    struct detect_ctx_result *res, const struct detect_str *kind,
    struct detect_str *value);

#endif /* DETECT_PARSER_H */
