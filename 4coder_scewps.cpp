/*
4coder_scewps.cpp - Supplies the bindings used for default 4coder behavior.
*/

/*

	TODO

	[X] brace lines
	[X] 4coder highlighting
	[X] go to definition
	[X] select + search -> insert string
	[X] token occurance underline
	[X] focus color theme
	[X] error annotations
	[X] stop seek next identifier at beginning or end of line
	[X] higher cursor
	[X] fix cursor
	[X] finish error annotations
	[X] go to definition in other panel
	[X] completion list
	[X] function signature help
	[X] completion ignore case
	[X] completion display perfect match at top
	[X] fix argument underline
	[ ] selection ctrl move end after identifier
	[ ] error underline
	[ ] simplify custom layer
	[X] ui font
	[ ] autocomplete
	[ ] better bindings file syntax
	[X] fix cursor disappearing bug
	[X] fix alphanumeric boundary movement/deletion
	[X] scroll quarter page
	[X] cursor
		[X] block cursor for notepad style
		[X] smooth cursor
		[X] blinking cursor (timeout after a while?)
	[X] filebar
		[X] at bottom
		[X] better info (file, state, line, col, line endings, language)
	[X] line numbers
		[X] side padding
		[X] highlight line number at current line
		[X] line numbers same background
	[X] editing
		[X] proper home
		[X] move selected lines with alt
		[X] block select
		[X] slower autoscroll
	[X] hot reloading
		[X] config
		[X] bindings
		[X] themes
			[X] smooth interpolate
			[X] preview on lister hover
	[X] lister
		[X] floating lister
		[X] darken buffer behind lister
		[X] ctrl+backspace
		[X] backspace on path deletes whole directory
		[X] display keybindings on right
		[X] display commands as capitalized words
		[X] preselect last command (or put at top)
		[X] lister description status bar
	[ ] language support
		[X] c++
		[X] 4coder
		[ ] glsl
		[ ] batch
		[X] snek
		[X] highlighting
			[X] function
			[X] type
			[X] operator
			[X] comments
			[X] numbers
			[X] strings
			[X] literals
		[X] go to definition
		[ ] preview function signature
		[ ] code peek
		[X] code index
	[X] highlight current parens
	[ ] hex color preview
	[ ] comment dividers
	[ ] todo buffer
	[ ] minimap
	[ ] relative line numbers
	[ ] vim mode


	Commands:
	- quarter_page_down
	- quarter_page_up
	- delete_rect: deletes text in block mode
	- jump_to_definition_at_cursor_other_panel
	- backspace_alpha_numeric_or_camel_boundary
	- delete_alpha_numeric_or_camel_boundary

	Colors
	- defcolor_line_numbers_highlight
	- defcolor_brace_highlight
	- defcolor_brace_line
	- defcolor_symbol_highlight
	- defcolor_error
	- defcolor_warning

	Config settings:
	- b32 interpolate_cursor
	- u64 lister_item_height
	- u64 lister_width
	- u64 lister_height
	- u64 lister_panel_roundness
	- u64 lister_item_roundness
	- u64 buffer_margin
	- u64 lister_margin
	- u64 lister_inner_margin
	- u64 lister_item_margin
	- string ui_font_name
	- u64 ui_font_size

	Fixes:
	- fixed seek_beginning_of_line: scrolls view to left side and toggles between first non-whitespace character and actual beginning
	- fixed move_line_up: supports selections
	- fixed move_line_down: supports selections
	- fixed mouse_wheel_scroll: scrolls hovered view instead of active
	- fixed selection being cancelled when opening command lister


*/

// TOP

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_scewps_include.h"
#include "4coder_fleury_lang.h"
#include "4coder_fleury_index.h"
#include "4coder_fleury_ubiquitous.h"
#include "4coder_fleury_colors.h"

// custom globals
global Face_ID small_code_font = 0;
global Face_ID ui_font = 0;

global Rect_f32 current_cursor_rect;
global Rect_f32 next_cursor_rect;
global f32 cursor_blink_acc;
global b32 cursor_blink_state;
global u32 cursor_blink_idx;
global b32 cursor_blink_paused;

global f32 color_transition;
global Color_Table current_color_table;
global Color_Table last_color_table;
global Color_Table next_color_table;

global i32 lister_open;
global View_ID lister_view;
global Custom_Command_Function* last_used_command;

