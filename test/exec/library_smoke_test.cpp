#include "test/exec/smoke_test_fixture.hpp"

#include <gmock/gmock.h>

namespace cmsl::exec::test {
using ::testing::Eq;

using LibrarySmokeTest = ExecutionSmokeTest;

TEST_F(LibrarySmokeTest, NameReturnsProperName)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"some name\", sources);"
    ""
    "    string name = lib.name();"
    "    return int(name == \"some name\");"
    "}";

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(1));
}

TEST_F(LibrarySmokeTest, LinkTo_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    ""
    "    cmake::library some_lib = p.add_library(\"some name\", sources);"
    ""
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    "    lib.link_to(some_lib);"
    ""
    "    return 42;"
    "}";

  EXPECT_CALL(
    m_facade,
    target_link_library("lib", facade::visibility::private_, "some name"));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, LinkToWithVisibility_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    ""
    "    cmake::library some_lib = p.add_library(\"some name\", sources);"
    ""
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    "    lib.link_to(some_lib, cmake::visibility::public);"
    ""
    "    return 42;"
    "}";

  EXPECT_CALL(
    m_facade,
    target_link_library("lib", facade::visibility::public_, "some name"));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, IncludeDirectories_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> include_dirs = { \"dir\" };"
    "    lib.include_directories(include_dirs);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_dirs = { "dir" };
  EXPECT_CALL(m_facade,
              target_include_directories("lib", facade::visibility::private_,
                                         expected_dirs));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, IncludeDirectoriesWithVisibility_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> include_dirs = { \"dir\" };"
    "    lib.include_directories(include_dirs, cmake::visibility::public);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_dirs = { "dir" };
  EXPECT_CALL(m_facade,
              target_include_directories("lib", facade::visibility::public_,
                                         expected_dirs));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, CompileDefinitions_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> defs = { \"-DFOO\" };"
    "    lib.compile_definitions(defs);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_definitions = { "-DFOO" };
  EXPECT_CALL(m_facade,
              target_compile_definitions("lib", facade::visibility::private_,
                                         expected_definitions));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, CompileDefinitionsWithVisibility_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> defs = { \"-DFOO\" };"
    "    lib.compile_definitions(defs, cmake::visibility::public);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_definitions = { "-DFOO" };
  EXPECT_CALL(m_facade,
              target_compile_definitions("lib", facade::visibility::public_,
                                         expected_definitions));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, CompileOptions_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> options = { \"-Wall\" };"
    "    lib.compile_options(options);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_options = { "-Wall" };
  EXPECT_CALL(m_facade,
              target_compile_options("lib", facade::visibility::private_,
                                     expected_options));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, CompileOptionsWithVisibility_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> options = { \"-Wall\" };"
    "    lib.compile_options(options, cmake::visibility::public);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_options = { "-Wall" };
  EXPECT_CALL(m_facade,
              target_compile_options("lib", facade::visibility::public_,
                                     expected_options));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, AddSources_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> sources = { \"foo.cpp\" };"
    "    lib.add_sources(sources);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_options = { "foo.cpp" };
  EXPECT_CALL(
    m_facade,
    target_sources("lib", facade::visibility::private_, expected_options));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}

TEST_F(LibrarySmokeTest, AddSourcesWithVisibility_CallsFacadeMethod)
{
  const auto source =
    "int main()"
    "{"
    "    cmake::project p = cmake::project(\"foo\");"
    "    list<string> sources;"
    "    cmake::library lib = p.add_library(\"lib\", sources);"
    ""
    "    list<string> sources = { \"foo.cpp\" };"
    "    lib.add_sources(sources, cmake::visibility::public);"
    "    return 42;"
    "}";

  std::vector<std::string> expected_options = { "foo.cpp" };
  EXPECT_CALL(
    m_facade,
    target_sources("lib", facade::visibility::public_, expected_options));

  const auto result = m_executor->execute(source);
  EXPECT_THAT(result, Eq(42));
}
}
