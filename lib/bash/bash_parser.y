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

word_list: TOK_WORD[u1] {
            YYUSE($u1);
        }
        | word_list TOK_WORD[u1] {
            YYUSE($u1);
        }
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
        | shell_command
        | function_def
        | coproc
        ;

shell_command: for_command
        | case_command
        | TOK_WHILE[tk1] compound_list TOK_DO[tk2] compound_list TOK_DONE[tk3] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        | TOK_UNTIL[tk1] compound_list TOK_DO[tk2] compound_list TOK_DONE[tk3] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        | select_command
        | if_command
        | subshell
        | group_command
        | arith_command
        | cond_command
        | arith_for_command
        ;

for_command: TOK_FOR[tk1] TOK_WORD[tk2] newline_list
            TOK_DO[tk3] compound_list TOK_DONE[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] newline_list
            TOK_BEGIN[tk3] compound_list TOK_END[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] ';'[u1] newline_list
            TOK_DO[tk3] compound_list TOK_DONE[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            YYUSE($u1);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] ';'[u1] newline_list
            TOK_BEGIN[tk3] compound_list TOK_END[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            YYUSE($u1);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] word_list
            list_terminator newline_list
            TOK_DO[tk4] compound_list TOK_DONE[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] word_list
            list_terminator newline_list
            TOK_BEGIN[tk4] compound_list TOK_END[tk5]
        {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3]
            list_terminator newline_list
            TOK_DO[tk4] compound_list TOK_DONE[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        | TOK_FOR[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3]
            list_terminator newline_list
            TOK_BEGIN[tk4] compound_list TOK_END[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        ;

arith_for_command: TOK_FOR[tk1] TOK_ARITH_FOR_EXPRS[tk2]
            list_terminator newline_list
            TOK_DO[tk3] compound_list TOK_DONE[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_ARITH_FOR_EXPRS[tk2] list_terminator newline_list
            TOK_BEGIN[tk3] compound_list TOK_END[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_ARITH_FOR_EXPRS[tk2]
            TOK_DO[tk3] compound_list TOK_DONE[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_FOR[tk1] TOK_ARITH_FOR_EXPRS[tk2]
            TOK_BEGIN[tk3] compound_list TOK_END[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        ;

select_command: TOK_SELECT[tk1] TOK_WORD[tk2] newline_list
            TOK_DO[tk3] list TOK_DONE[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] newline_list
            TOK_BEGIN[tk3] list TOK_END[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] ';'[u1] newline_list
            TOK_DO[tk3] list TOK_DONE[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            YYUSE($u1);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] ';'[u1] newline_list
            TOK_BEGIN[tk3] list TOK_END[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            YYUSE($u1);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] word_list
            list_terminator newline_list TOK_DO[tk4] list TOK_DONE[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] word_list
            list_terminator newline_list TOK_BEGIN[tk4] list TOK_END[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] list_terminator
            newline_list TOK_DO[tk4] compound_list TOK_DONE[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        | TOK_SELECT[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] list_terminator
            newline_list TOK_BEGIN[tk4] compound_list TOK_END[tk5] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
            bash_store_data(ctx, &$tk5);
        }
        ;

case_command: TOK_CASE[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] newline_list
            TOK_ESAC[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_CASE[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3]
            case_clause_sequence newline_list TOK_ESAC[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_CASE[tk1] TOK_WORD[tk2] newline_list TOK_IN[tk3] case_clause
            TOK_ESAC[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        ;

function_def: TOK_WORD[tk1] '('[u1] ')'[u2] newline_list function_body {
            bash_store_data(ctx, &$tk1);
            YYUSE($u1);
            YYUSE($u2);
        }
        | TOK_FUNCTION[tk1] TOK_WORD[tk2] '('[u1] ')'[u2] newline_list
            function_body {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            YYUSE($u1);
            YYUSE($u2);
        }
        | TOK_FUNCTION[tk1] TOK_WORD[tk2] newline_list function_body {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
        }
        ;

function_body: shell_command
        ;

subshell: '('[u1] compound_list ')'[u2] {
            YYUSE($u1);
            YYUSE($u2);
        }
        ;

coproc: TOK_COPROC[tk1] shell_command {
            bash_store_data(ctx, &$tk1);
        }
        | TOK_COPROC[tk1] TOK_WORD[tk2] shell_command {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
        }
        | TOK_COPROC[tk1] simple_command {
            bash_store_data(ctx, &$tk1);
        }
        ;

if_command: TOK_IF[tk1] compound_list TOK_THEN[tk2] compound_list TOK_FI[tk3] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        | TOK_IF[tk1] compound_list TOK_THEN[tk2] compound_list
            TOK_ELSE[tk3] compound_list TOK_FI[tk4] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
            bash_store_data(ctx, &$tk4);
        }
        | TOK_IF[tk1] compound_list TOK_THEN[tk2] compound_list
            elif_clause TOK_FI[tk3] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        ;

group_command: TOK_BEGIN[tk1] compound_list TOK_END[tk2] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
        }
        ;

arith_command: TOK_ARITH_CMD[tk1] {
            bash_store_data(ctx, &$tk1);
        }
        ;

cond_command: TOK_COND_START[tk1] TOK_COND_CMD[tk2] TOK_COND_END[tk3] {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        ;

elif_clause: TOK_ELIF[tk1] compound_list TOK_THEN[tk2] compound_list {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
        }
        | TOK_ELIF[tk1] compound_list TOK_THEN[tk2] compound_list
            TOK_ELSE[tk3] compound_list {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
            bash_store_data(ctx, &$tk3);
        }
        | TOK_ELIF[tk1] compound_list TOK_THEN[tk2] compound_list elif_clause {
            bash_store_data(ctx, &$tk1);
            bash_store_data(ctx, &$tk2);
        }
        ;

case_clause:pattern_list
        | case_clause_sequence pattern_list
        ;

pattern_list: newline_list pattern ')'[u1] compound_list {
            YYUSE($u1);
        }
        | newline_list pattern ')'[u1] newline_list {
            YYUSE($u1);
        }
        | newline_list '('[u1] pattern ')'[u2] compound_list {
            YYUSE($u1);
            YYUSE($u2);
        }
        | newline_list '('[u1] pattern ')'[u2] newline_list {
            YYUSE($u1);
            YYUSE($u2);
        }
        ;

case_clause_sequence: pattern_list TOK_SEMI_SEMI[u1] {
            YYUSE($u1);
        }
        | case_clause_sequence pattern_list TOK_SEMI_SEMI[u1] {
            YYUSE($u1);
        }
        | pattern_list TOK_SEMI_AND[u1] {
            YYUSE($u1);
        }
        | case_clause_sequence pattern_list TOK_SEMI_AND[u1] {
            YYUSE($u1);
        }
        | pattern_list TOK_SEMI_SEMI_AND[u1] {
            YYUSE($u1);
        }
        |case_clause_sequence pattern_list TOK_SEMI_SEMI_AND[u1] {
            YYUSE($u1);
        }
        ;

pattern: TOK_WORD[tk1] {
            bash_store_data(ctx, &$tk1);
        }
        | pattern '|'[u1] TOK_WORD[tk1] {
            YYUSE($u1);
            bash_store_data(ctx, &$tk1);
        }
        ;

list: newline_list list0
        ;

compound_list: list
        | newline_list list1
        ;

list0: list1 '\n'[u1] newline_list {
            YYUSE($u1);
        }
        | list1 '&'[u1] newline_list {
            YYUSE($u1);
        }
        | list1 ';'[u1] newline_list {
            YYUSE($u1);
        }
        ;

list1: list1 TOK_AND_AND[u1] newline_list list1 {
            YYUSE($u1);
        }
        | list1 TOK_OR_OR[u1] newline_list list1 {
            YYUSE($u1);
        }
        | list1 '&'[u1] newline_list list1 {
            YYUSE($u1);
        }
        | list1 ';'[u1] newline_list list1 {
            YYUSE($u1);
        }
        | list1 '\n'[u1] newline_list list1 {
            YYUSE($u1);
        }
        |pipeline_command
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
