/*
4coder_scewps.cpp - Supplies the bindings used for default 4coder behavior.
*/

/*

	TODO

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
		[ ] move selected lines with alt
		[ ] block select
		[ ] slower autoscroll
	[ ] hot reloading
		[ ] config
		[ ] bindings
		[ ] themes
			[ ] smooth interpolate
			[ ] preview on lister hover
	[ ] lister
		[ ] floating lister
		[ ] darken buffer behind lister
		[ ] ctrl+backspace
		[ ] backspace on path deletes whole directory
		[ ] display keybindings on right
		[ ] display commands as capitalized words
		[ ] preselect last command (or put at top)
	[ ] hex color preview
	[ ] comment dividers
	[ ] brace lines
	[ ] highlight current parens
	[ ] token occurance underline
	[ ] todo buffer
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
	[ ] minimap
	[ ] focus color theme

*/

// TOP

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_scewps_include.h"
#include "4coder_scewps_include.cpp"

// NOTE(allen): Users can declare their own managed IDs here.

// custom globals
global Rect_f32 current_cursor_rect;
global Rect_f32 next_cursor_rect;
global f32 cursor_blink_acc;
global b32 cursor_blink_state;
global u32 cursor_blink_idx;
global b32 cursor_blink_paused;

// custom files
#include "4coder_scewps_config.cpp"
#include "4coder_scewps_helper.cpp"
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
	setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

