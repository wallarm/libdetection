#include <CUnit/Basic.h>
#include <detect/detect.h>
#include <stdlib.h>

#define STR_LEN_ARGS(str) str, sizeof(str) - 1

#define s_sqli_attacks(...)                                    \
    s_type_attacks(                                            \
        "sqli",                                                \
        (const struct detect_str []){__VA_ARGS__},             \
        sizeof((const struct detect_str []){__VA_ARGS__})      \
            / sizeof(const struct detect_str))

static void
s_type_attacks(
    const char *typename,
    const struct detect_str *tests, size_t ntests)
{
    struct detect *detect;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open(typename));
    for (size_t i = 0; i != ntests; i++) {
        uint32_t attack_types;

        CU_ASSERT_EQUAL(detect_start(detect), 0);
        CU_ASSERT_EQUAL(
            detect_add_data(detect, tests[i].str, tests[i].len, true), 0);
        CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
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
Tsqli_union_distinct(void)
{
    s_sqli_attacks({CSTR_LEN("1' union distinct select 1")});
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
        {CSTR_LEN("1 MATCH(col) AGAINST('text')")},
        {CSTR_LEN("1 AND EXIST(SELECT 1)")},
        {CSTR_LEN("1 AND xmlelement('user', "
                  "login || ':' || pass).getStringVal()")},
        {CSTR_LEN("1 AND FileToClob('/etc/passwd', "
                  "'server')::html")},
        {CSTR_LEN("1 AND U&'pgsql evade' uescape '!'")},
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
    s_sqli_attacks({CSTR_LEN("1; DBMS_LOCK.SLEEP()")});
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
    s_sqli_attacks({CSTR_LEN("SELECT 1 FROM table_name INTO OUTFILE 1")});
}

static void
Tsqli_declare(void)
{
    s_sqli_attacks(
        {CSTR_LEN("1; DECLARE name varchar(42)")},
        {CSTR_LEN("1; DECLARE name CURSOR FOR select 1")},
    );
    s_sqli_attacks({CSTR_LEN("1; DECLARE name varchar(42)")});
}

static void
Tsqli_execute(void)
{
    s_sqli_attacks({CSTR_LEN("EXEC master.dbo.xp_cmdshell 'cmd'")});
}

static void
Tsqli_nul_in_str(void)
{
    s_sqli_attacks({CSTR_LEN("1\0' ^ '")});
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
    s_sqli_attacks({CSTR_LEN("SET @t=1")});
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
        {"union_distinct", Tsqli_union_distinct},
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
        CU_TEST_INFO_NULL
    };
    CU_SuiteInfo suites[] = {
        {.pName = "generic", .pTests = generic_tests},
        {.pName = "sqli", .pTests = sqli_tests,
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
