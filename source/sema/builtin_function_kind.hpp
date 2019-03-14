#pragma once

namespace cmsl
{
    namespace sema
    {
        enum class builtin_function_kind
        {
            bool_ctor,
            bool_ctor_bool,
            bool_ctor_int,
            bool_operator_equal,
            bool_operator_equal_equal,
            bool_operator_pipe_pipe,
            bool_operator_amp_amp,
            bool_to_string,

            int_ctor,
            int_ctor_bool,
            int_ctor_int,
            int_ctor_double,
            int_to_string,
            int_operator_plus,
            int_operator_minus,
            int_operator_star,
            int_operator_slash,
            int_operator_equal,
            int_operator_plus_equal,
            int_operator_minus_equal,
            int_operator_star_equal,
            int_operator_slash_equal,
            int_operator_less,
            int_operator_less_equal,
            int_operator_greater,
            int_operator_greater_equal,
            int_operator_equal_equal,

            double_ctor,
            double_ctor_double,
            double_ctor_int,
            double_operator_plus,
            double_operator_minus,
            double_operator_star,
            double_operator_slash,
            double_operator_equal,
            double_operator_plus_equal,
            double_operator_minus_equal,
            double_operator_star_equal,
            double_operator_slash_equal,
            double_operator_less,
            double_operator_less_equal,
            double_operator_greater,
            double_operator_greater_equal,
            double_to_string,

            string_ctor,
            string_ctor_string,
            string_ctor_string_count,
            string_empty,
            string_size,
            string_operator_equal_equal,
            string_operator_not_equal,
            string_operator_less,
            string_operator_less_equal,
            string_operator_greater,
            string_operator_greater_equal,
            string_operator_plus,
            string_operator_plus_equal,
            string_clear,
            string_insert_pos_str,
            string_erase_pos,
            string_erase_pos_count,
            string_starts_with,
            string_ends_with,
            string_replace_pos_count_str,
            string_substr_pos,
            string_substr_pos_count,
            string_resize_newsize,
            string_resize_newsize_fill,
            string_find_str,
            string_find_str_pos,
            string_find_not_of_str,
            string_find_not_of_str_pos,
            string_find_last_str,
            string_find_last_not_of_str,
            string_contains,
            string_lower,
            string_make_lower,
            string_upper,
            string_make_upper,

            version_ctor_major,
            version_ctor_major_minor,
            version_ctor_major_minor_patch,
            version_ctor_major_minor_patch_tweak,
            version_operator_equal_equal,
            version_operator_not_equal,
            version_operator_less,
            version_operator_less_equal,
            version_operator_greater,
            version_operator_greater_equal,
            version_major,
            version_minor,
            version_patch,
            version_tweak,
            version_to_string,

            list_ctor

        };
    }
}
