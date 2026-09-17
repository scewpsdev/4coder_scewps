/*
* Miscellaneous helpers for common operations.
*/

// TOP

// Editing commands

CUSTOM_COMMAND_SIG(delete_rect)
CUSTOM_DOC("Deletes the selected range in rectangle mode.")
{
	View_ID view = get_active_view(app, Access_ReadWriteVisible);
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
	if (cursor_pos != mark_pos) {
		Range_i64 range = get_view_range(app, view);
		Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

		Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(cursor_pos));
		Buffer_Cursor mark = buffer_compute_cursor(app, buffer, seek_pos(mark_pos));

		i64 line_min = Min(cursor.line, mark.line);
		i64 line_max = Max(cursor.line, mark.line);
		i64 col_min = Min(cursor.col, mark.col);
		i64 col_max = Max(cursor.col, mark.col);

		History_Group group = history_group_begin(app, buffer);
		for (i64 line = line_min; line <= line_max; line++) {
			i64 delete_start = buffer_compute_cursor(app, buffer, seek_line_col(line, col_min)).pos;
			i64 delete_end = buffer_compute_cursor(app, buffer, seek_line_col(line, col_max)).pos;

			i64 line_end = get_line_end_pos(app, buffer, line);
			delete_end = Min(delete_end, line_end);

			buffer_replace_range(app, buffer, Ii64(delete_start, delete_end), string_u8_empty);
		}
		history_group_end(group);
	}
}

CUSTOM_COMMAND_SIG(seek_beginning_of_line)
CUSTOM_DOC("Seeks the cursor to the beginning of the visual line.")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	i64 prev_cursor_pos = view_get_cursor_pos(app, view);

	default_seek_beginning_of_line(app);
	
	i64 beginning_of_line = view_get_cursor_pos(app, view);
	i64 new_pos = beginning_of_line;
	for (;;) {
		u8 c = buffer_get_char(app, buffer, new_pos);
		if (c != ' ' && c != '\t') {
			break;
		}
		new_pos++;
	}
	if (new_pos == prev_cursor_pos) {
		new_pos = beginning_of_line;
	}
	view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
	no_mark_snap_to_cursor_if_shift(app, view);

	Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
	scroll.target.pixel_shift.x = 0.0f;
	view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

function i64
boundary_character(Application_Links* app, Buffer_ID buffer, Scan_Direction direction, i64 pos, b32(*func)(u8)) {
	if (direction == Scan_Forward) {
		while (func(buffer_get_char(app, buffer, pos)))
			pos++;
	} else {
		while (func(buffer_get_char(app, buffer, pos - 1)))
			pos--;
	}
	return pos;
}

function i64
boundary_alpha_numeric_underscore_camel_not_newline(Application_Links* app, Buffer_ID buffer, Scan_Direction direction, i64 pos) {
	if (direction == Scan_Forward) {
		b32 last_lowercase = false;
		u8 c;
		while (is_alpha_numeric_underscore_not_newline(c = buffer_get_char(app, buffer, pos)) && !is_newline(c) && (!last_lowercase || !character_is_upper(c))) {
			pos++;
			if (character_is_lower(c))
				last_lowercase = true;
		}
	} else {
		u8 c;
		while (is_alpha_numeric_underscore_not_newline(c = buffer_get_char(app, buffer, pos - 1)) && !is_newline(c)) {
			pos--;
			if (character_is_upper(c))
				break;
		}
	}
	return pos;
}

function i64
skip_past(Application_Links* app, Buffer_ID buffer, Scan_Direction direction, i64 pos, u8 stop_char) {
	if (direction == Scan_Forward) {
		u8 c;
		do {
			c = buffer_get_char(app, buffer, pos++);
		} while (c != stop_char);
	} else {
		u8 c;
		do {
			c = buffer_get_char(app, buffer, --pos);
		} while (c != stop_char);
	}
	return pos;
}

