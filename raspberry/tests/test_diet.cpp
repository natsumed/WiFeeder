#include "config.h"
#include "diet_engine.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <vector>

#ifdef HAVE_GTEST
#include <gtest/gtest.h>
#else
#define EXPECT_EQ(a, b) do { if ((a) != (b)) { std::cerr << "FAIL " << __LINE__ << std::endl; ++g_failures; } } while (0)
#define EXPECT_NEAR(a, b, eps) do { if (std::fabs((a) - (b)) > (eps)) { std::cerr << "FAIL " << __LINE__ << std::endl; ++g_failures; } } while (0)
static int g_failures = 0;
#endif

using namespace wifeeder;

static JsonValue make_test_diet()
{
    JsonValue diet;
    diet["diet_id"] = static_cast<unsigned int>(10);
    JsonValue target;
    JsonValue feed;
    feed["feed_id"] = static_cast<unsigned int>(0);
    feed["start"] = "00:01:00";
    feed["end"] = "23:59:00";
    feed["inter"] = static_cast<uint64_t>(3600);
    feed["d_max"] = static_cast<unsigned int>(5000);
    feed["d_qty"] = static_cast<unsigned int>(3000);
    feed["p_max"] = static_cast<unsigned int>(500);
    feed["density"] = static_cast<unsigned int>(480);
    target.append(feed);
    diet["target"] = target;

    JsonValue cons;
    JsonValue cons_item;
    cons_item["feed_id"] = static_cast<unsigned int>(0);
    cons_item["ts"] = static_cast<uint64_t>(1);
    cons_item["d_eff"] = static_cast<unsigned int>(0);
    cons_item["p_qty"] = static_cast<unsigned int>(0);
    cons_item["p_eff"] = static_cast<unsigned int>(0);
    cons_item["missed"] = static_cast<unsigned int>(0);
    cons.append(cons_item);
    diet["cons"] = cons;
    return diet;
}

#ifdef HAVE_GTEST
TEST(DietEngine, GramsToRevsCalibration) {
    EXPECT_EQ(16U, grams_to_revs(480, 480));
}

TEST(DietEngine, RevsToGramsCalibration) {
    EXPECT_EQ(480U, revs_to_grams(16, 480));
}

TEST(DietEngine, RevsToGramsHalfDensity) {
    EXPECT_NEAR(240.0, static_cast<double>(revs_to_grams(16, 240)), 0.01);
}

TEST(DietEngine, GetPortionWithinFeedWindow) {
    DietEngine diet(make_test_diet());
    std::vector<uint32_t> portions;
    EXPECT_EQ(ENOERR, diet.get_portion(portions));
    ASSERT_FALSE(portions.empty());
    EXPECT_LE(portions[0], 500U);
}

TEST(DietEngine, LogPortionUpdatesConsumption) {
    DietEngine diet(make_test_diet());
    std::vector<uint32_t> served = {100};
    EXPECT_EQ(ENOERR, diet.log_portion(served));
    JsonValue cons = diet.cons_json();
    EXPECT_EQ(100U, cons["cons"][static_cast<size_t>(0)]["d_eff"].asUInt());
}

TEST(DietEngine, DailyResetClearsDailyEff) {
    DietEngine diet(make_test_diet());
    diet.log_portion({500});
    if (std::time(nullptr) >= daytime_to_ts("23:50:00")) {
        diet.daily_reset();
        JsonValue cons = diet.cons_json();
        EXPECT_EQ(0U, cons["cons"][static_cast<size_t>(0)]["d_eff"].asUInt());
    } else {
        EXPECT_EQ(ENOERR, diet.daily_reset());
    }
}

TEST(DietEngine, RevolutionRoundTrip) {
    const uint32_t revs = 50;
    const uint32_t density = 500;
    const uint32_t grams = revs_to_grams(revs, density);
    EXPECT_EQ(1550U, grams);
}
#else
static void test_grams_revs()
{
    EXPECT_EQ(16U, grams_to_revs(480, 480));
    EXPECT_EQ(480U, revs_to_grams(16, 480));
    EXPECT_NEAR(240.0, static_cast<double>(revs_to_grams(16, 240)), 0.01);
}

static void test_get_portion()
{
    DietEngine diet(make_test_diet());
    std::vector<uint32_t> portions;
    if (diet.get_portion(portions) != ENOERR || portions.empty()) {
        ++g_failures;
    }
    if (portions[0] > 500U) {
        ++g_failures;
    }
}

static void test_log_and_reset()
{
    DietEngine diet(make_test_diet());
    diet.log_portion({100});
    JsonValue cons = diet.cons_json();
    if (cons["cons"][static_cast<size_t>(0)]["d_eff"].asUInt() != 100U) {
        ++g_failures;
    }
    diet.daily_reset();
    if (std::time(nullptr) >= daytime_to_ts("23:50:00")) {
        cons = diet.cons_json();
        if (cons["cons"][static_cast<size_t>(0)]["d_eff"].asUInt() != 0U) {
            ++g_failures;
        }
    }
}
#endif

#ifndef HAVE_GTEST
int main()
{
    test_grams_revs();
    test_get_portion();
    test_log_and_reset();
    if (g_failures != 0) {
        std::cerr << g_failures << " test failures" << std::endl;
        return 1;
    }
    std::cout << "All diet tests passed" << std::endl;
    return 0;
}
#endif
