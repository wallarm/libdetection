#ifndef DETECT_BUF_H
#define DETECT_BUF_H

#include "detect.h"
#include <errno.h>
#include <stdlib.h>

struct detect_buf {
    struct detect_str data;
    size_t allocated;
    size_t minsiz;
    ssize_t maxsiz;
};

static inline int
detect_buf_reinit(struct detect_buf *buf)
{
    buf->data.str = NULL;
    buf->data.len = 0;
    buf->allocated = 0;
    return (0);
}

static inline int
detect_buf_init(
    struct detect_buf *buf, size_t minsiz, ssize_t maxsiz)
{
    if (!minsiz)
        minsiz = 32;
    buf->minsiz = minsiz;
    if (maxsiz > 0 && minsiz > (size_t)maxsiz)
        maxsiz = minsiz;
    buf->maxsiz = maxsiz;
    return (detect_buf_reinit(buf));
}

static inline void
detect_buf_deinit(struct detect_buf *buf)
{
    if (buf->data.str != NULL)
        free(buf->data.str);
}

static inline int
detect_buf_add_char(struct detect_buf *buf, unsigned char ch)
{
    void *str;
    size_t new_allocated;

    if (buf->data.len != buf->allocated) {
        buf->data.str[buf->data.len++] = ch;
        return (0);
    }
    new_allocated = buf->allocated ? buf->allocated * 2 : buf->minsiz;
    if (buf->maxsiz >= 0 && new_allocated > buf->maxsiz)
        new_allocated = buf->maxsiz;
    if (buf->data.len == new_allocated)
        return (EOVERFLOW);
    if ((str = realloc(buf->data.str, new_allocated)) == NULL) {
        buf->data.str = NULL;
        return (ENOMEM);
        };
    buf->data.str = str;
    buf->allocated = new_allocated;
    buf->data.str[buf->data.len++] = ch;
    return (0);
}

#endif /* DETECT_BUF_H */
