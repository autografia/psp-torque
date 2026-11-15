#include "obd/profile.h"
#include "test_util.h"

void test_parser(void);
void test_pid(void);
void test_boost(void);
void test_scheduler(void);
void test_samples(void);
void test_discovery(void);

int g_failures = 0;

static void test_profile(void)
{
    CHECK(PROFILE_QV_POLL_COUNT >= 5);
    CHECK(PROFILE_ENGINE_POLL_COUNT >= 10);
    CHECK(PROFILE_QV_POLL[0].pid == 0x0C);
    CHECK(PROFILE_QV_POLL[1].pid == 0x0B);
    CHECK(PROFILE_QV_POLL[0].period_ms == 100);
    CHECK(PROFILE_BARO_FALLBACK_KPA > 50.0f);
    CHECK(PROFILE_BARO_FALLBACK_KPA < 110.0f);
}

int main(void)
{
    test_parser();
    test_pid();
    test_boost();
    test_scheduler();
    test_samples();
    test_discovery();
    test_profile();

    if (g_failures != 0) {
        printf("%d comprobaciones fallidas\n", g_failures);
        return 1;
    }

    printf("OK: todas las pruebas pasan\n");
    return 0;
}
