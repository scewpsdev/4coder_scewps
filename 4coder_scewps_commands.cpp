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
	default_mouse_wheel_scroll_over_hovered_view(app);
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

// BOTTOM