global Word_Complete_Menu complete_menu;

global F4_Language_State f4_langs;

// NOTE(allen): Users can declare their own managed IDs here.
CUSTOM_ID(colors, defcolor_line_numbers_highlight);
CUSTOM_ID(colors, defcolor_brace_highlight);
CUSTOM_ID(colors, defcolor_brace_line);
CUSTOM_ID(colors, defcolor_symbol_highlight);
CUSTOM_ID(colors, defcolor_error);
CUSTOM_ID(colors, defcolor_warning);

CUSTOM_ID(colors, fleury_color_syntax_crap);
CUSTOM_ID(colors, fleury_color_operators);
CUSTOM_ID(colors, fleury_color_inactive_pane_overlay);
CUSTOM_ID(colors, fleury_color_inactive_pane_background);
CUSTOM_ID(colors, fleury_color_file_progress_bar);
CUSTOM_ID(colors, fleury_color_brace_highlight);
CUSTOM_ID(colors, fleury_color_brace_line);
CUSTOM_ID(colors, fleury_color_brace_annotation);
CUSTOM_ID(colors, fleury_color_index_sum_type);
CUSTOM_ID(colors, fleury_color_index_product_type);
CUSTOM_ID(colors, fleury_color_index_function);
CUSTOM_ID(colors, fleury_color_index_macro);
CUSTOM_ID(colors, fleury_color_index_constant);
CUSTOM_ID(colors, fleury_color_index_comment_tag);
CUSTOM_ID(colors, fleury_color_index_decl);
CUSTOM_ID(colors, fleury_color_cursor_macro);
CUSTOM_ID(colors, fleury_color_cursor_power_mode);
CUSTOM_ID(colors, fleury_color_cursor_inactive);
CUSTOM_ID(colors, fleury_color_plot_cycle);
CUSTOM_ID(colors, fleury_color_token_highlight);
CUSTOM_ID(colors, fleury_color_token_minor_highlight);
CUSTOM_ID(colors, fleury_color_comment_user_name);
CUSTOM_ID(colors, fleury_color_lego_grab);
CUSTOM_ID(colors, fleury_color_lego_splat);
CUSTOM_ID(colors, fleury_color_error_annotation);

#include "4coder_scewps_include.cpp"

// custom files

#include "4coder_fleury_lang.cpp"
#include "4coder_fleury_index.cpp"
#include "4coder_fleury_lang_list.h"
#include "4coder_fleury_ubiquitous.cpp"
#include "4coder_fleury_colors.cpp"

#include "4coder_scewps_theme.cpp"
#include "4coder_scewps_commands.cpp"
#include "4coder_scewps_draw.cpp"
#include "4coder_scewps_hooks.cpp"

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

void
custom_layer_init(Application_Links* app) {
	Thread_Context* tctx = get_thread_context(app);

	// NOTE(allen): setup for default framework
	default_framework_init(app);

	// NOTE(allen): default hooks and command maps
	set_all_default_hooks(app);

	// custom hooks
	set_custom_hook(app, HookID_ViewEventHandler, sc_view_input_handler);
	set_custom_hook(app, HookID_Tick, sc_tick);
	set_custom_hook(app, HookID_DeltaRule, original_delta);
	set_custom_hook_memory_size(app, HookID_DeltaRule, delta_ctx_size(original_delta_memory_size));
	set_custom_hook(app, HookID_RenderCaller, sc_render);
	set_custom_hook(app, HookID_WholeScreenRenderCaller, sc_whole_screen_render_caller);
	set_custom_hook(app, HookID_Layout, sc_layout);
	set_custom_hook(app, HookID_BeginBuffer, sc_begin_buffer);
	set_custom_hook(app, HookID_BufferRegion, sc_buffer_region);
	set_custom_hook(app, HookID_BufferEditRange, sc_buffer_edit);
	set_custom_hook(app, HookID_SaveFile, sc_file_save);

	mapping_init(tctx, &framework_mapping);

	String_ID global_map_id = vars_save_string_lit("keys_global");
	String_ID file_map_id = vars_save_string_lit("keys_file");
	String_ID code_map_id = vars_save_string_lit("keys_code");

#if OS_MAC
	setup_mac_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#else
	setup_default_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#endif

	sc_setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);

	F4_Index_Initialize();
	F4_RegisterLanguages();
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

