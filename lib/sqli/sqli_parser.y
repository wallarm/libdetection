%{
#include "sqli.h"

static void
sqli_parser_error(struct sqli_detect_ctx *ctx, const char *s)
{
    ctx->res.parse_error = true;
}

%}

%define api.pure full
%define api.push-pull push
%define api.prefix {sqli_parser_}
%parse-param {struct sqli_detect_ctx *ctx}
%union {
    struct sqli_token_arg_data data;
}
%define lr.type lalr

%type <data> data_name data_name_only
%type <data> operator command
%type <data> join_type
%type <data> select_extra_tk
%type <data> union_tk
%type <data> func_args_modifier_tk
%type <data> post_expr_tk
%type <data> delete_modifier

%token TOK_START_DATA
%token TOK_START_STRING
%token TOK_START_RCE
%token <data> TOK_DISTINCT TOK_VARIADIC
%token <data> TOK_DATA TOK_NAME TOK_OPERATOR
%token <data> '.' ',' '(' ')' '*' '[' ']' ';' '='
%token <data> TOK_OR TOK_AND TOK_IS TOK_NOT TOK_DIV
              TOK_MOD TOK_XOR TOK_REGEXP
              TOK_BINARY TOK_SOUNDS TOK_OUTFILE TOK_MATCH TOK_AGAINST
%token <data> TOK_COLLATE
%token <data> TOK_FOR
%token <data> TOK_FROM TOK_INTO TOK_WHERE
%token <data> TOK_AS TOK_ON TOK_USING
%token <data> TOK_UNION TOK_INTERSECT TOK_EXCEPT TOK_ALL
%token <data> TOK_ORDER TOK_GROUP TOK_BY TOK_HAVING
%token <data> TOK_TOP TOK_PERCENT
%token <data> TOK_DESC TOK_ASC
%token <data> TOK_CROSS TOK_FULL TOK_INNER TOK_LEFT TOK_RIGHT
%token <data> TOK_LIMIT TOK_OFFSET
%token <data> TOK_NATURAL TOK_JOIN
%token <data> TOK_SELECT TOK_UPDATE TOK_INSERT TOK_EXECUTE TOK_DELETE
%token <data> TOK_ATTACH
%token <data> TOK_DROP
%token <data> TOK_COMMA
%token <data> TOK_SET
%token <data> TOK_BETWEEN TOK_LIKE TOK_RLIKE TOK_IN TOK_BOOLEAN TOK_MODE
%token <data> TOK_CASE TOK_WHEN TOK_THEN TOK_ELSE TOK_BEGIN TOK_END
%token <data> TOK_WAITFOR TOK_DELAY TOK_TIME
%token <data> TOK_CREATE TOK_REPLACE TOK_FUNCTION TOK_RETURNS TOK_LANGUAGE TOK_STRICT
%token <data> TOK_SHUTDOWN
%token <data> TOK_DECLARE
%token <data> TOK_TABLE
%token <data> TOK_USE
%token <data> TOK_IGNORE TOK_LOW_PRIORITY TOK_QUICK
%token <data> TOK_PRINT
%token TOK_FUNC
%token TOK_ERROR

%destructor {
    sqli_token_data_destructor(&$$);
} <data>

%%

context:  start_data
        | start_string
        | start_rce
        ;

start_data: TOK_START_DATA data_name_only {
            /*
             * Caution! Mid-rule action.
             * $data_name_only may be later freed by a destructor.
             */
            sqli_store_data(ctx, &$data_name_only);
            $data_name_only.value.str = NULL;
        } post_exprs_opt data_cont
        ;

start_string: TOK_START_STRING {
            ctx->lexer.instring = true;
        } data_name_only {
            /*
             * Caution! Mid-rule action.
             * $data_name_only may be later freed by a destructor.
             */
            sqli_store_data(ctx, &$data_name_only);
            $data_name_only.value.str = NULL;
        } post_exprs_opt data_cont
        ;

