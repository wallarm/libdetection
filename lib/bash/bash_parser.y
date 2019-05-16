%{
#include "bash.h"

static void
bash_parser_error(struct bash_detect_ctx *ctx, const char *s)
{
    ctx->res.parse_error = true;
}
%}

%define api.pure full
%define api.push-pull push
%define api.prefix {bash_parser_}
%parse-param {struct bash_detect_ctx *ctx}
%union {
    struct bash_token_arg_data data;
}

%token TOK_START_RCE
%token TOK_START_WORD
%token <data> '<' '>' '-' ';' '(' ')' '|' '&' '\n' '\r'
%token <data> TOK_WORD TOK_ASSIGNMENT_WORD
%token TOK_ERROR

%destructor {
    bash_token_data_destructor(&$$);
} <data>

%%

context: start_rce
        | start_word
        ;

start_rce: TOK_START_RCE simple_command
        ;

start_word: TOK_START_WORD {
            ctx->lexer.inword = true;
        } simple_command
        ;

simple_command_element: TOK_WORD[tk1] {
          bash_store_data(ctx, &$tk1);
        }
        | TOK_ASSIGNMENT_WORD[u1] {
          YYUSE($u1);
        }
        ;

simple_command: simple_command_element
        | simple_command simple_command_element
        ;

%%
