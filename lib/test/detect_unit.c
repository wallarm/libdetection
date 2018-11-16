#include <CUnit/Basic.h>
#include <detect/detect.h>
#include <stdlib.h>

#define STR_LEN_ARGS(str) str, sizeof(str) - 1

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
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1' where 1=1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_union_distinct(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1' union distinct select 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_rlike_mod_xor_regexp_binary(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1' RLIKE '.*'"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1 MOD 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1 XOR 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1' REGEXP '.*'"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1' + BINARY '1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1' INTO OUTFILE '1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1 AND 1 SOUNDS LIKE 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_begin_end(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1'; BEGIN UPDATE table_name SET column1 = value1 END"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_waitfor(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1'; WAITFOR DELAY '02:00'"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1'; WAITFOR TIME '02:00'"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_top(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("SELECT TOP 5 * FROM table_name"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_identifier_quote(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(detect_add_data(detect,
                                    STR_LEN_ARGS("SELECT * FROM `select`"),
                                    true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_whitespace(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1\vor\v1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1\u00A0or\u00A01"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_null(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1-\\N union select 1"), true),0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_left_func(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1 AND left(a, 2) = 'ab'"), true),
        0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_func(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1; DBMS_LOCK.SLEEP()"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_var_start_with_dollar(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1 AND $func(a, 2) = 'ab'"), true),
        0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_create_func(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect,
                        STR_LEN_ARGS("1; CREATE OR REPLACE FUNCTION SLEEP(int) \
                                      RETURNS int AS '/lib/libc.so.6','sleep'  \
                                      language 'C' STRICT"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_bit_num(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("0b01UNION SELECT 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("B01UNION ALL SELECT 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("0b'01'UNION SELECT 1"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("B'01' UNION ALL SELECT 1"), true), 0);
}

static void
Tsqli_join_wo_join_qual(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
      detect_add_data(detect,
                      STR_LEN_ARGS("SELECT * FROM (SELECT * FROM (SELECT 1) \
                                    as t JOIN (SELECT 2)b)a"), true), 0);
  CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
  CU_ASSERT_EQUAL(detect_stop(detect), 0);
  CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_empty_schema(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("SELECT 1 FROM master..sysdatabases"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_asc_desc(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("SELECT 1 ORDER BY 1 ASC"), true),
        0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("SELECT 1 ORDER BY 1 DESC"), true),
        0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_shutdown(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1; SHUTDOWN"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
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
        {"rlike_mod_xor_regexp_binary", Tsqli_rlike_mod_xor_regexp_binary},
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
