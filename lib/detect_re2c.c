#include <detect/detect_re2c.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int
detect_re2c_add_data(
    struct detect_re2c *ctx, const void *data, size_t siz, bool fin)
{
    assert(ctx->data == NULL ||
           ctx->pos == ctx->data + ctx->siz);

    if (!ctx->tmp_data_in_use) {
        ctx->pos = data;
        ctx->start = data;
        ctx->marker = NULL;
        ctx->ctxmarker = NULL;
    }
    ctx->data = data;
    ctx->siz = siz;
    ctx->data_copied = 0;
    ctx->fin = fin;

    return (0);
}

static bool
detect_re2c_chk_switch_to_data(
    struct detect_re2c *ctx, const unsigned char **end)
{
    size_t data_since_start;
    const unsigned char *tmp_data_end = *end;
    const unsigned char *data_copied_ptr;

    data_since_start = tmp_data_end - ctx->start;
    if (ctx->data_copied < data_since_start)
        return (false);

    data_copied_ptr = ctx->data + ctx->data_copied;

    ctx->pos = data_copied_ptr - (tmp_data_end - ctx->pos);
    ctx->start = data_copied_ptr - data_since_start;
    ctx->tmp_data_in_use = false;
    ctx->data_copied = 0;
    ctx->tmp_data_siz = 0;
    *end = ctx->data + ctx->siz;

    return (true);
}

int
detect_re2c_prepare_input(
    struct detect_re2c *ctx, const unsigned char **end, unsigned maxfill)
{
    if (!ctx->tmp_data_in_use) {
        *end = ctx->data + ctx->siz;
        goto fill;
    }

    *end = ctx->tmp_data + ctx->tmp_data_siz;
    detect_re2c_chk_switch_to_data(ctx, end);

  fill:
    if (!ctx->yyfill_need && *end == ctx->pos && ctx->fin) {
        /*
         * A special case: we have to finalize the data
         * but have no more characters in the buffer.
         * Require at least one character.
         */
        ctx->yyfill_need = 1;
    }
    if (ctx->yyfill_need && *end - ctx->pos < ctx->yyfill_need)
        return (detect_re2c_yyfill(ctx, end, ctx->yyfill_need, maxfill));
    return (0);
}

static int
detect_re2c_switch_to_tmp_data(
    struct detect_re2c *ctx, const unsigned char **end,
    unsigned need, unsigned maxfill)
{
    unsigned start_pos_offset = ctx->pos - ctx->start;
    unsigned start_marker_offset = ctx->marker - ctx->start;
    unsigned start_ctxmarker_offset = ctx->ctxmarker - ctx->start;
    unsigned siz = start_pos_offset + maxfill;

    if (ctx->tmp_data == NULL) {
        if ((ctx->tmp_data = malloc(siz)) == NULL)
            return (errno);
        ctx->tmp_data_alloc = siz;
    } else {
        if (siz > ctx->tmp_data_alloc) {
            unsigned new_alloc;
            unsigned char *new_tmp_data;

            for (new_alloc = ctx->tmp_data_alloc * 2;
                 siz > new_alloc; new_alloc *= 2);
            if ((new_tmp_data = realloc(ctx->tmp_data, new_alloc)) == NULL)
                return (errno);
            ctx->tmp_data = new_tmp_data;
            ctx->tmp_data_alloc = new_alloc;
        }
    }
    ctx->tmp_data_siz = *end - ctx->start;
    memcpy(ctx->tmp_data, ctx->start, ctx->tmp_data_siz);
    ctx->start = ctx->tmp_data;
    ctx->pos = ctx->tmp_data + start_pos_offset;
    ctx->marker = ctx->tmp_data + start_marker_offset;
    ctx->ctxmarker = ctx->tmp_data + start_ctxmarker_offset;
    ctx->tmp_data_in_use = true;
    ctx->data = NULL;
    ctx->siz = 0;
    ctx->data_copied = 0;
    *end = ctx->tmp_data + ctx->tmp_data_siz;

    return (EAGAIN);
}

