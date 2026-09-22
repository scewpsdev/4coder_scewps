/*
4coder_default_include.cpp - Default set of commands and setup used in 4coder.
*/

// TOP

#if !defined(FCODER_DEFAULT_INCLUDE_CPP)
#define FCODER_DEFAULT_INCLUDE_CPP

#if !defined(FCODER_TRANSITION_TO)
#define FCODER_TRANSITION_TO 0
#endif

#include "4coder_base_types.cpp"
#include "4coder_stringf.cpp"
#include "4coder_app_links_allocator.cpp"
#include "4coder_system_allocator.cpp"

#include "4coder_file.cpp"

#define DYNAMIC_LINK_API
#include "generated/custom_api.cpp"
#define DYNAMIC_LINK_API
#include "generated/system_api.cpp"
#include "4coder_system_helpers.cpp"
#include "4coder_layout.cpp"
#include "4coder_profile.cpp"
#include "4coder_profile_static_enable.cpp"
#include "4coder_events.cpp"
#include "4coder_custom.cpp"
#include "4coder_log.cpp"
#include "4coder_hash_functions.cpp"
#include "4coder_table.cpp"
#include "4coder_codepoint_map.cpp"
#include "4coder_async_tasks.cpp"
#include "4coder_string_match.cpp"
#include "4coder_buffer_seek_constructors.cpp"
#include "4coder_token.cpp"
#include "4coder_command_map.cpp"

#include "generated/lexer_cpp.cpp"

#include "4coder_default_map.cpp"
#include "4coder_mac_map.cpp"

#include "4coder_default_framework_variables.cpp"
#include "4coder_default_colors.cpp"

#define seek_beginning_of_line default_seek_beginning_of_line
#include "4coder_helper.cpp"
#undef seek_beginning_of_line

#include "4coder_delta_rule.cpp"
#include "4coder_layout_rule.cpp"
#include "4coder_code_index.cpp"
#include "4coder_fancy.cpp"
#include "4coder_draw.cpp"
#include "4coder_font_helper.cpp"
#include "4coder_config.cpp"
#include "4coder_scewps_config.cpp"
#include "4coder_dynamic_bindings.cpp"
#include "4coder_scewps_default_framework.cpp"
#include "4coder_clipboard.cpp"

#define run_lister default_run_lister
#define run_lister_with_refresh_handler default_run_lister_with_refresh_handler
#define lister_add_item default_lister_add_item
#define get_choice_from_user default_get_choice_from_user
#include "4coder_lister_base.cpp"
#undef run_lister
#undef run_lister_with_refresh_handler
#undef lister_add_item
#undef get_choice_from_user

#include "4coder_scewps_lister.cpp"

#define move_right_alpha_numeric_boundary default_move_right_alpha_numeric_boundary
#define move_left_alpha_numeric_boundary default_move_left_alpha_numeric_boundary
#define move_right_alpha_numeric_or_camel_boundary default_move_right_alpha_numeric_or_camel_boundary
#define move_left_alpha_numeric_or_camel_boundary default_move_left_alpha_numeric_or_camel_boundary
#define backspace_alpha_numeric_boundary default_backspace_alpha_numeric_boundary
#define delete_alpha_numeric_boundary default_delete_alpha_numeric_boundary
#define backspace_alpha_numeric_or_camel_boundary default_backspace_alpha_numeric_or_camel_boundary
#define delete_alpha_numeric_or_camel_boundary default_delete_alpha_numeric_or_camel_boundary
#define click_set_cursor_and_mark default_click_set_cursor_and_mark
#define click_set_cursor default_click_set_cursor
#define click_set_cursor_if_lbutton default_click_set_cursor_if_lbutton
#define mouse_wheel_scroll default_mouse_wheel_scroll
#define move_line_up default_move_line_up
#define move_line_down default_move_line_down
#define search default_search
#define reverse_search default_reverse_search

#include "4coder_scewps_base_commands.cpp"

#undef move_right_alpha_numeric_boundary
#undef move_left_alpha_numeric_boundary
#undef move_right_alpha_numeric_or_camel_boundary
#undef move_left_alpha_numeric_or_camel_boundary
#undef backspace_alpha_numeric_boundary
#undef delete_alpha_numeric_boundary
#undef backspace_alpha_numeric_or_camel_boundary
#undef delete_alpha_numeric_or_camel_boundary
#undef click_set_cursor_and_mark
#undef click_set_cursor
#undef click_set_cursor_if_lbutton
#undef mouse_wheel_scroll
#undef move_line_up
#undef move_line_down
#undef search
#undef reverse_search

#include "4coder_insertion.cpp"
#include "4coder_eol.cpp"

#define command_lister default_command_lister
#define theme_lister default_theme_lister
#include "4coder_scewps_lists.cpp"
#undef command_lister
#undef theme_lister

#include "4coder_auto_indent.cpp"
#include "4coder_scewps_search.cpp"
#include "4coder_scewps_completion.cpp"
#include "4coder_jumping.cpp"
#include "4coder_jump_sticky.cpp"
#include "4coder_jump_lister.cpp"

#define jump_to_definition_at_cursor default_jump_to_definition_at_cursor
#include "4coder_code_index_listers.cpp"
#undef jump_to_definition_at_cursor

#include "4coder_log_parser.cpp"
#include "4coder_keyboard_macro.cpp"
#include "4coder_scewps_cli_command.cpp"
#include "4coder_build_commands.cpp"
#include "4coder_project_commands.cpp"
#include "4coder_prj_v1.cpp"
#include "4coder_function_list.cpp"
#include "4coder_scope_commands.cpp"
#include "4coder_combined_write_commands.cpp"
#include "4coder_miblo_numbers.cpp"
#include "4coder_profile_inspect.cpp"
#include "4coder_tutorial.cpp"
#include "4coder_doc_content_types.cpp"
#include "4coder_doc_commands.cpp"
#include "4coder_docs.cpp"
#include "4coder_variables.cpp"
#include "4coder_audio.cpp"
#include "4coder_search_list.cpp"

#include "4coder_examples.cpp"

#include "4coder_default_hooks.cpp"

#endif

// BOTTOM

