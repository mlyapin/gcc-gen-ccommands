extern "C" {
#include "saver.h"
}

#include "cppdefs.h"

#include <CppUTest/TestHarness.h>

#include <fstream>
#include <filesystem>
#include <unistd.h>

static std::filesystem::path ourtempfile;

static auto get_tempfile_contents(void)
{
        std::ifstream ifs(ourtempfile);
        auto it = std::istreambuf_iterator<char>(ifs);
        auto eos_it = std::istreambuf_iterator<char>();
        return (std::string(it, eos_it));
}

/* clang-format off */
TEST_GROUP(test_saver)
{
        void setup(void)
        {
                auto t = std::filesystem::temp_directory_path() / "genccommands_saver.XXXXXX";
                ourtempfile = std::filesystem::path(mktemp(const_cast<char*>(t.c_str())));
        }

        void teardown(void)
        {
                std::filesystem::remove(ourtempfile);
        }
};
/* clang-format on */

TEST(test_saver, simple)
{
        struct saver jsaver;
        CHECK(saver_init(&jsaver, "/somedir", "somefile", SFLAGS_NONE));

        CHECK(saver_save(&jsaver, ourtempfile.c_str()));
        saver_deinit(&jsaver);

        char const *expected_json =
                "[{\"file\": \"somefile\", \"directory\": \"/somedir\", \"arguments\": []}]";
        STRCMP_EQUAL(expected_json, get_tempfile_contents().c_str());
}

TEST(test_saver, style_arguments)
{
        struct saver jsaver;
        CHECK(saver_init(&jsaver, "/somedir", "somefile", SFLAGS_NONE));

        struct arg arg0;
        arg0.type = arg::ARG_NORMAL;
        arg0.normal.arg = "-test";
        arg0.normal.opts[0] = "somearg";
        arg0.normal.opts_len = 1;

        CHECK(saver_append(&jsaver, &arg0));

        struct arg arg1;
        arg1.type = arg::ARG_NORMAL;
        arg1.normal.arg = "-another";
        arg1.normal.opts[0] = "one";
        arg1.normal.opts_len = 1;

        CHECK(saver_append(&jsaver, &arg1));

        CHECK(saver_save(&jsaver, ourtempfile.c_str()));
        saver_deinit(&jsaver);

        char const *expected_json =
                "[{\"file\": \"somefile\", \"directory\": \"/somedir\","
                " \"arguments\": [\"-test\", \"somearg\", \"-another\", \"one\"]}]";
        STRCMP_EQUAL(expected_json, get_tempfile_contents().c_str());
}

TEST(test_saver, style_command)
{
        struct saver jsaver;
        CHECK(saver_init(&jsaver, "/somedir", "somefile", SFLAGS_STYLE_COMMAND));

        struct arg arg0;
        arg0.type = arg::ARG_NORMAL;
        arg0.normal.arg = "-test";
        arg0.normal.opts[0] = "somearg";
        arg0.normal.opts_len = 1;

        CHECK(saver_append(&jsaver, &arg0));

        struct arg arg1;
        arg1.type = arg::ARG_NORMAL;
        arg1.normal.arg = "-another";
        arg1.normal.opts[0] = "one";
        arg1.normal.opts_len = 1;

        CHECK(saver_append(&jsaver, &arg1));

        CHECK(saver_save(&jsaver, ourtempfile.c_str()));
        saver_deinit(&jsaver);

        char const *expected_json = "[{\"file\": \"somefile\", \"directory\": \"/somedir\","
                                    " \"command\": \"-test somearg -another one \"}]";
        STRCMP_EQUAL(expected_json, get_tempfile_contents().c_str());
}