data_name:  TOK_DATA
        |   TOK_NAME
        /* Tokens-as-identifiers here */
        ;

data_name_only:  data_name
        ;

data_cont:
        | after_exp_cont_op after_exp_cont
        | close_multiple_parens_opt semicolons_opt multiple_sqls
        | after_exp_cont
        ;

update: TOK_UPDATE[tk1] colref_exact TOK_SET[tk2] expr_list {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

sql_no_parens:
        select
        | update
        | begin_end
        | waitfor_delay
        | func
        | create_function
        | shutdown
        | declare
        | execute
        | drop
        | use
        | _delete
        | print
        | command error {
            sqli_store_data(ctx, &$command);
            yyclearin;
        }
        ;

sql_parens: sql_no_parens
        | '('[u1] sql_parens ')'[u2] {YYUSE($u1); YYUSE($u2);}
        ;

colref_exact:
        data_name {
            sqli_store_data(ctx, &$data_name);
        }
        | data_name[tname] '.'[u1] data_name[colname] {
            sqli_store_data(ctx, &$tname);
            sqli_store_data(ctx, &$colname);
            YYUSE($u1);
        }
        | data_name[dname] '.'[u1] data_name[tname] '.'[u2] data_name[colname] {
            sqli_store_data(ctx, &$dname);
            sqli_store_data(ctx, &$tname);
            sqli_store_data(ctx, &$colname);
            YYUSE($u1);
            YYUSE($u2);
        }
        | data_name[dname] '.'[u1] '.'[u2] data_name[colname] {
            sqli_store_data(ctx, &$dname);
            sqli_store_data(ctx, &$colname);
            YYUSE($u1);
            YYUSE($u2);
        }
        ;

colref_asterisk:
        data_name '.'[u1] '*'[u2] {
            sqli_store_data(ctx, &$data_name);
            YYUSE($u1);
            YYUSE($u2);
        }
        | data_name[dname] '.'[u1] data_name[tname] '.'[u2] '*'[u3] {
            sqli_store_data(ctx, &$dname);
            sqli_store_data(ctx, &$tname);
            YYUSE($u1);
            YYUSE($u2);
            YYUSE($u3);
        }
        ;

name_list: colref_exact
        | name_list ','[u1] colref_exact {YYUSE($u1);}
        ;

expr_common:
          func
        | operator expr {
            sqli_store_data(ctx, &$operator);
        }
        | '('[tk] select ')'[u1] alias_opt {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
        }
        | '('[tk] expr_list ')'[u1] {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
        }
        | '('[tk] error {
            sqli_store_data(ctx, &$tk);
        }
        | '('[tk] error ')'[u1] {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
        }
        ;

op_expr:  expr_common
        | '('[tk] select ')'[u1] expr {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
        }
        | '('[tk] expr ')'[u1] expr {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
        }
        | op_expr post_exprs
        ;

post_expr_tk:
        TOK_IN[tk] TOK_BOOLEAN[u1] TOK_MODE[u2] {
            $$ = $tk;
            $$.value = (struct detect_str){CSTR_LEN("IN_BOOLEAN_MODE")};
            YYUSE($u1);
            YYUSE($u2);
        }
        | TOK_END
        ;

post_expr:
        post_expr_tk[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

post_exprs:
          post_expr
        | post_exprs post_expr
        ;

post_exprs_opt:
        | post_exprs
        ;

expr:   expr_common
        | colref_exact
        | colref_asterisk
        | expr operator expr {
            sqli_store_data(ctx, &$operator);
        }
        | expr post_exprs
        ;

func_name:  colref_exact
        | TOK_LEFT
        ;

expr_list:
          expr
        | expr_list ','[u1] expr {YYUSE($u1);}
        ;

expr_list_opt:
        | expr_list
        ;

func_args_list:
          expr
        | func_args_list ','[u1] expr {YYUSE($u1);}
        | error
        ;

func_args_modifier_tk:
          TOK_DISTINCT
        | TOK_ALL
        | TOK_VARIADIC
        ;

func_args_modifier:
        func_args_modifier_tk[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;
        
func_args_modifier_opt:
        | func_args_modifier
        ;

func_distinct_opt:
        | TOK_DISTINCT
        ;

func_args:  '('[u1] func_distinct_opt ')'[u2] {
            YYUSE($u1);
            YYUSE($u2);
        }
        |   '('[u1] func_distinct_opt '*'[u2] ')'[u3] {
            YYUSE($u1);
            YYUSE($u2);
            YYUSE($u3);
        }
        |   '('[u1] func_distinct_opt func_args_modifier_opt
            func_args_list ')'[u2] {

            YYUSE($u1);
            YYUSE($u2);
        }
        ;

func:     func_name func_args {
            struct sqli_token_arg_data key = {
                .value = {CSTR_LEN("FUNC")},
                .flags = SQLI_KEY_INSTR,
                .tok = TOK_FUNC,
            };
            sqli_store_data(ctx, &key);
        }
        ;

operator: TOK_OR
        | TOK_AND
        | TOK_IS
        | TOK_NOT
        | TOK_DIV
        | TOK_MOD
        | TOK_XOR
        | TOK_REGEXP
        | TOK_BETWEEN
        | TOK_LIKE
        | TOK_RLIKE
        | TOK_OPERATOR
        | TOK_CASE
        | TOK_WHEN
        | TOK_THEN
        | TOK_ELSE
        | TOK_WAITFOR
        | TOK_DELAY
        | TOK_IN
        | TOK_BINARY
        | TOK_SOUNDS
        | TOK_INTO
        | TOK_OUTFILE
        | TOK_MATCH
        | TOK_AGAINST
        | TOK_COLLATE
        | '*'
        | '='
        ;

select_distinct_opt:
        | TOK_DISTINCT
        | TOK_ALL
        ;

select_arg:
          expr alias_opt
        | error
        ;

select_list: select_arg
        | select_list ','[u1] select_arg {YYUSE($u1);}
        ;

select_args: select_distinct_opt
        | select_distinct_opt '*'[key] {
            sqli_store_data(ctx, &$key);
        }
        | select_distinct_opt select_list
        ;

as_opt:
        | TOK_AS[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

alias: as_opt data_name[data] {
            sqli_store_data(ctx, &$data);
        }
        ;

alias_opt:
        | alias
        ;

table_ref:
          func alias_opt
        | colref_exact alias_opt
        | '('[u1] select ')'[u2] alias_opt {YYUSE($u1); YYUSE($u2);}
        | joined_table
        | '('[u1] joined_table ')'[u2] alias {YYUSE($u1); YYUSE($u2);}
        ;

join_type:
          TOK_CROSS
        | TOK_FULL
        | TOK_LEFT
        | TOK_RIGHT
        | TOK_INNER
        ;

join_type_opt:
        | join_type[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

natural_opt:
        | TOK_NATURAL[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

join_qual:
        | TOK_USING[tk] '('[u1] name_list ')'[u2] {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
            YYUSE($u2);
        }
        | TOK_ON[tk] expr {
            sqli_store_data(ctx, &$tk);
        }
        ;

joined_table:
          '('[u1] joined_table ')'[u2] {YYUSE($u1); YYUSE($u2);}
        | table_ref natural_opt join_type_opt TOK_JOIN[tk] table_ref join_qual {
            sqli_store_data(ctx, &$tk);
        }
        ;

from_list: table_ref
        | from_list ','[u1] table_ref {YYUSE($u1);}
        ;

from_opt:
        | TOK_FROM[key] from_list {
            sqli_store_data(ctx, &$key);
        }
        | TOK_FROM[key] '('[u1] from_list ')'[u2] {
              sqli_store_data(ctx, &$key);
              YYUSE($u1);
              YYUSE($u2);
        }
        ;

where_opt:
        | TOK_WHERE[key] expr {
            sqli_store_data(ctx, &$key);
        }
        ;

group_opt:
        | TOK_GROUP[tk1] TOK_BY[tk2] func_args_list {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

having_opt:
        | TOK_HAVING[tk1] expr {
            sqli_store_data(ctx, &$tk1);
        }
        ;

order:
        | TOK_DESC[tk] {
            sqli_store_data(ctx, &$tk);
        }
        | TOK_ASC [tk]{
            sqli_store_data(ctx, &$tk);
        }
        ;

sort_list:
          expr order
        | sort_list ','[u1] expr order {YYUSE($u1);}
        | error
        ;

sort_opt:
        | TOK_ORDER[tk1] TOK_BY[tk2] sort_list {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

select_extra_tk:
          TOK_LIMIT
        | TOK_OFFSET
        | TOK_FOR[tk] TOK_UPDATE[u1] {
            $$ = $tk;
            $$.value = (struct detect_str){CSTR_LEN("FOR_UPDATE")};
            YYUSE($u1);
        }
        ;

select_extra:
        select_extra_tk[tk] expr_list_opt {
            sqli_store_data(ctx, &$tk);
        }
        ;

select_extras: select_extra
        | select_extras select_extra
        ;

select_extras_opt:
        | select_extras
        ;

select_after_where:
        group_opt having_opt sort_opt select_extras_opt
        ;

outfile_opt:
        | TOK_OUTFILE[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

into_opt:
        | TOK_INTO[tk] outfile_opt colref_exact {
            sqli_store_data(ctx, &$tk);
        }
        ;

union_tk:
          TOK_UNION
        | TOK_INTERSECT
        | TOK_EXCEPT
        ;

union_c: union_tk[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

all_distinct_opt:
        | TOK_ALL[tk] {
            sqli_store_data(ctx, &$tk);
        }
        | TOK_DISTINCT[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

select_parens:
        select
        | '('[u1] select_parens ')'[u2] {YYUSE($u1); YYUSE($u2);}
        ;

percent_opt:
        | TOK_PERCENT[tk] {
                sqli_store_data(ctx, &$tk);
        }

top_opt:
        | TOK_TOP[tk] data_name[data] percent_opt {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$data);
        }
        | TOK_TOP[tk] '('[u1] expr ')'[u2] percent_opt {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
            YYUSE($u2);
        }
        ;

select:   TOK_SELECT[tk] top_opt select_args into_opt from_opt
          where_opt select_after_where {
            sqli_store_data(ctx, &$tk);
        }
        | TOK_SELECT[tk] top_opt select_args from_opt into_opt
          where_opt select_after_where {
            sqli_store_data(ctx, &$tk);
        }
        | select union_c all_distinct_opt select_parens
        ;

begin_end: TOK_BEGIN[tk1] multiple_sqls TOK_END[tk2] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

waitfor_delay: TOK_WAITFOR[tk1] TOK_DELAY[tk2] data_name[data] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$data);
        }
        | TOK_WAITFOR[tk1] TOK_TIME[tk2] data_name[data] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$data);
        }
        ;

or_replace_opt:
        | TOK_OR[tk1] TOK_REPLACE[tk2] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

create_function_body: TOK_AS[tk] data_name[obj_file] ','[u1] data_name[link_symbol] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$obj_file);
            YYUSE($u1);
            sqli_store_data(ctx, &$link_symbol);
        }
        | TOK_LANGUAGE[tk] data_name[lang_name] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$lang_name);
        }
        | TOK_STRICT[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

create_function_bodies: create_function_body
        | create_function_bodies create_function_body
        ;

create_function: TOK_CREATE[tk1] or_replace_opt TOK_FUNCTION[tk2] func
                 TOK_RETURNS[tk3] data_name[rettype] create_function_bodies {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$rettype);
        }
        ;

shutdown: TOK_SHUTDOWN[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

var_list: data_name[name] {
            sqli_store_data(ctx, &$name);
        }
        | data_name[name] ','[u1] var_list {
            sqli_store_data(ctx, &$name);
            YYUSE($u1);
        }
        ;

declare: TOK_DECLARE[tk] var_list as_opt data_name[type] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$type);
        }
        | TOK_DECLARE[tk] var_list as_opt data_name[type] '('[u1] data_name[len] ')'[u2] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$type);
            YYUSE($u1);
            sqli_store_data(ctx, &$len);
            YYUSE($u2);
        }
        ;

