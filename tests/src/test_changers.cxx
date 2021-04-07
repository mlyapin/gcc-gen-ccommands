#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include "changers.h"
#include "mocks/mock_changer.h"
}

#include "arg.h"
#include "cppdefs.h"

static changer_chain FCHAIN;

/* clang-format off */
TEST_GROUP(test_changer_chain){
        void setup(void)
        {
                changer_chain_init(&FCHAIN);
        }

        void teardown(void)
        {
                mock().clear();
                changer_chain_deinit(&FCHAIN);
        }
};
/* clang-format on */

TEST(test_changer_chain, can_append)
{
        size_t const count = 10;
        struct changer f;
        f.fn = NULL;
        f.data = NULL;
        for (unsigned i = 0; i < count; i++) {
                changer_chain_add(&FCHAIN, f);
        }

        CHECK(NULL != FCHAIN.changers);
        CHECK(count <= FCHAIN.changers_cap);
        CHECK_COMPARE(count, <=, FCHAIN.changers_cap);
        UNSIGNED_LONGS_EQUAL(count, FCHAIN.changers_len);
}

TEST(test_changer_chain, arg_passed)
{
        struct changer f = { .fn = mock_changer, .data = const_cast<char *>("changer") };

        CHECK(changer_chain_add(&FCHAIN, f));

        struct arg a;
        a.type = arg::ARG_NORMAL;
        a.normal.arg = "-I";

        a.normal.opts[0] = "/usr/include";
        a.normal.opts_len = 1;

        mock().expectOneCall("mock_changer")
                .withParameter("id", "changer")
                .withParameter("type", a.type)
                .withParameter("normal_arg", a.normal.arg)
                .withParameter("normal_opts_len", a.normal.opts_len)
                .withParameter("normal_opts", a.normal.opts)
                .andReturnValue(true);

        CHECK(changer_chain_handle(&FCHAIN, &a));

        mock().checkExpectations();
}

TEST(test_changer_chain, can_drop_a_flag)
{
        struct changer first = { .fn = mock_changer, .data = const_cast<char *>("first_changer") };
        struct changer second = { .fn = mock_changer,
                                  .data = const_cast<char *>("second_changer") };

        CHECK(changer_chain_add(&FCHAIN, first));
        CHECK(changer_chain_add(&FCHAIN, second));

        mock().expectOneCall("mock_changer")
                .withParameter("id", "first_changer")
                .ignoreOtherParameters()
                .andReturnValue(false);

        struct arg a unused;

        CHECK_FALSE(changer_chain_handle(&FCHAIN, &a));

        mock().checkExpectations();
}

/* clang-format off */
TEST_GROUP(test_changers)
{
        void setup(void)
        {}

        void teardown(void)
        {}
};
/* clang-format on */

TEST(test_changers, with_static)
{
        char origin_compiler[] = "/some/long/path/cc1";
        char new_compiler[] = "gcc";

        struct changer ch = { .fn = changer_replace_compiler_with_static, .data = new_compiler };

        struct arg a;
        a.type = arg::ARG_COMPILER_NAME;
        a.name = origin_compiler;
        changer_replace_compiler_with_static(&a, &ch);

        POINTERS_EQUAL(new_compiler, a.name);
}

TEST(test_changers, filterout_gccspecific)
{
        struct arg a;
        a.type = arg::ARG_NORMAL;
        a.normal.arg = "-iplugindir";

        CHECK_FALSE(changer_drop_gccspecific(&a, nullptr));
}

TEST(test_changers, filterout_gccinternal)
{
        struct arg a;
        a.type = arg::ARG_NORMAL;
        a.normal.arg = "-auxbase";
        CHECK_FALSE(changer_drop_internal(&a, nullptr));
}

TEST(test_changers, filterout_gccspecific_match)
{
        char const *flags[] = { "-fplugin", "-fplugin-arg-something-something" };

        for (size_t i = 0; i < ARRSIZE(flags); i++) {
                struct arg a;
                a.type = arg::ARG_NORMAL;
                a.normal.arg = flags[i];
                CHECK_FALSE(changer_drop_gccspecific(&a, nullptr));
        }
}

TEST(test_changers, dont_filterout_valid)
{
        struct arg a;
        a.type = arg::ARG_NORMAL;
        a.normal.arg = "-Wall";
        CHECK(changer_drop_gccspecific(&a, nullptr));
        CHECK(changer_drop_internal(&a, nullptr));
}

