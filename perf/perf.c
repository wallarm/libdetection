#include <detect/detect.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

#define ERR(fmt, ...) fprintf(stderr, fmt"\n", ##__VA_ARGS__)

#define MAXLINE 4096
#define MAX_DISABLED_CTXS 8

enum perf_action {
    PERF_ACTION_PARSE = 0,
    PERF_ACTION_LIST_PARSERS,
};

static void
s_perf_dump_result(struct detect *detect)
{
    unsigned nctx, i;

    nctx = detect_get_nctx(detect);
    for (i = 0; i < nctx; i++) {
        const struct detect_ctx_desc *desc;
        struct detect_ctx_result *res;

        desc = detect_ctx_get_desc(detect, i);
        res = (typeof(res))detect_ctx_get_result(detect, i);
        printf("[%s]%.*s:%s\n",
               desc->rce ? "rce" : "inj",
               (int)desc->name.len, desc->name.str,
               res->parse_error ? "error:" : "");
        if (!RB_EMPTY(&res->stat_by_flags)) {
            struct detect_flag_stat *fs;

            printf(" flags:\n");
            RB_FOREACH(fs, detect_flag_stat_tree, &res->stat_by_flags) {
                struct detect_token_stat *ts;

                printf("  %.*s:%zu:\n  ",
                       (int)fs->flag_name.len, fs->flag_name.str,
                       fs->count);

                RB_FOREACH(ts, detect_token_stat_tree, &fs->stat_by_tokens) {
                    printf(" %.*s:%zu",
                           (int)ts->token_name.len, ts->token_name.str,
                           ts->count);
                }
                printf("\n");
            }
        }
        if (!STAILQ_EMPTY(&res->datas)) {
            struct detect_data *dd;

            printf(" datas:\n");
            STAILQ_FOREACH(dd, &res->datas, link) {
                printf("  kind:%.*s value:%.*s\n",
                       (int)dd->kind.len, dd->kind.str,
                       (int)dd->value.len, dd->value.str);
            }
        }
    }
}

static void
s_report_attack(uint64_t cstr, bool has_attack, uint32_t attack_types)
{
    if (!has_attack) {
        printf("Test %"PRIu64": clear\n", cstr + 1);
        return;
    }
    printf("Test %"PRIu64":", cstr + 1);
    if (!!(attack_types & DETECT_ATTACK_INJ))
        printf(" inj");
    if (!!(attack_types & DETECT_ATTACK_RCE))
        printf(" rce");
    printf("\n");
}

static void
perf_usage(const char *progname)
{
    fprintf(stderr,
            "Usage:\n"
            "\t%s [-Pehnrv] [-p parser] [-d disabled_context]\n"
            "\n"
            "\t-h   Show this help message\n"
            "\t-v   Increase verbose level\n"
            "\t-P   List available parsers and exit\n"
            "\t-p   Parser to use (default is sqli)\n"
            "\t-d   Context number to disable\n"
            "\t-e   Echo input strings if attack is detected\n"
            "\t-f   Echo input strings if attack is not detected\n"
            "\t     (verbose=0: attacks only, verbose>0: all)\n"
            "\t-r   Show small report for each input string\n"
            "\t-n   Show total number of strings and attacks\n"
            , progname);
}

