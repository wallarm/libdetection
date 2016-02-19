%{
#include "pt.h"

static void
pt_parser_error(struct pt_detect_ctx *ctx, const char *s)
{
    ctx->res.parse_error = true;
}
%}

%define api.pure full
%define api.push-pull push
%define api.prefix {pt_parser_}
%parse-param {struct pt_detect_ctx *ctx}
%define api.value.type {union pt_token_arg}

%token TOK_START_PT_INJ
%token <data> TOK_SEP TOK_TRAV
%token <data> TOK_ROOT TOK_NAME

%left TOK_TRAV TOK_NAME TOK_ROOT
%left PTRAV

%destructor {
    pt_token_data_destructor(&$$);
} <data>

%%

context: start_pt
        ;
start_pt: TOK_START_PT_INJ inj
        ;
seps:     TOK_SEP[k] {
            pt_store_data(ctx, &$k);
        }
        | seps TOK_SEP[k] {
            pt_store_data(ctx, &$k);
        }
        ;

travs:    TOK_TRAV[k] {
            pt_store_data(ctx, &$k);
        }
        | travs seps TOK_TRAV[k] {
            pt_store_data(ctx, &$k);
        }
        ;
inj_pref:
        | inj_pref TOK_ROOT[r] seps {
            pt_store_data(ctx, &$r);
        }
        | inj_pref TOK_NAME[n] seps {
            pt_store_data(ctx, &$n);
        }
        | inj_pref travs seps TOK_NAME[n] seps {
            pt_store_data(ctx, &$n);
        }
        ;
inj:    inj_pref travs seps TOK_ROOT[r] {
            pt_store_data(ctx, &$r);
            YYACCEPT;
        }
        ;
%%
