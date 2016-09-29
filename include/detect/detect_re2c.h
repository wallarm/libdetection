#ifndef DETECT_RE2C_H
#define DETECT_RE2C_H

#include <stddef.h>
#include <stdbool.h>

#define DETECT_RE2C_YYCTYPE unsigned char
#define DETECT_RE2C_YYCURSOR(ctx) (ctx)->pos
#define DETECT_RE2C_YYMARKER(ctx) (ctx)->marker
#define DETECT_RE2C_YYCTXMARKER(ctx) (ctx)->ctxmarker
#define DETECT_RE2C_YYFILL(ctx, pend, need, maxfill)                    \
    do {                                                                \
        int rv;                                                         \
        if (!!(rv = detect_re2c_yyfill(ctx, pend, need, maxfill)))      \
            return (-rv);                                               \
    } while (0)
#define DETECT_RE2C_UNUSED_BEFORE(ctx)                  \
    do {                                                \
        (ctx)->start = DETECT_RE2C_YYCURSOR(ctx);       \
    } while (0)

struct detect_re2c {
    unsigned char *tmp_data;
    const unsigned char *data;
    const unsigned char *start;
    const unsigned char *pos;
    const unsigned char *marker;
    const unsigned char *ctxmarker;
    size_t siz;
    unsigned yyfill_need;
    unsigned data_copied;
    unsigned tmp_data_siz;
    unsigned tmp_data_alloc;

    unsigned fin:1;
    unsigned tmp_data_in_use:1;
};

int detect_re2c_init(struct detect_re2c *ctx);
int detect_re2c_deinit(struct detect_re2c *ctx);

int
detect_re2c_add_data(
    struct detect_re2c *ctx, const void *data, size_t siz, bool fin);

int detect_re2c_yyfill(
    struct detect_re2c *ctx, const unsigned char **end,
    unsigned need, unsigned maxfill);

int detect_re2c_prepare_input(
    struct detect_re2c *ctx, const unsigned char **end, unsigned maxfill);

#endif /* DETECT_RE2C_H */