static int
perf_list_parsers(void)
{
    void *list;
    const struct detect_str *name;

    printf("Parsers available:\n");
    for (list = detect_parser_list(&name); list;
         list = detect_parser_list_next(list, &name)) {
        struct detect *detect;

        printf("%.*s\n", (int)name->len, name->str);
        if ((detect = detect_open(name->str)) != NULL) {
            unsigned i, nctx;

            nctx = detect_get_nctx(detect);
            for (i = 0; i != nctx; i++) {
                const struct detect_ctx_desc *ctx;

                ctx = detect_ctx_get_desc(detect, i);
                printf("\tContext %u: %.*s (%s)\n",
                       i, (int)ctx->name.len, ctx->name.str,
                       ctx->rce ? "rce" : "inj");
            }
        }
    }
    return (EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
    struct detect *detect;
    char *progname;
    char buf[MAXLINE];
    unsigned disabled_ctxs[MAX_DISABLED_CTXS];
    unsigned n_disabled_ctxs = 0, ictx;
    char parser[32] = "sqli";
    size_t len;
    int argval;
    unsigned verbose = 0;
    enum perf_action action = PERF_ACTION_PARSE;
    bool print_nstr = false;
    bool report_attacks = false;
    bool echo = false;
    bool echo_false = false;
    bool detect_initialized = false;
    uint64_t nstr, nattacks;
    int rc = EXIT_SUCCESS;

    if ((progname = strrchr(argv[0], '/')) != NULL)
        progname++;
    else
        progname = argv[0];
    progname = strdup(progname);

    while ((argval = getopt(argc, argv, "Pp:efhnrvd:")) != EOF) {
        switch (argval) {
        case 'P':
            action = PERF_ACTION_LIST_PARSERS;
            break;
        case 'p':
            snprintf(parser, sizeof(parser), "%s", optarg);
            break;
        case 'd': {
            unsigned ctx = atoi(optarg);

            for (ictx = 0; ictx != n_disabled_ctxs; ictx++)
                if (disabled_ctxs[ictx] == ctx)
                    break;
            if (ictx == n_disabled_ctxs) {
                if (n_disabled_ctxs == MAX_DISABLED_CTXS) {
                    ERR("Too many disabled contexts specified");
                    rc = EXIT_FAILURE;
                    goto done;
                }
                disabled_ctxs[n_disabled_ctxs++] = ctx;
            }
            break;
        }
        case 'e':
            echo = true;
            break;
        case 'f':
            echo_false = true;
            break;
        case 'n':
            print_nstr = true;
            break;
        case 'v':
            verbose++;
            break;
        case 'r':
            report_attacks = true;
            break;
        default:
            rc = EXIT_FAILURE;
        /* FALLTHROUGH */
        case 'h':
            perf_usage(progname);
            goto done;
        }
    }
    if (detect_init()) {
        ERR("Cannot initialize detect");
        rc = EXIT_FAILURE;
        goto done;
    }
    detect_initialized = true;

    switch (action) {
    case PERF_ACTION_LIST_PARSERS:
        rc = perf_list_parsers();
        goto done;
    default:
        break;
    }

    if ((detect = detect_open(parser)) == NULL) {
        ERR("Cannot open %s parser", parser);
        rc = EXIT_FAILURE;
        goto done;
    }
    for (ictx = 0; ictx != n_disabled_ctxs; ictx++) {
        const struct detect_ctx_desc *ctx;
        int rv;

        ctx = detect_ctx_get_desc(detect, disabled_ctxs[ictx]);
        if (ctx == NULL) {
            ERR("Cannot get context[%u] description: %s",
                disabled_ctxs[ictx], strerror(errno));
            rc = EXIT_FAILURE;
            goto done;
        }
        if (!!(rv = detect_ctx_disable(detect, disabled_ctxs[ictx]))) {
            fprintf(stderr, "Cannot disable context %u[%.*s]: %s\n",
                    disabled_ctxs[ictx], (int)ctx->name.len,
                    ctx->name.str, strerror(rv));
            rc = EXIT_FAILURE;
            goto done;
        }
    }
    nstr = 0;
    nattacks = 0;
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        bool has_attack;
        bool multiline;
        uint32_t attack_types;

        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            --len;
            if (len > 0 && buf[len - 1] == '\r')
                --len;
        }
        /* skip comment lines */
        if (len != 0 && buf[0] == '#')
            continue;
        multiline = (len != 0 && buf[len - 1] == '\\');
        if (multiline)
            --len;
        detect_start(detect);
        detect_add_data(detect, buf, len, !multiline);
        if (multiline)
            continue;
        has_attack = detect_has_attack(detect, &attack_types);
        if (report_attacks)
            s_report_attack(nstr, has_attack, attack_types);
        if ((echo && (has_attack || verbose > 0)) ||
            (echo_false && !has_attack))
            printf("%.*s\n", (int)len, buf);
        if (has_attack ? verbose > 0 : verbose > 1)
            s_perf_dump_result(detect);
        detect_stop(detect);
        nstr++;
        if (has_attack)
            nattacks++;
    }
    detect_close(detect);
    detect_deinit();
    if (print_nstr)
        fprintf(stderr, "Got %"PRIu64" strings, %"PRIu64" attacks\n",
                nstr, nattacks);
  done:
    if (detect_initialized)
        detect_deinit();
    free(progname);
    return (rc);
}