param: data_name[value] {
            sqli_store_data(ctx, &$value);
        }
        | data_name[name] '='[u1] data_name[value] {
            sqli_store_data(ctx, &$name);
            YYUSE($u1);
            sqli_store_data(ctx, &$value);
        }
        ;

param_list: param
        | param ','[u1] param_list {
            YYUSE($u1);
        }
        ;
execute:
        TOK_EXECUTE[tk] func_name param_list {
            sqli_store_data(ctx, &$tk);
        }
        ;

drop:     TOK_DROP[tk1] TOK_FUNCTION[tk2] func_name {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        | TOK_DROP[tk1] TOK_TABLE[tk2] func_name {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

use:
        TOK_USE[tk] data_name[database] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$database);
        }
        ;

delete_modifier:
         TOK_IGNORE
       | TOK_LOW_PRIORITY
       | TOK_QUICK
       ;

delete_modifier_opt:
        | delete_modifier[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

limit_op: TOK_LIMIT[tk] data_name[row_count] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$row_count);
        }
        ;

_delete:  TOK_DELETE[tk1] delete_modifier_opt TOK_FROM[key] from_list where_opt sort_opt limit_op {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$key);
        }
        | TOK_DELETE[tk1] delete_modifier_opt from_list TOK_FROM[key] from_list where_opt {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$key);
        }
        | TOK_DELETE[tk1] delete_modifier_opt TOK_FROM[key] from_list TOK_USING[tk2] from_list where_opt {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$key);
            sqli_store_data(ctx, &$tk2);
        }
        | TOK_DELETE[tk1] top_opt TOK_FROM[key] from_list where_opt {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$key);
        }
        ;

