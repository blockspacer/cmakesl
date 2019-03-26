#include "common/source_view.hpp"
#include "source_view.hpp"


namespace cmsl
{
    source_view::source_view(cmsl::string_view source)
        : source_view("<unknown source>", source)
    {}

    source_view::source_view(cmsl::string_view source_path, cmsl::string_view source)
        : m_path{ source_path }
        , m_source{ source }
    {}

    cmsl::string_view source_view::path() const
    {
        return m_path;
    }

    cmsl::string_view source_view::source() const
    {
        return m_source;
    }

    cmsl::string_view source_view::line(unsigned line_no) const
    {
        unsigned counter{ 1u };

        auto pos = cmsl::string_view::size_type{ 0u };

        while(counter < line_no)
        {
            pos = m_source.find('\n', pos);
            if(pos == cmsl::string_view::npos)
            {
                return "";
            }

            ++counter;
        }

        const auto line_end = [pos, source = m_source]
        {
            const auto p = source.find('\n', pos);
            return p != cmsl::string_view::npos ? p : source.size();
        }();

        const auto line_size = line_end - pos;
        const auto line_begin = std::next(m_source.cbegin(), pos);

        return cmsl::string_view{ line_begin, line_size };
    }
}
