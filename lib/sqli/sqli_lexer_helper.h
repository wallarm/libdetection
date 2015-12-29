#ifndef SQLI_LEXER_HELPER_H
#define SQLI_LEXER_HELPER_H

#include <stdlib.h>
#include <stdbool.h>

#define MAXBUFSIZ 1024

static inline void
sqli_data_start(struct sqli_detect_lexer_ctx *lexer, size_t maxsiz)
{
    lexer->buf.str = NULL;
    lexer->buf.len = 0;
    lexer->bufsiz = maxsiz ? maxsiz : MAXBUFSIZ;
}

static inline bool
sqli_data_eat_char(
    struct sqli_detect_lexer_ctx *lexer, unsigned char ch)
{
    if (lexer->buf.str == NULL) {
        if (!lexer->bufsiz)
            lexer->bufsiz = MAXBUFSIZ;
        lexer->buf.str = malloc(lexer->bufsiz);
        lexer->buf.len = 0;
    }
    if (lexer->buf.len < lexer->bufsiz - 1) {
        lexer->buf.str[lexer->buf.len++] = ch;
        return (true);
    }
    return (false);
}

static inline void
sqli_data_done(struct sqli_detect_lexer_ctx *lexer)
{
    if (lexer->buf.str == NULL) {
        lexer->bufsiz = 1;
        lexer->buf.str = calloc(1, 1);
        lexer->buf.len = 0;
    } else
        lexer->buf.str[lexer->buf.len] = 0;
}

static inline void
sqli_data_free(struct sqli_detect_lexer_ctx *lexer)
{
    if (lexer->bufsiz) {
        free(lexer->buf.str);
        sqli_data_start(lexer, 0);
    }
}

static inline void
sqli_data_error(struct sqli_detect_lexer_ctx *lexer)
{
    sqli_data_free(lexer);
}

#endif /* SQLI_LEXER_HELPER_H */