print:    TOK_PRINT[tk] expr {
            sqli_store_data(ctx, &$tk);
        }
        ;

command:  TOK_INSERT
        | TOK_ATTACH
        ;

close_multiple_parens: ')'[u1] {YYUSE($u1);}
        |  close_multiple_parens ')'[u1] {YYUSE($u1);}
        ;

close_multiple_parens_opt:
        | close_multiple_parens
        ;

start_rce: TOK_START_RCE start_rce_cont
        ;

semicolons: ';'[u1] {YYUSE($u1);}
        | semicolons ';'[u1] {YYUSE($u1);}
        ;

semicolons_opt:
        | semicolons
        ;

multiple_sqls: sql_parens semicolons_opt
        | sql_parens semicolons multiple_sqls
        ;

select_after_where_optunion:
        select_after_where
        | select_after_where union_c all_distinct_opt select_parens
        ;

after_exp_cont_op_noexpr:
        select_after_where_optunion
        ;

after_exp_cont_op:
        expr after_exp_cont_op_noexpr
        | where_opt after_exp_cont_op_noexpr
        ;

after_exp_cont:
        close_multiple_parens_opt semicolons multiple_sqls
        | close_multiple_parens after_exp_cont_op after_exp_cont
        | after_exp_cont_op_noexpr close_multiple_parens after_exp_cont
        ;

start_rce_cont: close_multiple_parens_opt semicolons_opt multiple_sqls
        | close_multiple_parens_opt op_expr after_exp_cont
        ;

%%
