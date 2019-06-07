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

%type <data> data data_name
%type <data> operator important_operator command
%type <data> join_type
%type <data> select_extra_tk
%type <data> union_tk
%type <data> func_args_modifier_tk
%type <data> post_expr_tk
%type <data> delete_modifier
%type <data> load_modifier data_xml

%token TOK_START_DATA
%token TOK_START_STRING
%token TOK_START_RCE
%token <data> TOK_DISTINCT TOK_VARIADIC
%token <data> TOK_DATA TOK_NAME TOK_OPERATOR
%token <data> '.' ',' '(' ')' '*' '[' ']' ';' '=' ':' '{' '}'
%token <data> TOK_OR TOK_AND TOK_IS TOK_NOT TOK_DIV
              TOK_MOD TOK_XOR TOK_REGEXP
              TOK_BINARY TOK_SOUNDS TOK_OUTFILE TOK_MATCH TOK_AGAINST TOK_EXIST
%token <data> TOK_COLLATE TOK_UESCAPE
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
%token <data> TOK_TABLE TOK_DATABASE
%token <data> TOK_USE
%token <data> TOK_WHILE
%token <data> TOK_IGNORE TOK_LOW_PRIORITY TOK_QUICK
%token <data> TOK_PRINT
%token <data> TOK_LOAD TOK_DATA2 TOK_XML TOK_CONCURRENT TOK_LOCAL TOK_INFILE
%token <data> TOK_PROCEDURE
%token <data> TOK_GOTO
%token <data> TOK_CALL
%token <data> TOK_CURSOR
%token <data> TOK_PATH
%token <data> TOK_VALUES
%token <data> TOK_OPEN
%token <data> TOK_OVER
%token <data> TOK_WITHIN
%token <data> TOK_ALTER TOK_RECOVERY TOK_SIMPLE
%token <data> TOK_LOCK TOK_SHARE
%token <data> TOK_IF
%token <data> TOK_LABEL
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

start_data: TOK_START_DATA data_cont
        | TOK_START_DATA data_expr ','[u1] expr_cont {
            YYUSE($u1);
        }
        ;

start_string: TOK_START_STRING {
            ctx->lexer.instring = true;
        } data_cont
        ;

data:     TOK_DATA
        | data TOK_DATA {
            if ($1.flags == SQLI_VALUE_NEEDFREE) {
                $$.value.str = realloc($1.value.str, $1.value.len +
                                       $2.value.len);
                if ($$.value.str) {
                    $1.flags = 0;
                }
            } else {
                $$.value.str = malloc($1.value.len + $2.value.len);
                memcpy($$.value.str, $1.value.str, $1.value.len);
                $$.flags = SQLI_VALUE_NEEDFREE;
            }
            memcpy($$.value.str + $1.value.len, $2.value.str, $2.value.len);
            $$.value.len = $1.value.len + $2.value.len;

            sqli_token_data_destructor(&$1);
            sqli_token_data_destructor(&$2);
        }
        ;

data_name:  data
        |   TOK_NAME
        |   TOK_DATA2
        |   TOK_TABLE
        | '['[u1] TOK_NAME[name] ']'[u2] {
            YYUSE($u1);
            $$ = $name;
            YYUSE($u2);
        }
        | '['[u1] TOK_TABLE[name] ']'[u2] {
            YYUSE($u1);
            $$ = $name;
            YYUSE($u2);
        }
        | '{'[u1] TOK_NAME[name] expr '}'[u2] {
            YYUSE($u1);
            $$ = $name;
            YYUSE($u2);
        }
        /* Tokens-as-identifiers here */
        ;

expr_cont:
        | expr after_exp_cont_op_noexpr after_exp_cont
        | expr where_opt after_exp_cont_op_noexpr after_exp_cont
        ;

data_cont:
        | data_expr after_exp_cont_op_noexpr after_exp_cont
        | data_expr where_opt after_exp_cont_op_noexpr after_exp_cont
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
        | load
        | set
        | _goto
        | call
        | insert
        | open
        | alter
        | if_else
        | _while
        | _label
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
        | '{'[u1] colref_exact '}'[u2] {
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

over_opt:
        | TOK_OVER[tk] '('[u1] sort_opt ')'[u2] {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1); YYUSE($u2);
        }
        ;