function i64
token_boundary(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
	if (direction == Scan_Forward) {
		u8 c = buffer_get_char(app, buffer, pos);
		if (is_alpha_numeric_underscore_not_newline(c)) {
			pos = boundary_character(app, buffer, direction, pos, is_alpha_numeric_underscore_not_newline);
		} else if (is_newline(c)) {
			pos = skip_past(app, buffer, direction, pos, '\n');
		} else {
			pos++;
		}
		pos = boundary_character(app, buffer, direction, pos, is_whitespace_not_newline);
	} else {
		u8 c = buffer_get_char(app, buffer, pos - 1);
		if (is_newline(c)) {
			pos = skip_past(app, buffer, direction, pos, '\n');
		}
		pos = boundary_character(app, buffer, direction, pos, is_whitespace_not_newline);
		if (is_alpha_numeric_underscore_not_newline(buffer_get_char(app, buffer, pos - 1))) {
			pos = boundary_character(app, buffer, direction, pos, is_alpha_numeric_underscore_not_newline);
		} else {
			pos--;
		}
	}
	return pos;
}

function i64
sub_token_boundary(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
	if (direction == Scan_Forward) {
		u8 c = buffer_get_char(app, buffer, pos);
		if (is_alpha_numeric_underscore_not_newline(c)) {
			pos = boundary_alpha_numeric_underscore_camel_not_newline(app, buffer, direction, pos);
		} else if (is_newline(c)) {
			pos = skip_past(app, buffer, direction, pos, '\n');
		} else {
			pos++;
		}
		pos = boundary_character(app, buffer, direction, pos, is_whitespace_not_newline);
	} else {
		u8 c = buffer_get_char(app, buffer, pos - 1);
		if (is_newline(c)) {
			pos = skip_past(app, buffer, direction, pos, '\n');
		}
		pos = boundary_character(app, buffer, direction, pos, is_whitespace_not_newline);
		if (is_alpha_numeric_underscore_not_newline(buffer_get_char(app, buffer, pos - 1))) {
			pos = boundary_alpha_numeric_underscore_camel_not_newline(app, buffer, direction, pos);
		} else {
			pos--;
		}
	}
	return pos;
}

CUSTOM_COMMAND_SIG(move_right_alpha_numeric_boundary)
CUSTOM_DOC("Seek right for boundary between alphanumeric characters and non-alphanumeric characters.")
{
	Scratch_Block scratch(app);
	current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, token_boundary));
}

CUSTOM_COMMAND_SIG(move_left_alpha_numeric_boundary)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters and non-alphanumeric characters.")
{
	Scratch_Block scratch(app);
	current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, token_boundary));
}

CUSTOM_COMMAND_SIG(move_right_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek right for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
	Scratch_Block scratch(app);
	current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, sub_token_boundary));
}

CUSTOM_COMMAND_SIG(move_left_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters or camel case word and non-alphanumeric characters.")
{
	Scratch_Block scratch(app);
	current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, sub_token_boundary));
}

