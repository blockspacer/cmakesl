#include "exec/instance/project_value.hpp"
#include "exec/instance/list_value.hpp"
#include "exec/instance/instance.hpp"

#include "cmake_facade.hpp"
#include "project_value.hpp"


namespace cmsl
{
    namespace exec
    {
        namespace inst
        {
            project_value::project_value(const std::string& name)
                 : m_name{ name }
            {}

            std::string project_value::name() const
            {
                return m_name;
            }

            void project_value::add_executable(facade::cmake_facade& cmake_facade, const std::string &name, const list_value &sources)
            {
                const auto collected_sources = list_to_sources(sources);
                cmake_facade.add_executable(name, collected_sources);
            }

            void
            project_value::add_library(facade::cmake_facade &cmake_facade, const std::string &name, const list_value &sources)
            {
                const auto collected_sources = list_to_sources(sources);
                cmake_facade.add_library(name, collected_sources);
            }

            std::vector<std::string> project_value::list_to_sources(const list_value& sources)
            {
                std::vector<std::string> collected_sources;
                collected_sources.reserve(sources.size());

                for(auto i = 0u; i < sources.size(); ++i)
                {
                    const auto& instance = sources.at(i);
                    collected_sources.emplace_back(instance.get_value_cref().get_string_cref());
                }

                return collected_sources;
            }
        }
    }
}
