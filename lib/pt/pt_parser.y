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
%union {
    struct pt_token_arg_data data;
}

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

ssep: TOK_SEP[k] {
            pt_store_data(ctx, &$k);
        }
        ;
strav: TOK_TRAV[k] {
            pt_store_data(ctx, &$k);
        }
        ;
sroot: TOK_ROOT[r] {
            pt_store_data(ctx, &$r);
        }
        ;
sname: TOK_NAME[n] {
            pt_store_data(ctx, &$n);
        }
        ;

seps:     ssep
        | seps ssep
        ;
inj_pref:
        | inj_pref sroot seps
        | inj_pref sname seps
        ;
inj_pref2: inj_pref strav seps
        | inj_pref3 strav seps
        | inj_pref2 strav seps
        ;
inj_pref3: inj_pref2 sname seps
        | inj_pref3 sname seps
        ;
inj:      inj_pref2 sroot {
            YYACCEPT;
        }
        | inj_pref3 sroot {
            YYACCEPT;
        }
        ;
%%