CUSTOM_COMMAND_SIG(backspace_alpha_numeric_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric boundary to the left.")
{
	Scratch_Block scratch(app);
	current_view_boundary_delete(app, Scan_Backward,
		push_boundary_list(scratch, token_boundary));
}

CUSTOM_COMMAND_SIG(delete_alpha_numeric_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric boundary to the right.")
{
	Scratch_Block scratch(app);
	current_view_boundary_delete(app, Scan_Forward,
		push_boundary_list(scratch, token_boundary));
}

CUSTOM_COMMAND_SIG(backspace_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric boundary to the left.")
{
	Scratch_Block scratch(app);
	current_view_boundary_delete(app, Scan_Backward,
		push_boundary_list(scratch, sub_token_boundary));
}

CUSTOM_COMMAND_SIG(delete_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Delete characters between the cursor position and the first alphanumeric boundary to the right.")
{
	Scratch_Block scratch(app);
	current_view_boundary_delete(app, Scan_Forward,
		push_boundary_list(scratch, sub_token_boundary));
}


function void
sc_activate_cursor(Application_Links* app) {
	cursor_blink_state = 2;
	animate_in_n_milliseconds(app, 0);
}

CUSTOM_COMMAND_SIG(click_set_cursor_and_mark)
CUSTOM_DOC("Sets the cursor position and mark to the mouse position.")
{
	default_click_set_cursor_and_mark(app);
	sc_activate_cursor(app);
}

CUSTOM_COMMAND_SIG(click_set_cursor)
CUSTOM_DOC("Sets the cursor position to the mouse position.")
{
	default_click_set_cursor(app);
	sc_activate_cursor(app);
}

CUSTOM_COMMAND_SIG(click_set_cursor_if_lbutton)
CUSTOM_DOC("If the mouse left button is pressed, sets the cursor position to the mouse position.")
{
	default_click_set_cursor_if_lbutton(app);
	sc_activate_cursor(app);
}

CUSTOM_COMMAND_SIG(mouse_wheel_scroll)
CUSTOM_DOC("Reads the scroll wheel value from the mouse state and scrolls the view currently under the mouse accordingly.")
{
	Mouse_State mouse = get_mouse_state(app);
	View_ID active_view = get_active_view(app, Access_ReadVisible);
	if (mouse.wheel.y != 0.f || mouse.wheel.x != 0.f) {
		for (View_ID view = get_view_next(app, 0, Access_ReadVisible);
			view != 0;
			view = get_view_next(app, view, Access_ReadVisible)) {
			Rect_f32 view_rect = view_get_screen_rect(app, view);
			if (rect_contains_point(view_rect, V2f32(mouse.p)))
			{
				Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
				scroll.target = view_move_buffer_point(app, view, scroll.target, mouse.wheel);
				view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
				active_view = view;
				break;
			}
		}
	}
	if (mouse.l) {
		no_mark_snap_to_cursor(app, active_view);
	}
}

CUSTOM_COMMAND_SIG(quarter_page_down)
CUSTOM_DOC("Scrolls the view down a quarter view height and moves the cursor with it.")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Rect_f32 region = view_get_buffer_region(app, view);
	f32 jump = rect_height(region) * 0.25f;
	move_vertical_pixels(app, jump);
}

CUSTOM_COMMAND_SIG(quarter_page_up)
CUSTOM_DOC("Scrolls the view up a quarter view height and moves the cursor with it.")
{
	View_ID view = get_active_view(app, Access_ReadVisible);
	Rect_f32 region = view_get_buffer_region(app, view);
	f32 jump = rect_height(region) * 0.25f;
	move_vertical_pixels(app, -jump);
}

function void
sc_move_lines(Application_Links* app, Buffer_ID buffer, i64 cursor_line, i64 mark_line, Scan_Direction direction) {
	i64 upper = Min(cursor_line, mark_line);
	i64 lower = Max(cursor_line, mark_line);

	History_Group group = history_group_begin(app, buffer);
	if (direction == Scan_Forward) {
		for (i64 line_number = lower; line_number >= upper; line_number--) {
			swap_lines(app, buffer, line_number, line_number + 1);
		}
	}
	else {
		for (i64 line_number = upper; line_number <= lower; line_number++) {
			swap_lines(app, buffer, line_number - 1, line_number);
		}
	}
	history_group_end(group);
}

internal void
sc_current_view_move_line(Application_Links* app, Scan_Direction direction) {
	View_ID view = get_active_view(app, Access_ReadWriteVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
	Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(cursor_pos));
	Buffer_Cursor mark = buffer_compute_cursor(app, buffer, seek_pos(mark_pos));

	sc_move_lines(app, buffer, cursor.line, mark.line, direction);

	i64 delta = direction == Scan_Forward ? 1 : -1;

	view_set_cursor_and_preferred_x(app, view, seek_line_col(cursor.line + delta, cursor.col));
	view_set_mark(app, view, seek_line_col(mark.line + delta, mark.col));
	no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(move_line_up)
CUSTOM_DOC("Swaps the line under the cursor with the line above it, and moves the cursor up with it.")
{
	sc_current_view_move_line(app, Scan_Backward);
}

CUSTOM_COMMAND_SIG(move_line_down)
CUSTOM_DOC("Swaps the line under the cursor with the line below it, and moves the cursor down with it.")
{
	sc_current_view_move_line(app, Scan_Forward);
}

// Misc

function void
add_command_to_list(Lister* lister, i32 command_id, Arena* arena, Command_Lister_Status_Rule* status_rule) {
	Custom_Command_Function* proc = fcoder_metacmd_table[command_id].proc;
	String_Const_u8 status = {};
	switch (status_rule->mode) {
	case CommandLister_Descriptions:
	{
		status = SCu8(fcoder_metacmd_table[command_id].description);
	}break;
	case CommandLister_Bindings:
	{
		Command_Trigger_List triggers = map_get_triggers_recursive(arena, status_rule->mapping, status_rule->map_id, proc);

		List_String_Const_u8 list = {};
		for (Command_Trigger* node = triggers.first;
			node != 0;
			node = node->next) {
			command_trigger_stringize(arena, &list, node);
			if (node->next != 0) {
				string_list_push(arena, &list, string_u8_litexpr(" "));
			}
		}

		status = string_list_flatten(arena, list);
	}break;
	}

	String_Const_u8 cmd = SCu8(fcoder_metacmd_table[command_id].name);
	String_Const_u8 name = push_string_copy(arena, cmd);
	b32 next_is_token_start = true;
	for (i32 k = 0; k < name.size; k++) {
		if (name.str[k] == '_') {
			name.str[k] = ' ';
			next_is_token_start = true;
		}
		else if (next_is_token_start) {
			name.str[k] = character_to_upper(name.str[k]);
			next_is_token_start = false;
		}
	}

	String_Const_u8 description = SCu8(fcoder_metacmd_table[command_id].description);

	lister_add_item(lister, name, status, description,
		(void*)proc, 0);
}

function Custom_Command_Function*
sc_get_command_from_user(Application_Links* app, String_Const_u8 query, i32* command_ids, i32 command_id_count, Command_Lister_Status_Rule* status_rule) {
	if (command_ids == 0) {
		command_id_count = command_one_past_last_id;
	}

	Scratch_Block scratch(app);
	Lister_Block lister(app, scratch);
	lister_set_query(lister, query);
	lister_set_default_handlers(lister);

	if (last_used_command) {
		add_command_to_list(lister, get_command_id(last_used_command), scratch, status_rule);
	}

	for (i32 i = 0; i < command_id_count; i += 1) {
		i32 j = i;
		if (command_ids != 0) {
			j = command_ids[i];
		}
		j = clamp(0, j, command_one_past_last_id);

		add_command_to_list(lister, j, scratch, status_rule);
	}

	Lister_Result l_result = run_lister(app, lister);

	Custom_Command_Function* result = 0;
	if (!l_result.canceled) {
		result = (Custom_Command_Function*)l_result.user_data;
		last_used_command = result;
	}
	return(result);
}

function Custom_Command_Function*
sc_get_command_from_user(Application_Links* app, String_Const_u8 query, Command_Lister_Status_Rule* status_rule) {
	return(sc_get_command_from_user(app, query, 0, 0, status_rule));
}

function Custom_Command_Function*
sc_get_command_from_user(Application_Links* app, char* query,
	i32* command_ids, i32 command_id_count, Command_Lister_Status_Rule* status_rule) {
	return(sc_get_command_from_user(app, SCu8(query), command_ids, command_id_count, status_rule));
}

function Custom_Command_Function*
sc_get_command_from_user(Application_Links* app, char* query, Command_Lister_Status_Rule* status_rule) {
	return(sc_get_command_from_user(app, SCu8(query), 0, 0, status_rule));
}

CUSTOM_COMMAND_SIG(command_lister)
CUSTOM_DOC("Opens an interactive list of all registered commands.")
{
	View_ID view = get_this_ctx_view(app, Access_Always);
	if (view != 0) {
		Command_Lister_Status_Rule rule = {};
		Buffer_ID buffer = view_get_buffer(app, view, Access_Visible);
		Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
		Command_Map_ID* map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
		if (map_id_ptr != 0) {
			rule = command_lister_status_bindings(&framework_mapping, *map_id_ptr);
		}
		else {
			rule = command_lister_status_descriptions();
		}
		Custom_Command_Function* func = sc_get_command_from_user(app, "Command:", &rule);
		if (func != 0) {
			view_enqueue_command_function(app, view, func);
		}
	}

	no_mark_snap_to_cursor(app, view);
}

CUSTOM_UI_COMMAND_SIG(theme_lister)
CUSTOM_DOC("Opens an interactive list of all registered themes.")
{
	Color_Table* color_table = sc_get_color_table_from_user(app);
	if (color_table != 0) {
		active_color_table = *color_table;
	}
}

CUSTOM_COMMAND_SIG(reload_config)
CUSTOM_DOC("Reloads config file")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
	
	Scratch_Block scratch(app);

	String_Const_u8 path = push_buffer_file_name(app, scratch, buffer);
	String_Const_u8 name = string_front_of_path(path);

	if (string_match(name, string_u8_litexpr("config.4coder"))) {
		load_config_and_apply(app, &global_config_arena, 0, false);
	}
}

CUSTOM_COMMAND_SIG(reload_bindings)
CUSTOM_DOC("Reloads bindings file")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);

	Scratch_Block scratch(app);

	String_Const_u8 path = push_buffer_file_name(app, scratch, buffer);
	String_Const_u8 name = string_front_of_path(path);

	if (string_match(name, string_u8_litexpr("bindings.4coder")) && dynamic_binding_load_from_file(app, &framework_mapping, name)) {
		String_ID global_map_id = vars_save_string_lit("keys_global");
		String_ID file_map_id = vars_save_string_lit("keys_file");
		String_ID code_map_id = vars_save_string_lit("keys_code");

		sc_setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
	}
}

CUSTOM_UI_COMMAND_SIG(jump_to_definition_at_cursor)
CUSTOM_DOC("Jump to the first definition in the code index matching an identifier at the cursor")
{
	View_ID view = get_active_view(app, Access_ReadVisible);

	if (view != 0) {
		Scratch_Block scratch(app);
		String_Const_u8 query = push_token_or_word_under_active_cursor(app, scratch);

		F4_Index_Lock();

		F4_Index_Note* note = F4_Index_LookupNote(query);
		point_stack_push_view_cursor(app, view);
		jump_to_location(app, view, note->file->buffer, note->range.min);
		view_set_mark(app, view, seek_pos(note->range.min));

		F4_Index_Unlock();
	}
}

CUSTOM_UI_COMMAND_SIG(jump_to_definition_at_cursor_other_panel)
CUSTOM_DOC("Jump to the first definition in the code index matching an identifier at the cursor")
{
	View_ID view = get_active_view(app, Access_ReadVisible);

	if (view != 0) {
		Scratch_Block scratch(app);
		String_Const_u8 query = push_token_or_word_under_active_cursor(app, scratch);

		F4_Index_Lock();

		F4_Index_Note* note = F4_Index_LookupNote(query);
		View_ID target_view = get_next_view_looped_primary_panels(app, view, Access_Always);
		point_stack_push_view_cursor(app, target_view);
		jump_to_location(app, target_view, note->file->buffer, note->range.min);
		view_set_mark(app, target_view, seek_pos(note->range.min));

		F4_Index_Unlock();
	}
}

function void
sc_isearch(Application_Links* app, Scan_Direction start_scan, i64 first_pos,
	String_Const_u8 query_init) {
	View_ID view = get_active_view(app, Access_ReadVisible);
	Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	if (!buffer_exists(app, buffer)) {
		return;
	}

	i64 buffer_size = buffer_get_size(app, buffer);

	Query_Bar_Group group(app);
	Query_Bar bar = {};
	if (start_query_bar(app, &bar, 0) == 0) {
		return;
	}

	Vec2_f32 old_margin = {};
	Vec2_f32 old_push_in = {};
	view_get_camera_bounds(app, view, &old_margin, &old_push_in);

	Vec2_f32 margin = old_margin;
	margin.y = clamp_bot(200.f, margin.y);
	view_set_camera_bounds(app, view, margin, old_push_in);

	Scan_Direction scan = start_scan;
	i64 pos = first_pos;

	u8 bar_string_space[256];
	bar.string = SCu8(bar_string_space, query_init.size);
	block_copy(bar.string.str, query_init.str, query_init.size);

	String_Const_u8 isearch_str = string_u8_litexpr("Search: ");
	String_Const_u8 rsearch_str = string_u8_litexpr("Backwards Search: ");

	u64 match_size = bar.string.size;

	User_Input in = {};
	for (;;) {
		switch (scan) {
		case Scan_Forward:
		{
			bar.prompt = isearch_str;
		}break;
		case Scan_Backward:
		{
			bar.prompt = rsearch_str;
		}break;
		}
		isearch__update_highlight(app, view, Ii64_size(pos, match_size));

		in = get_next_input(app, EventPropertyGroup_Any, 0/*EventProperty_Escape*/);
		if (in.abort) {
			break;
		}

		String_Const_u8 string = to_writable(&in);

		b32 string_change = false;
		if (match_key_code(&in, KeyCode_Escape)) {
			Input_Modifier_Set* mods = &in.event.key.modifiers;
			if (has_modifier(mods, KeyCode_Control)) {
				bar.string.size = cstring_length(previous_isearch_query);
				block_copy(bar.string.str, previous_isearch_query, bar.string.size);
			}
			else {
				u64 size = bar.string.size;
				size = clamp_top(size, sizeof(previous_isearch_query) - 1);
				block_copy(previous_isearch_query, bar.string.str, size);
				previous_isearch_query[size] = 0;
				break;
			}
		}
		else if (string.str != 0 && string.size > 0) {
			String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
			string_append(&bar_string, string);
			bar.string = bar_string.string;
			string_change = true;
		}
		else if (match_key_code(&in, KeyCode_Backspace)) {
			if (is_unmodified_key(&in.event)) {
				u64 old_bar_string_size = bar.string.size;
				bar.string = backspace_utf8(bar.string);
				string_change = (bar.string.size < old_bar_string_size);
			}
			else if (has_modifier(&in.event.key.modifiers, KeyCode_Control)) {
				if (bar.string.size > 0) {
					string_change = true;
					bar.string.size = 0;
				}
			}
		}

		b32 do_scan_action = false;
		b32 do_scroll_wheel = false;
		Scan_Direction change_scan = scan;
		if (!string_change) {
			if (match_key_code(&in, KeyCode_PageDown) ||
				match_key_code(&in, KeyCode_Down) ||
				match_key_code(&in, KeyCode_Return) && !has_modifier(&in.event.key.modifiers, KeyCode_Shift)) {
				change_scan = Scan_Forward;
				do_scan_action = true;
			}
			else if (match_key_code(&in, KeyCode_PageUp) ||
				match_key_code(&in, KeyCode_Up) ||
				match_key_code(&in, KeyCode_Return) && has_modifier(&in.event.key.modifiers, KeyCode_Shift)) {
				change_scan = Scan_Backward;
				do_scan_action = true;
			}
			else {
				// NOTE(allen): is the user trying to execute another command?
				View_Context ctx = view_current_context(app, view);
				Mapping* mapping = ctx.mapping;
				Command_Map* map = mapping_get_map(mapping, ctx.map_id);
				Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
				if (binding.custom != 0) {
					if (binding.custom == search) {
						change_scan = Scan_Forward;
						do_scan_action = true;
					}
					else if (binding.custom == reverse_search) {
						change_scan = Scan_Backward;
						do_scan_action = true;
					}
					else {
						Command_Metadata* metadata = get_command_metadata(binding.custom);
						if (metadata != 0) {
							if (metadata->is_ui) {
								view_enqueue_command_function(app, view, binding.custom);
								break;
							}
						}
						binding.custom(app);
					}
				}
				else {
					leave_current_input_unhandled(app);
				}
			}
		}

		if (string_change) {
			switch (scan) {
			case Scan_Forward:
			{
				i64 new_pos = 0;
				seek_string_insensitive_forward(app, buffer, pos - 1, 0, bar.string, &new_pos);
				if (new_pos < buffer_size) {
					pos = new_pos;
					match_size = bar.string.size;
				}
			}break;

			case Scan_Backward:
			{
				i64 new_pos = 0;
				seek_string_insensitive_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
				if (new_pos >= 0) {
					pos = new_pos;
					match_size = bar.string.size;
				}
			}break;
			}
		}
		else if (do_scan_action) {
			scan = change_scan;
			switch (scan) {
			case Scan_Forward:
			{
				i64 new_pos = 0;
				seek_string_insensitive_forward(app, buffer, pos, 0, bar.string, &new_pos);
				if (new_pos < buffer_size) {
					pos = new_pos;
					match_size = bar.string.size;
				}
			}break;

			case Scan_Backward:
			{
				i64 new_pos = 0;
				seek_string_insensitive_backward(app, buffer, pos, 0, bar.string, &new_pos);
				if (new_pos >= 0) {
					pos = new_pos;
					match_size = bar.string.size;
				}
			}break;
			}
		}
		else if (do_scroll_wheel) {
			mouse_wheel_scroll(app);
		}
	}

	view_disable_highlight_range(app, view);

	if (in.abort) {
		u64 size = bar.string.size;
		size = clamp_top(size, sizeof(previous_isearch_query) - 1);
		block_copy(previous_isearch_query, bar.string.str, size);
		previous_isearch_query[size] = 0;
		view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
	}

	view_set_camera_bounds(app, view, old_margin, old_push_in);
}

function void
sc_isearch(Application_Links* app, Scan_Direction start_scan, String_Const_u8 query_init) {
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);;
	sc_isearch(app, start_scan, pos, query_init);
}