within_opt:
        | TOK_WITHIN[tk1] TOK_GROUP[tk2] '('[u1] sort_opt ')'[u2] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            YYUSE($u1); YYUSE($u2);
        }
        ;

expr_common:
          func over_opt within_opt
        | important_operator expr {
            sqli_store_data(ctx, &$important_operator);
        }
        | operator expr {
            YYUSE($operator);
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
        | TOK_CASE[tk1] TOK_WHEN[tk2] expr TOK_THEN[tk3] expr TOK_END[tk4] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$tk4);
        }
        | TOK_CASE[tk1] expr TOK_WHEN[tk2] expr TOK_THEN[tk3] expr TOK_END[tk4] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$tk4);
        }
        | TOK_CASE[tk1] TOK_WHEN[tk2] expr TOK_THEN[tk3] expr TOK_ELSE[tk4] expr TOK_END[tk5] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$tk4);
            sqli_store_data(ctx, &$tk5);
        }
        | TOK_CASE[tk1] expr TOK_WHEN[tk2] expr TOK_THEN[tk3] expr TOK_ELSE[tk4] expr TOK_END [tk5] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$tk4);
            sqli_store_data(ctx, &$tk5);
        }
        ;

op_expr:  expr_common
        | op_expr important_operator expr {
            sqli_store_data(ctx, &$important_operator);
        }
        | op_expr operator expr {
            YYUSE($operator);
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

func_name:  colref_exact
        | TOK_LEFT
        | TOK_DATABASE
        | TOK_IF
        | TOK_REPLACE
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

data_expr: colref_exact
        | colref_asterisk
        | data_expr important_operator expr {
            sqli_store_data(ctx, &$important_operator);
        }
        | data_expr operator expr {
            YYUSE($operator);
        }
        | data_expr post_exprs
        ;

expr:     expr_common
        | data_expr
        | expr important_operator expr {
            sqli_store_data(ctx, &$important_operator);
        }
        | expr operator expr {
            YYUSE($operator);
        }
        | expr post_exprs
        ;

operator: TOK_OPERATOR
        | '*'
        | '='
        | '.'
        | ':'
        ;

important_operator: TOK_OR
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
        | TOK_EXIST
        | TOK_AS
        | TOK_UESCAPE
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

select_args: select_distinct_opt
        | select_distinct_opt top_opt '*'[key] {
            sqli_store_data(ctx, &$key);
        }
        | select_distinct_opt top_opt select_list
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
        | TOK_FOR[tk] TOK_XML[u1] TOK_PATH[u2] '('[u3] data_name[elem] ')'[u4] {
            $$ = $tk;
            $$.value = (struct detect_str){CSTR_LEN("TOK_FOR TOK_XML TOK_PATH")};
            YYUSE($u1);
            YYUSE($u2);
            YYUSE($u3);
            sqli_store_data(ctx, &$elem);
            YYUSE($u4);
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

procedure:
          TOK_PROCEDURE[tk1] func {
            sqli_store_data(ctx, &$tk1);
        }
        ;

procedure_opt:
        | procedure
        ;

lock_opt:
        | TOK_LOCK[tk1] TOK_IN[tk2] TOK_SHARE[tk3] TOK_MODE[tk4] {
              sqli_store_data(ctx, &$tk1);
              sqli_store_data(ctx, &$tk2);
              sqli_store_data(ctx, &$tk3);
              sqli_store_data(ctx, &$tk4);
        }
        ;

select_after_where:
        group_opt having_opt sort_opt select_extras_opt procedure_opt lock_opt
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

select:   TOK_SELECT[tk] select_args into_opt from_opt
          where_opt select_after_where {
            sqli_store_data(ctx, &$tk);
        }
        | TOK_SELECT[tk] select_args from_opt into_opt
          where_opt select_after_where {
            sqli_store_data(ctx, &$tk);
        }
        | select union_c all_distinct_opt select_parens
        | select union_c all_distinct_opt execute
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
        | TOK_DECLARE[tk] var_list as_opt data_name[type] '='[u1] expr {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$type);
            YYUSE($u1);
        }
        | TOK_DECLARE[tk] var_list as_opt data_name[type] '('[u1] data_name[len] ')'[u2] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$type);
            YYUSE($u1);
            sqli_store_data(ctx, &$len);
            YYUSE($u2);
        }
        | TOK_DECLARE[tk1] data_name[name] TOK_CURSOR[tk2] TOK_FOR[tk3] select_parens {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$name);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
        }
        | TOK_DECLARE[tk] var_list as_opt data_name[type] '('[u1] data_name[len] ')'[u2] '='[u3] expr {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$type);
            YYUSE($u1);
            sqli_store_data(ctx, &$len);
            YYUSE($u2);
            YYUSE($u3);
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
        | TOK_EXECUTE[tk] '('[u1] data_name[name] ')'[u2] {
            sqli_store_data(ctx, &$tk);
            YYUSE($u1);
            sqli_store_data(ctx, &$name);
            YYUSE($u2);
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
        | TOK_DROP[tk1] TOK_DATABASE[tk2] func_name {
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

load_modifier:
          TOK_LOW_PRIORITY
        | TOK_CONCURRENT
        ;

load_modifier_opt:
        | load_modifier[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

local_opt:
        | TOK_LOCAL[tk] {
            sqli_store_data(ctx, &$tk);
        }
        ;

data_xml:
          TOK_DATA2
        | TOK_XML
        ;

load:
        TOK_LOAD[tk1] data_xml[tk2] load_modifier_opt local_opt TOK_INFILE[tk3] data_name[file_name] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$file_name);
        }
        ;

set:      TOK_SET[tk] expr {
            sqli_store_data(ctx, &$tk);
        }
        ;

_goto:    TOK_GOTO[tk] data_name[label] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$label);
        }
        ;