static int
detect_re2c_update_tmp_data(
    struct detect_re2c *ctx, const unsigned char **end,
    unsigned need, unsigned maxfill)
{
    unsigned tmp_data_size, siz;
    unsigned shift;

    /* first of all, shift the data to the beginning of the tmp buffer */
    tmp_data_size = *end - ctx->start;
    shift = ctx->start - ctx->tmp_data;
    if (shift) {
        memmove(ctx->tmp_data, ctx->start, tmp_data_size);
        ctx->start = ctx->tmp_data;
        ctx->tmp_data_siz = tmp_data_size;
        ctx->pos -= shift;
        ctx->marker -= shift;
        ctx->ctxmarker -= shift;
        (*end) -= shift;
    }

    /*
     * Make sure that the temporary buffer is of enough size
     * to fill maxfill bytes.
     */
    siz = (ctx->pos - ctx->tmp_data) + maxfill;
    if (siz > ctx->tmp_data_alloc) {
        unsigned new_alloc;
        unsigned char *new_tmp_data;

        for (new_alloc = ctx->tmp_data_alloc * 2;
             siz > new_alloc; new_alloc *= 2);
        if ((new_tmp_data = realloc(ctx->tmp_data, new_alloc)) == NULL)
            return (errno);
        ctx->start = new_tmp_data;
        ctx->pos = new_tmp_data + (ctx->pos - ctx->tmp_data);
        ctx->marker = new_tmp_data + (ctx->marker - ctx->tmp_data);
        ctx->ctxmarker = new_tmp_data + (ctx->ctxmarker - ctx->tmp_data);
        *end = new_tmp_data + (*end - ctx->tmp_data);
        ctx->tmp_data = new_tmp_data;
        ctx->tmp_data_alloc = new_alloc;
    }

    /*
     * Now, try to copy available bytes from the original buffer into
     * the tmp one.
     */
    if (ctx->data != NULL) {
        unsigned avail = ctx->siz - ctx->data_copied;

        /*
         * It is possible to (avail == 0) here,
         * when ctx->siz == 0 && ctx->data_copied == 0.
         */

        need -= *end - ctx->pos;

        if (avail > need)
            avail = need;
        memcpy((void *)*end, ctx->data + ctx->data_copied, avail);
        (*end) += avail;
        ctx->data_copied += avail;
        ctx->tmp_data_siz += avail;
        if (ctx->data_copied == ctx->siz) {
            ctx->data = NULL;
            ctx->siz = 0;
            ctx->data_copied = 0;
        } else
            detect_re2c_chk_switch_to_data(ctx, end);
    }
    if (*end - ctx->pos < need)
        return (EAGAIN);
    return (0);
}

int
detect_re2c_yyfill(
    struct detect_re2c *ctx, const unsigned char **end,
    unsigned need, unsigned maxfill)
{
    int rv;

    assert(need <= maxfill);

    if (!ctx->tmp_data_in_use)
        rv = detect_re2c_switch_to_tmp_data(ctx, end, need, maxfill);
    else
        rv = detect_re2c_update_tmp_data(ctx, end, need, maxfill);

    if (rv != EAGAIN) {
        ctx->yyfill_need = 0;
        return (rv);
    }

    if (!ctx->fin) {
        ctx->yyfill_need = need;
        return (EAGAIN);
    } else {
        /* zero-fill the requested part of tmp_data buffer */
        unsigned zero_fill_size = need - (*end - ctx->pos);
        memset((void *)*end, 0, zero_fill_size);
        (*end) += zero_fill_size;
        ctx->tmp_data_siz += zero_fill_size;
        return (0);
    }
}

int
detect_re2c_init(struct detect_re2c *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    return (0);
}

int
detect_re2c_deinit(struct detect_re2c *ctx)
{
    free(ctx->tmp_data);
    return (0);
}
