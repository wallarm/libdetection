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
        {"rlike_mod_xor_regexp_binary", Tsqli_rlike_mod_xor_regexp_binary},
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