call: TOK_CALL[tk] func {
            sqli_store_data(ctx, &$tk);
        }
        ;

insert:   TOK_INSERT[tk1] TOK_INTO[tk2] colref_exact execute {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        | TOK_INSERT[tk1] TOK_INTO[tk2] colref_exact '('[u1] name_list ')'[u2] TOK_VALUES[tk3] '('[u3] func_args_list ')'[u4] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            YYUSE($u1);
            YYUSE($u2);
            sqli_store_data(ctx, &$tk3);
            YYUSE($u3);
            YYUSE($u4);
        }
        ;

open: TOK_OPEN[tk] data_name[name] {
            sqli_store_data(ctx, &$tk);
            sqli_store_data(ctx, &$name);
        }
        ;

alter: TOK_ALTER[tk1] TOK_DATABASE[tk2] data_name[name] TOK_SET[tk3] TOK_RECOVERY[tk4] TOK_SIMPLE[tk5] {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
            sqli_store_data(ctx, &$name);
            sqli_store_data(ctx, &$tk3);
            sqli_store_data(ctx, &$tk4);
            sqli_store_data(ctx, &$tk5);
        }
        ;

if_else:  TOK_IF[tk1] expr sql_parens {
            sqli_store_data(ctx, &$tk1);
        }
        | TOK_IF[tk1] expr sql_parens TOK_ELSE[tk2] sql_parens {
            sqli_store_data(ctx, &$tk1);
            sqli_store_data(ctx, &$tk2);
        }
        ;

_while:  TOK_WHILE[tk1] expr sql_parens {
            sqli_store_data(ctx, &$tk1);
        }
        ;

_label:   TOK_LABEL[tk] {
            YYUSE(&$tk);
        }
        ;

command:  TOK_ATTACH
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
        | sql_parens semicolons_opt multiple_sqls
        ;

select_after_where_optunion:
        select_after_where
        | select_after_where union_c all_distinct_opt select_parens
        | select_after_where union_c all_distinct_opt execute
        ;

after_exp_cont_op_noexpr:
        select_after_where_optunion
        ;

after_exp_cont_op:
        expr after_exp_cont_op_noexpr
        | where_opt after_exp_cont_op_noexpr
        ;

after_exp_cont:
        close_multiple_parens_opt semicolons_opt multiple_sqls
        | close_multiple_parens after_exp_cont_op after_exp_cont
        | close_multiple_parens ','[u1] select_list from_opt where_opt after_exp_cont {
            YYUSE($u1);
        }
        | after_exp_cont_op_noexpr close_multiple_parens after_exp_cont
        ;

start_rce_cont: close_multiple_parens_opt semicolons_opt multiple_sqls
        | close_multiple_parens_opt op_expr after_exp_cont
        | after_exp_cont_op_noexpr close_multiple_parens after_exp_cont
        | op_expr where_opt after_exp_cont_op_noexpr after_exp_cont
        ;

%%
