#include <detect.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#define ERR(fmt, ...) fprintf(stderr, fmt"\n", ##__VA_ARGS__)

#define MAXLINE 4096

static void
s_perf_dump_result(struct detect *detect)
{
    unsigned nctx, i;

    nctx = detect_get_nctx(detect);
    for (i = 0; i < nctx; i++) {
        const struct detect_ctx_desc *desc;
        const struct detect_ctx_result *res;

        desc = detect_ctx_get_desc(detect, i);
        res = detect_ctx_get_result(detect, i);
        printf("[%s]%.*s:%s\n",
               desc->rce ? "rce" : "inj",
               (int)desc->name.len, desc->name.str,
               res->parse_error ? "error:" : "");
        if (res->stat_by_flags.head != NULL) {
            avl_node_t *flag_link;

            printf(" flags:\n");
            for (flag_link = res->stat_by_flags.head;
                 flag_link != NULL; flag_link = flag_link->next) {
                avl_node_t *token_link;
                struct detect_flag_stat *fs =
                    container_of(flag_link, typeof(*fs), link);

                printf("  %.*s:%zu:\n  ",
                       (int)fs->flag_name.len, fs->flag_name.str,
                       fs->count);

                for (token_link = fs->stat_by_tokens.head;
                     token_link != NULL; token_link = token_link->next) {
                    struct detect_token_stat *ts =
                        container_of(token_link, typeof(*ts), link);
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
            "\t%s [-ehnrv] [-p parser]\n"
            "\n"
            "\t-h   Show this help message\n"
            "\t-v   Increase verbose level\n"
            "\t-p   Parser to use (default is sqli)\n"
            "\t-e   Echo input strings\n"
            "\t     (verbose=1: attacks only, verbose=1: all)\n"
            "\t-r   Show small report for each input string\n"
            "\t-n   Show total number of strings and attacks\n"
            , progname);
}

int
main(int argc, char **argv)
{
    struct detect *detect;
    char *progname;
    char buf[MAXLINE];
    char parser[32] = "sqli";
    size_t len;
    int argval;
    unsigned verbose = 0;
    bool print_nstr = false;
    bool report_attacks = false;
    bool echo = false;
    uint64_t nstr, nattacks;
    int rc = EXIT_SUCCESS;

    if ((progname = strrchr(argv[0], '/')) != NULL)
        progname++;
    else
        progname = argv[0];
    progname = strdup(progname);

    while ((argval = getopt(argc, argv, "p:ehnrv")) != EOF) {
        switch (argval) {
        case 'p':
            snprintf(parser, sizeof(parser), "%s", optarg);
            break;
        case 'e':
            echo = true;
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
    if ((detect = detect_open(parser)) == NULL) {
        ERR("Cannot open %s parser", parser);
        rc = EXIT_FAILURE;
        goto done;
    }
    nstr = 0;
    nattacks = 0;
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        bool has_attack;
        uint32_t attack_types;

        len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[--len] = 0;
            if (len > 0 && buf[len - 1] == '\r')
                buf[--len] = 0;
        }
        /* skip empty & comment lines */
        if (len == 0 || buf[0] == '#' || buf[0] == 0)
            continue;
        detect_start(detect);
        detect_add_data(detect, buf, len, true);
        has_attack = detect_has_attack(detect, &attack_types);
        if (report_attacks)
            s_report_attack(nstr, has_attack, attack_types);
        if (echo && (has_attack || verbose > 0))
            puts(buf);
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
    free(progname);
    return (rc);
}