function void
sc_isearch(Application_Links* app, Scan_Direction start_scan) {
	View_ID view = get_active_view(app, Access_ReadVisible);
	i64 pos = view_get_cursor_pos(app, view);;
	sc_isearch(app, start_scan, pos, SCu8());
}

CUSTOM_COMMAND_SIG(search)
CUSTOM_DOC("Begins an incremental search down through the current buffer for a user specified string.")
{
	View_ID view = get_active_view(app, Access_ReadVisible);

	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);

	if (cursor_pos != mark_pos) {
		Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
		
		Scratch_Block scratch(app);
		String_Const_u8 identifier = push_buffer_range(app, scratch, buffer, Ii64(cursor_pos, mark_pos));

		sc_isearch(app, Scan_Forward, Min(cursor_pos, mark_pos), identifier);
	} else {
		sc_isearch(app, Scan_Forward);
	}
}

CUSTOM_COMMAND_SIG(reverse_search)
CUSTOM_DOC("Begins an incremental search up through the current buffer for a user specified string.")
{
	View_ID view = get_active_view(app, Access_ReadVisible);

	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);

	if (cursor_pos != mark_pos && fcoder_mode == FCoderMode_NotepadLike) {
		Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

		Scratch_Block scratch(app);
		String_Const_u8 identifier = push_buffer_range(app, scratch, buffer, Ii64(cursor_pos, mark_pos));

		sc_isearch(app, Scan_Backward, Min(cursor_pos, mark_pos), identifier);
	}
	else {
		sc_isearch(app, Scan_Backward);
	}
}

// BOTTOM

