#include "detect_int.h"
#include <string.h>

int
detect_str_cmp(const void *a, const void *b, void *user)
{
    const struct detect_str *s1 = a, *s2 = b;
    int rc;

    if (s1->len < s2->len) {
        if (!(rc = memcmp(s1->str, s2->str, s1->len)))
            rc = -1;
    } else if (!(rc = memcmp(s1->str, s2->str, s2->len)))
        rc = (s1->len != s2->len);
    return (rc);
}

int
detect_str_cmp_fast(const void *a, const void *b, void *user)
{
    const struct detect_str *s1 = a, *s2 = b;

    if (s1->len < s2->len)
        return (-1);
    if (s1->len > s2->len)
        return (1);
    return (memcmp(s1->str, s2->str, s1->len));
}
