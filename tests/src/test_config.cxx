extern "C" {
#include "plugin_config.h"
}

#include <CppUTest/TestHarness.h>

/* clang-format off */
TEST_GROUP(test_config)
{
        void setup(void)
        {
        }

        void teardown(void)
        {
                config_reset_opts();
        }
};
/* clang-format on */

TEST(test_config, there_are_defaults)
{
        auto result = config_get(CONFIG_IGNORE_VERS).ignore_versions;
}

TEST(test_config, can_apply_arg)
{
        auto def = config_get(CONFIG_IGNORE_VERS).ignore_versions;
        CHECK_FALSE(def);

        enum config_opts o = config_apply_arg("ignore_ver", NULL);
        CHECK(CONFIG_UNKNOWN_KEY != o);

        auto changed = config_get(CONFIG_IGNORE_VERS).ignore_versions;
        CHECK_EQUAL(true, changed);
}

TEST(test_config, cant_apply_unknown_arg)
{
        enum config_opts o = config_apply_arg("there is no such flag", NULL);
        CHECK_EQUAL(CONFIG_UNKNOWN_KEY, o);
}

TEST(test_config, can_reset_arg)
{
        auto def = config_get(CONFIG_IGNORE_VERS).ignore_versions;
        CHECK_FALSE(def);

        enum config_opts o = config_apply_arg("ignore_ver", NULL);
        CHECK(CONFIG_UNKNOWN_KEY != o);

        auto changed = config_get(CONFIG_IGNORE_VERS).ignore_versions;
        CHECK_EQUAL(true, changed);

        config_reset_opts();

        auto after_reset = config_get(CONFIG_IGNORE_VERS).ignore_versions;
        CHECK_EQUAL(false, after_reset);
}
