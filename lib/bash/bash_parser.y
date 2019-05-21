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
%token <data> TOK_WORD TOK_NUMBER TOK_IF TOK_THEN TOK_ELSE TOK_ELIF TOK_FI
              TOK_CASE TOK_ESAC TOK_FOR TOK_SELECT TOK_WHILE TOK_UNTIL TOK_DO
              TOK_DONE TOK_IN TOK_FUNCTION TOK_TIME TOK_TIMEOPT TOK_TIMEIGN
              TOK_BEGIN TOK_END TOK_BANG TOK_COND_START TOK_COND_END TOK_COPROC
              TOK_ASSIGNMENT_WORD TOK_REDIR_WORD TOK_ARITH_CMD
              TOK_ARITH_FOR_EXPRS TOK_COND_CMD
%token <data> TOK_LESS_LESS_LESS TOK_LESS_LESS_MINUS TOK_AND_GREATER_GREATER
              TOK_SEMI_SEMI_AND TOK_LESS_LESS TOK_LESS_GREATER TOK_LESS_AND
              TOK_GREATER_GREATER TOK_GREATER_AND TOK_GREATER_BAR
              TOK_AND_GREATER TOK_AND_AND TOK_OR_OR TOK_SEMI_SEMI TOK_SEMI_AND
              TOK_BAR_AND
%token TOK_ERROR

%destructor {
    bash_token_data_destructor(&$$);
} <data>

%%

context: start_rce
        | start_word
        ;

start_rce: TOK_START_RCE simple_list
        ;

start_word: TOK_START_WORD {
            ctx->lexer.inword = true;
        } simple_list
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

command: simple_command
        ;

list_terminator: '\n'[u1] {
            YYUSE($u1);
        }
        | '\r'[u1] '\n'[u2] {
            YYUSE($u1);
            YYUSE($u2);
        }
        | ';'[u1] {
            YYUSE($u1);
        }
        ;

newline_list:
        | newline_list '\n'[u1] {
            YYUSE($u1);
        }
        | newline_list '\r'[u1] '\n'[u2] {
            YYUSE($u1);
            YYUSE($u2);
        }
        ;

simple_list: simple_list1
        | simple_list1 '&'[u1] {
            YYUSE($u1);
        }
        | simple_list1 ';'[u1] {
            YYUSE($u1);
        }
        ;

simple_list1: simple_list1 TOK_AND_AND[u1] newline_list simple_list1 {
            YYUSE($u1);
        }
        | simple_list1 TOK_OR_OR[u1] newline_list simple_list1 {
            YYUSE($u1);
        }
        | simple_list1 TOK_AND_AND[u1]{
            YYUSE($u1);
        }
        | simple_list1 TOK_OR_OR[u1]{
            YYUSE($u1);
        }
        | simple_list1 '&'[u1] simple_list {
            YYUSE($u1);
        }
        | simple_list1 ';'[u1] simple_list {
            YYUSE($u1);
        }
        | pipeline_command
        ;

pipeline_command: pipeline
        | TOK_BANG[u1] pipeline_command {
            YYUSE($u1);
        }
        | timespec pipeline_command
        | timespec list_terminator
        | TOK_BANG[u1] list_terminator {
            YYUSE($u1);
        }
        ;

pipeline: pipeline '|'[u1] newline_list pipeline {
            YYUSE($u1);
        }
        | pipeline '|'[u1] {
            YYUSE($u1);
        }
        | pipeline TOK_BAR_AND[u1] newline_list pipeline {
            YYUSE($u1);
        }
        | command
        ;

timespec: TOK_TIME[tk1] {
            bash_store_data(ctx, &$tk1);
        }
        | TOK_TIME[tk1] TOK_TIMEOPT[tk2] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
        }
        | TOK_TIME[tk1] TOK_TIMEOPT[tk2] TOK_TIMEIGN[tk3] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        ;
%%
