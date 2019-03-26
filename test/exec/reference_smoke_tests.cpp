#include "exec/source_executor.hpp"
#include "common/source_view.hpp"
#include "test/exec/mock/cmake_facade_mock.hpp"

#include <gmock/gmock.h>

namespace cmsl
{
    namespace exec
    {
        namespace test
        {
            using ::testing::Eq;

            class ReferenceTypeSmokeTest : public ::testing::Test
            {
            protected:
                cmake_facade_mock m_facade;
                source_executor m_executor{m_facade};
            };

            TEST_F(ReferenceTypeSmokeTest, BuiltinType)
            {
                const auto source =
                        "int foo(int& ref)\n"
                        "{\n"
                        "    ref = 42;\n"
                        "    return 0;"
                        "}\n"
                        "\n"
                        "int& set_and_get_ref(int& ref)\n"
                        "{\n"
                        "    ref = 24;\n"
                        "    return ref;\n"
                        "}\n"
                        "\n"
                        "int main()\n"
                        "{\n"
                        "    int i;\n"
                        "    int& i_ref = i;\n"
                        "    bool result = true;\n"
                        "    result = result && i == 0 && i_ref == 0;\n"
                        "\n"
                        "    i = 42;\n"
                        "    result = result && i == 42 && i_ref == 42;\n"
                        "\n"
                        "    i_ref = 24;\n"
                        "    result = result && i == 24 && i_ref == 24;\n"
                        "\n"
                        "    foo(i_ref);\n"
                        "    result = result && i == 42 && i_ref == 42;\n"
                        "\n"
                        "    int& got_ref = set_and_get_ref(i);\n"
                        "    result = result && i == 24 && i_ref == 24 && got_ref == 24;\n"
                        "\n"
                        "    got_ref = 42;\n"
                        "    result = result && i == 42 && i_ref == 42 && got_ref == 42;\n"
                        "\n"
                        "    return int(result);\n"
                        "}";
                const auto result = m_executor.execute2(cmsl::source_view{ source });
                EXPECT_THAT(result, Eq(1));
            }

            TEST_F(ReferenceTypeSmokeTest, UserType)
            {
                const auto source =
                        "class user_class\n"
                        "{\n"
                        "    int v;\n"
                        "};\n"
                        "\n"
                        "int foo(user_class& ref)\n"
                        "{\n"
                        "    ref.v = 42;\n"
                        "    return 0;\n"
                        "}\n"
                        "\n"
                        "user_class& set_and_get_ref(user_class& ref)\n"
                        "{\n"
                        "    ref.v = 24;\n"
                        "    return ref;\n"
                        "}\n"
                        "\n"
                        "int main()\n"
                        "{\n"
                        "    user_class c;\n"
                        "    user_class& c_ref = c;\n"
                        "    bool result = true;\n"
                        "    result = result && c.v == 0 && c_ref.v == 0;\n"
                        "\n"
                        "    c.v = 42;\n"
                        "    result = result && c.v == 42 && c_ref.v == 42;\n"
                        "\n"
                        "    c_ref.v = 24;\n"
                        "    result = result && c.v == 24 && c_ref.v == 24;\n"
                        "\n"
                        "    foo(c_ref);\n"
                        "    result = result && c.v == 42 && c_ref.v == 42;\n"
                        "\n"
                        "    user_class& got_ref = set_and_get_ref(c);\n"
                        "    result = result && c.v == 24 && c_ref.v == 24 && got_ref.v == 24;\n"
                        "\n"
                        "    got_ref.v = 42;\n"
                        "    result = result && c.v == 42 && c_ref.v == 42 && got_ref.v == 42;\n"
                        "\n"
                        "    return int(result);\n"
                        "}";
                const auto result = m_executor.execute2(cmsl::source_view{ source });
                EXPECT_THAT(result, Eq(1));
            }
        }
    }
}
