extern "C" {
#include "paths.h"
}

#include <CppUTest/TestHarness.h>

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <filesystem>


static std::filesystem::path TMPDIR;
static void check_errno(void)
{
        if (errno != 0) {
                FAIL(std::strerror(errno));
        }
}

/* clang-format off */
TEST_GROUP(test_paths_relpath){
        void setup(void)
        {
                std::filesystem::path tmpdir = std::filesystem::temp_directory_path();
                char temp[] = "genccommands.XXXXXX";
                tmpdir /= temp;
                mkdtemp(const_cast<char*>(tmpdir.c_str()));
                TMPDIR = tmpdir;
        }

        void teardown(void)
        {
                std::filesystem::remove_all(TMPDIR);
        }
};
/* clang-format on */

TEST(test_paths_relpath, cut_part)
{
        char const *dir = "/usr/lib";
        char const *source = "/usr/local/bin/clang";
        char const *expected = "bin/clang";

        char const *actual = relpath(source, dir);
        check_errno();

        STRCMP_EQUAL(expected, actual);
}

TEST(test_paths_relpath, from_root)
{
        char const *dir = "/";
        char const *source = "/usr/local/bin/clang";
        char const *expected = "usr/local/bin/clang";

        char const *actual = relpath(source, dir);
        check_errno();

        STRCMP_EQUAL(expected, actual);
}

TEST(test_paths_relpath, nested)
{
        char const *dir = "/x/y/a/f";
        char const *source = "/x/y/z/file";
        char const *expected = "../../z/file";

        char const *actual = relpath(source, dir);
        check_errno();

        STRCMP_EQUAL(expected, actual);
}

static std::filesystem::path PREVPATH;
TEST_GROUP(test_paths_abspath){
        void setup(void)
        {
                PREVPATH = std::filesystem::current_path();
                if (0 != chdir("/")) {
                        check_errno();
                }
        }

        void teardown(void)
        {
                char *prev = const_cast<char*>(PREVPATH.c_str());
                if (0 != chdir(prev)) {
                        check_errno();
                }
        }
};

TEST(test_paths_abspath, preserve_abspath) {
        char const *origin = "c/d/e";
        char const *expected = "/a/b/c/d/e";

        char const *actual = abspath(origin);

        STRCMP_EQUAL(expected, actual);

        std::free(const_cast<char*>(actual));
}

TEST_GROUP(test_paths_construct_path)
{
        void setup(void)
        {}

        void teardown(void)
        {}
};

TEST(test_paths_construct_path, simple_wo_trailing_sep) {
        char const *dir = "/a/b";
        char const *relpath = "c/d/e";
        char const *expected = "/a/b/c/d/e";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}

TEST(test_paths_construct_path, simple_with_trailing_sep) {
        char const *dir = "/a/b/";
        char const *relpath = "c/d/e";
        char const *expected = "/a/b/c/d/e";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}

TEST(test_paths_construct_path, from_root) {
        char const *dir = "/";
        char const *relpath = "a/b/c/d";
        char const *expected = "/a/b/c/d";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}

TEST(test_paths_construct_path, backref_simple) {
        char const *dir = "/a/b/c/";
        char const *relpath = "../../c/d";
        char const *expected = "/a/c/d";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}

TEST(test_paths_construct_path, backref_too_much) {
        char const *dir = "/a/b/c";
        char const *relpath = "../../../../../c/d";
        char const *expected = "/c/d";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}

TEST(test_paths_construct_path, curref_simple) {
        char const *dir = "/a/b/";
        char const *relpath = "././c/d";
        char const *expected = "/a/b/c/d";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}

TEST(test_paths_construct_path, overcomplicated_case) {
        char const *dir = "/a/b/./.././b/c/d/";
        char const *relpath = ".././e/f";
        char const *expected = "/a/b/c/e/f";

        char const *actual = construct_path(dir, relpath);

        STRCMP_EQUAL(expected, actual);
        std::free(const_cast<char*>(actual));
}
