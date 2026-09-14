/*
4coder_scewps.cpp - Supplies the bindings used for default 4coder behavior.
*/

/*

	TODO

	[ ] fix mouse scroll speed
	[X] scroll quarter page
	[ ] better bindings file syntax
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
	[ ] editing
		[X] proper home
		[X] move selected lines with alt
		[X] block select
		[X] slower autoscroll
		[ ] fix alphanumeric boundary movement/deletion
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
		[ ] c++
		[ ] 4coder
		[ ] snek
		[ ] highlighting
			[ ] function
			[ ] type
			[ ] operator
			[ ] comments
			[ ] numbers
			[ ] strings
			[ ] literals
		[ ] go to definition
		[ ] preview function signature
		[ ] code peek
		[ ] code index
	[ ] hex color preview
	[ ] comment dividers
	[ ] brace lines
	[ ] highlight current parens
	[ ] token occurance underline
	[ ] todo buffer
	[ ] minimap
	[ ] focus color theme
	[ ] relative line numbers
	[ ] vim mode


	Commands:
	- quarter_page_down
	- quarter_page_up
	- delete_rect: deletes text in block mode

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

// custom globals
global Rect_f32 current_cursor_rect;
global Rect_f32 next_cursor_rect;
global f32 cursor_blink_acc;
global b32 cursor_blink_state;
global u32 cursor_blink_idx;
global b32 cursor_blink_paused;

global Color_Table current_color_table;
global Color_Table next_color_table;

global i32 lister_open;
global View_ID lister_view;
global Custom_Command_Function* last_used_command;

#include "4coder_scewps_include.cpp"

// NOTE(allen): Users can declare their own managed IDs here.

// custom files
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
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

