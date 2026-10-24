/*
4coder_default_hooks.cpp - Sets up the hooks for the default framework.
*/

// TOP

#define lerp(a, b, t) ((a) + ((b) - (a)) * (t))
#define interp(a, b, dt, rate) ((b) + ((a) - (b)) * expf(-dt * rate))
#define interp2(a, b, dt, rate) ((a) + ((b) - (a)) * (1 - powf(rate, dt)))

CUSTOM_COMMAND_SIG(sc_startup)
CUSTOM_DOC("Default command for responding to a startup event")
{
	default_startup(app);

	current_color_table = init_color_table(app);
	last_color_table = init_color_table(app);
	next_color_table = init_color_table(app);

	Scratch_Block scratch(app);

	String_Const_u8 small_code_font_name = def_get_config_string(scratch, vars_save_string_lit("default_font_name"));
	if (small_code_font_name.size)
	{
		String_Const_u8 binary_path = system_get_path(scratch, SystemPath_Binary);
		String_Const_u8 font_path = push_stringf(scratch, (char*)"%.*sfonts/%.*s", binary_path.size, binary_path.str, small_code_font_name.size, small_code_font_name.str);
		
		u64 small_code_font_size = def_get_config_u64(app, vars_save_string_lit("default_font_size"), 15) - 2;

		Face_Description desc = { 0 };
		desc.font.file_name = font_path;
		desc.parameters.pt_size = (u32)small_code_font_size;
		desc.parameters.bold = 0;
		desc.parameters.italic = 0;
		desc.parameters.hinting = 0;

		small_code_font = try_create_new_face(app, &desc);
	}

	String_Const_u8 ui_font_name = def_get_config_string(scratch, vars_save_string_lit("ui_font_name"));
	if (ui_font_name.size) {
		u64 ui_font_size = def_get_config_u64(app, vars_save_string_lit("ui_font_size"), 15);

		Face_Description desc = { 0 };
		desc.font.file_name = ui_font_name;
		desc.parameters.pt_size = (u32)ui_font_size;
		desc.parameters.bold = 0;
		desc.parameters.italic = 0;
		desc.parameters.hinting = 0;

		ui_font = try_create_new_face(app, &desc);
	}
}

CUSTOM_COMMAND_SIG(sc_view_input_handler)
CUSTOM_DOC("Input consumption loop for default view behavior")
{
	Scratch_Block scratch(app);
	default_input_handler_init(app, scratch);

	View_ID view = get_this_ctx_view(app, Access_Always);
	Managed_Scope scope = view_get_managed_scope(app, view);

	for (;;) {
		// NOTE(allen): Get input
		User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
		if (input.abort) {
			break;
		}

		// activate cursor on input
		if (input.event.kind == InputEventKind_KeyStroke) {
			sc_activate_cursor(app);
		}

		ProfileScopeNamed(app, "before view input", view_input_profile);

		// NOTE(allen): Mouse Suppression
		Event_Property event_properties = get_event_properties(&input.event);
		if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0) {
			continue;
		}

		// NOTE(allen): Get binding
		if (implicit_map_function == 0) {
			implicit_map_function = default_implicit_map;
		}
		Implicit_Map_Result map_result = implicit_map_function(app, 0, 0, &input.event);
		if (map_result.command == 0) {
			leave_current_input_unhandled(app);
		} else {
			// NOTE(allen): Run the command and pre/post command stuff
			default_pre_command(app, scope);
			ProfileCloseNow(view_input_profile);
			map_result.command(app);
			ProfileScope(app, "after view input");
			default_post_command(app, scope);
		}

		Word_Complete_Menu** menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
		if (menu_ptr) {
			Word_Complete_Menu* menu = *menu_ptr;
			if (menu) {
				completion_list_on_event(app, view, menu, input);
			}
		}
	}
}

function void
sc_tick(Application_Links* app, Frame_Info frame_info) {
	b32 interpolate_cursor = def_get_config_b32(vars_save_string_lit("interpolate_cursor"));
	if (interpolate_cursor) {
		Vec2_f32 cursor_size = next_cursor_rect.p1 - next_cursor_rect.p0;
		current_cursor_rect.p0 = interp(current_cursor_rect.p0, next_cursor_rect.p0, frame_info.animation_dt, 50);
		current_cursor_rect.p1 = current_cursor_rect.p0 + cursor_size;
		if (near_zero(current_cursor_rect.p0 - next_cursor_rect.p0, 0.5f)) {
			current_cursor_rect = next_cursor_rect;
		}
		else {
			animate_in_n_milliseconds(app, 0);
		}
	}
	else {
		current_cursor_rect = next_cursor_rect;
	}

	cursor_blink_acc += frame_info.literal_dt;
	if (cursor_blink_state == 2) {
		cursor_blink_state = 1;
		cursor_blink_acc = 0.0f;
		cursor_blink_idx = 0;
		cursor_blink_paused = false;
	}
	if (cursor_blink_acc >= 0.5f && !cursor_blink_paused) {
		cursor_blink_acc = 0.0f;
		cursor_blink_state = cursor_blink_state ? 0 : 1;
		if (cursor_blink_state) {
			cursor_blink_idx++;
			if (cursor_blink_idx >= 10) {
				cursor_blink_paused = true;
			}
		}
	}
	if (!cursor_blink_paused) {
		animate_in_n_milliseconds(app, u32((0.5f - cursor_blink_acc) * 1000));
	}

	// if theme got changed, set that theme to be the interpolation dst
	if (active_color_table.arrays != current_color_table.arrays) {
		copy_color_table(active_color_table, next_color_table);
		copy_color_table(current_color_table, last_color_table);
		active_color_table = current_color_table;

		// handle different value counts.
		// since colors can get interpolated but not value counts we just snap to the next count.
		// we also fill the memory in the value array that will now be used for interpolation,
		// so that we're not operating on garbage values or leftover colors from other themes.
		for (i32 i = 0; i < active_color_table.count; i++) {
			active_color_table.arrays[i].count = next_color_table.arrays[i].count;
		}

		color_transition = 0;
	}
	if (color_transition != 1) {
		color_transition = interp2(color_transition, 1, frame_info.animation_dt, 1e-3f);
		if (near_zero(1 - color_transition, 0.01f)) {
			color_transition = 1;
		}

		for (i64 i = 0; i < active_color_table.count; i++) {
			for (i64 j = 0; j < active_color_table.arrays[i].count; j++) {
				ARGB_Color from_argb = last_color_table.arrays[i].vals[j % last_color_table.arrays[i].count];
				if (!F4_ARGBIsValid(from_argb))
					from_argb = fcolor_resolve(fcolor_id(defcolor_text_default));

				ARGB_Color to_argb = next_color_table.arrays[i].vals[j];
				if (!F4_ARGBIsValid(to_argb))
					to_argb = fcolor_resolve(fcolor_id(defcolor_text_default));

				Vec4_f32 from = unpack_color(from_argb);
				Vec4_f32 to = unpack_color(to_argb);

				Vec4_f32 result = lerp(from, to, color_transition);
				ARGB_Color result_argb = pack_color(result);

				active_color_table.arrays[i].vals[j] = result_argb;
			}
		}
		
		animate_in_n_milliseconds(app, 0);
	}

	F4_TickColors(app, frame_info);
	F4_Index_Tick(app);

	buffer_modified_set_clear();

	////////////////////////////////
	// NOTE(allen): Update fade ranges

	if (tick_all_fade_ranges(app, frame_info.animation_dt)) {
		animate_in_n_milliseconds(app, 0);
	}

	////////////////////////////////
	// NOTE(allen): Clear layouts if virtual whitespace setting changed.

	{
		b32 enable_virtual_whitespace = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
		if (enable_virtual_whitespace != def_enable_virtual_whitespace) {
			def_enable_virtual_whitespace = enable_virtual_whitespace;
			clear_all_layouts(app);
		}
	}
}

DELTA_RULE_SIG(fixed_time_linear_delta) {
	local_const f32 duration_in_seconds = (1.f / 8.f);
	local_const f32 dt_multiplier = 1.f / duration_in_seconds;
	f32 step = dt * dt_multiplier;
	f32* t = (f32*)data;
	*t = clamp(0.f, *t, 1.f);
	f32 prev_t = *t;
	if (is_new_target) {
		prev_t = 0.f;
		*t = step;
	}
	else {
		*t += step;
	}
	*t = clamp(0.f, *t, 1.f);
	Vec2_f32 result = pending;
	if (*t < 1.f) {
		f32 prev_x = cubic_reinterpolate(prev_t);
		f32 x = cubic_reinterpolate(*t);
		f32 portion = ((x - prev_x) / (1.f - prev_x));
		result *= portion;
	}
	return(result);
}

function void
sc_whole_screen_render_caller(Application_Links* app, Frame_Info frame_info) {
#if 0
	Rect_f32 region = global_get_screen_rectangle(app);
	Vec2_f32 center = rect_center(region);

	Face_ID face_id = get_face_id(app, 0);
	Scratch_Block scratch(app);
	draw_string_oriented(app, face_id, finalize_color(defcolor_text_default, 0),
		SCu8("Hello, World!"), center - V2f32(200.f, 300.f),
		0, V2f32(0.f, -1.f));
	draw_string_oriented(app, face_id, finalize_color(defcolor_text_default, 0),
		SCu8("Hello, World!"), center - V2f32(240.f, 300.f),
		0, V2f32(0.f, 1.f));
	draw_string_oriented(app, face_id, finalize_color(defcolor_text_default, 0),
		SCu8("Hello, World!"), center - V2f32(400.f, 400.f),
		0, V2f32(-1.f, 0.f));
	draw_string_oriented(app, face_id, finalize_color(defcolor_text_default, 0),
		SCu8("Hello, World!"), center - V2f32(400.f, -100.f),
		0, V2f32(cos_f32(pi_f32 * .333f), sin_f32(pi_f32 * .333f)));
#endif
}

function Layout_Item_List
sc_layout__inner(Application_Links* app, Arena* arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width, Layout_Virtual_Indent virt_indent) {
	Layout_Item_List list = get_empty_item_list(range);

	Scratch_Block scratch(app);
	String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);

	Face_Advance_Map advance_map = get_face_advance_map(app, face);
	Face_Metrics metrics = get_face_metrics(app, face);
	f32 tab_width = (f32)def_get_config_u64(app, vars_save_string_lit("default_tab_width"));
	tab_width = clamp_bot(1, tab_width);

	f32 line_height_scale = (f32)def_get_config_u64(app, vars_save_string_lit("line_height_scale"));
	if (line_height_scale == 0) line_height_scale = 100.0f;
	f32 line_padding = f32_ceil32((line_height_scale / 100 - 1) * metrics.line_height * 0.5f);
	line_padding = 10;

	LefRig_TopBot_Layout_Vars pos_vars = get_lr_tb_layout_vars(&advance_map, &metrics, tab_width, width);

	if (text.size == 0) {
		lr_tb_write_blank(&pos_vars, face, arena, &list, range.first);
	}
	else {
		b32 skipping_leading_whitespace = (virt_indent == LayoutVirtualIndent_On);
		Newline_Layout_Vars newline_vars = get_newline_layout_vars();

		u8* ptr = text.str;
		u8* end_ptr = ptr + text.size;
		for (; ptr < end_ptr;) {
			Character_Consume_Result consume = utf8_consume(ptr, (u64)(end_ptr - ptr));

			i64 index = layout_index_from_ptr(ptr, text.str, range.first);
			switch (consume.codepoint) {
			case '\t':
			case ' ':
			{
				newline_layout_consume_default(&newline_vars);
				f32 advance = lr_tb_advance(&pos_vars, face, consume.codepoint);
				if (!skipping_leading_whitespace) {
					lr_tb_write_with_advance(&pos_vars, face, advance, arena, &list, index, consume.codepoint);
				}
				else {
					lr_tb_advance_x_without_item(&pos_vars, advance);
				}
			}break;

			default:
			{
				newline_layout_consume_default(&newline_vars);
				lr_tb_write(&pos_vars, face, arena, &list, index, consume.codepoint);
			}break;

			case '\r':
			{
				newline_layout_consume_CR(&newline_vars, index);
			}break;

			case '\n':
			{
				i64 newline_index = newline_layout_consume_LF(&newline_vars, index);
				lr_tb_write_blank(&pos_vars, face, arena, &list, newline_index);
				lr_tb_next_line(&pos_vars);
			}break;

			case max_u32:
			{
				newline_layout_consume_default(&newline_vars);
				lr_tb_write_byte(&pos_vars, face, arena, &list, index, *ptr);
			}break;
			}

			ptr += consume.inc;
		}

		if (newline_layout_consume_finish(&newline_vars)) {
			i64 index = layout_index_from_ptr(ptr, text.str, range.first);
			lr_tb_write_blank(&pos_vars, face, arena, &list, index);
		}
	}

	layout_item_list_finish(&list, -pos_vars.line_to_text_shift);

	return(list);
}

function Layout_Item_List
sc_layout(Application_Links* app, Arena* arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width) {
	return(sc_layout__inner(app, arena, buffer, range, face, width, LayoutVirtualIndent_Off));
}

function void
sc_do_full_lex_async__inner(Async_Context* actx, Buffer_ID buffer_id) {
	Application_Links* app = actx->app;
	ProfileScope(app, "async lex");
	Scratch_Block scratch(app);

	String_Const_u8 contents = {};
	{
		ProfileBlock(app, "async lex contents (before mutex)");
		acquire_global_frame_mutex(app);
		ProfileBlock(app, "async lex contents (after mutex)");
		contents = push_whole_buffer(app, scratch, buffer_id);
		release_global_frame_mutex(app);
	}

	i32 limit_factor = 10000;

	Token_List list = {};
	b32 canceled = false;

	F4_Language* language = F4_LanguageFromBuffer(app, buffer_id);
	if (!language) {
		language = F4_LanguageFromString(S8Lit("cpp"));
	}

	void* lexing_state = push_array_zero(scratch, u8, language->lex_state_size);
	language->LexInit(lexing_state, contents);
	for (;;) {
		ProfileBlock(app, "async lex block");
		if (language->LexFullInput(scratch, &list, lexing_state, limit_factor)) {
			break;
		}
		if (async_check_canceled(actx)) {
			canceled = true;
			break;
		}
	}

	if (!canceled) {
		ProfileBlock(app, "async lex save results (before mutex)");
		acquire_global_frame_mutex(app);
		ProfileBlock(app, "async lex save results (after mutex)");
		Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
		if (scope != 0) {
			Base_Allocator* allocator = managed_scope_allocator(app, scope);
			Token_Array* tokens_ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
			base_free(allocator, tokens_ptr->tokens);
			Token_Array tokens = {};
			tokens.tokens = base_array(allocator, Token, list.total_count);
			tokens.count = list.total_count;
			tokens.max = list.total_count;
			token_fill_memory_from_list(tokens.tokens, &list);
			block_copy_struct(tokens_ptr, &tokens);
		}
		buffer_mark_as_modified(buffer_id);
		release_global_frame_mutex(app);
	}
}

function void
sc_do_full_lex_async(Async_Context* actx, String_Const_u8 data) {
	if (data.size == sizeof(Buffer_ID)) {
		Buffer_ID buffer = *(Buffer_ID*)data.str;
		sc_do_full_lex_async__inner(actx, buffer);
	}
}

BUFFER_HOOK_SIG(sc_begin_buffer) {
	ProfileScope(app, "begin buffer");

	Scratch_Block scratch(app);

	b32 treat_as_code = false;
	String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);

	if (file_name.size > 0) {
		String_Const_u8 ext = string_file_extension(file_name);

		String_Const_u8 treat_as_code_string = def_get_config_string(scratch, vars_save_string_lit("treat_as_code"));
		String_Const_u8_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
		for (i32 i = 0; i < extensions.count; ++i) {
			if (string_match(ext, extensions.strings[i])) {
				treat_as_code = true;
				break;
			}
		}

		if (!treat_as_code) {
			if (F4_LanguageFromBuffer(app, buffer_id)) {
				treat_as_code = true;
			}
		}
	}

	String_ID file_map_id = vars_save_string_lit("keys_file");
	String_ID code_map_id = vars_save_string_lit("keys_code");

	Command_Map_ID map_id = (treat_as_code) ? (code_map_id) : (file_map_id);
	Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
	Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
	*map_id_ptr = map_id;

	Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
	Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
	*eol_setting = setting;

	// NOTE(allen): Decide buffer settings
	b32 wrap_lines = true;
	b32 use_lexer = false;
	if (treat_as_code) {
		wrap_lines = def_get_config_b32(vars_save_string_lit("enable_code_wrapping"));
		use_lexer = true;
	}

	String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
	if (buffer_name.size > 0 && buffer_name.str[0] == '*' && buffer_name.str[buffer_name.size - 1] == '*') {
		wrap_lines = def_get_config_b32(vars_save_string_lit("enable_output_wrapping"));
	}

	if (use_lexer) {
		ProfileBlock(app, "begin buffer kick off lexer");
		Async_Task* lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
		*lex_task_ptr = async_task_no_dep(&global_async_system, sc_do_full_lex_async, make_data_struct(&buffer_id));
	}

	{
		b32* wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
		*wrap_lines_ptr = wrap_lines;
	}

	if (use_lexer) {
		buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
	}
	else {
		if (treat_as_code) {
			buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
		}
		else {
			buffer_set_layout(app, buffer_id, layout_generic);
		}
	}

	// no meaning for return
	return(0);
}

BUFFER_EDIT_RANGE_SIG(sc_buffer_edit) {
	// buffer_id, new_range, original_size
	ProfileScope(app, "default edit range");

	Range_i64 old_range = Ii64(old_cursor_range.min.pos, old_cursor_range.max.pos);

	buffer_shift_fade_ranges(buffer_id, old_range.max, (new_range.max - old_range.max));

	{
		code_index_lock();
		Code_Index_File* file = code_index_get_file(buffer_id);
		if (file != 0) {
			code_index_shift(file, old_range, range_size(new_range));
		}
		code_index_unlock();
	}

	i64 insert_size = range_size(new_range);
	i64 text_shift = replace_range_shift(old_range, insert_size);

	Scratch_Block scratch(app);

	Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
	Async_Task* lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);

	Base_Allocator* allocator = managed_scope_allocator(app, scope);
	b32 do_full_relex = false;

	if (async_task_is_running_or_pending(&global_async_system, *lex_task_ptr)) {
		async_task_cancel(app, &global_async_system, *lex_task_ptr);
		buffer_unmark_as_modified(buffer_id);
		do_full_relex = true;
		*lex_task_ptr = 0;
	}

	Token_Array* ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
	if (ptr != 0 && ptr->tokens != 0) {
		ProfileBlockNamed(app, "attempt resync", profile_attempt_resync);

		i64 token_index_first = token_relex_first(ptr, old_range.first, 1);
		i64 token_index_resync_guess =
			token_relex_resync(ptr, old_range.one_past_last, 16);

		if (token_index_resync_guess - token_index_first >= 4000) {
			do_full_relex = true;
		} else {
			Token* token_first = ptr->tokens + token_index_first;
			Token* token_resync = ptr->tokens + token_index_resync_guess;

			Range_i64 relex_range = Ii64(token_first->pos, token_resync->pos + token_resync->size + text_shift);
			String_Const_u8 partial_text = push_buffer_range(app, scratch, buffer_id, relex_range);

			F4_Language* language = F4_LanguageFromBuffer(app, buffer_id);
			if (!language) {
				language = F4_LanguageFromString(S8Lit("cpp"));
			}

			Token_List relex_list = F4_Language_LexFullInput_NoBreaks(app, language, scratch, partial_text); //lex_full_input_cpp(scratch, partial_text);
			if (relex_range.one_past_last < buffer_get_size(app, buffer_id)) {
				token_drop_eof(&relex_list);
			}

			Token_Relex relex = token_relex(relex_list, relex_range.first - text_shift, ptr->tokens, token_index_first, token_index_resync_guess);

			ProfileCloseNow(profile_attempt_resync);

			if (!relex.successful_resync) {
				do_full_relex = true;
			}
			else {
				ProfileBlock(app, "apply resync");

				i64 token_index_resync = relex.first_resync_index;

				Range_i64 head = Ii64(0, token_index_first);
				Range_i64 replaced = Ii64(token_index_first, token_index_resync);
				Range_i64 tail = Ii64(token_index_resync, ptr->count);
				i64 resynced_count = (token_index_resync_guess + 1) - token_index_resync;
				i64 relexed_count = relex_list.total_count - resynced_count;
				i64 tail_shift = relexed_count - (token_index_resync - token_index_first);

				i64 new_tokens_count = ptr->count + tail_shift;
				Token* new_tokens = base_array(allocator, Token, new_tokens_count);

				Token* old_tokens = ptr->tokens;
				block_copy_array_shift(new_tokens, old_tokens, head, 0);
				token_fill_memory_from_list(new_tokens + replaced.first, &relex_list, relexed_count);
				for (i64 i = 0, index = replaced.first; i < relexed_count; i += 1, index += 1) {
					new_tokens[index].pos += relex_range.first;
				}
				for (i64 i = tail.first; i < tail.one_past_last; i += 1) {
					old_tokens[i].pos += text_shift;
				}
				block_copy_array_shift(new_tokens, ptr->tokens, tail, tail_shift);

				base_free(allocator, ptr->tokens);

				ptr->tokens = new_tokens;
				ptr->count = new_tokens_count;
				ptr->max = new_tokens_count;

				buffer_mark_as_modified(buffer_id);
			}
		}
	}

	if (do_full_relex) {
		*lex_task_ptr = async_task_no_dep(&global_async_system, sc_do_full_lex_async,
			make_data_struct(&buffer_id));
	}

	// no meaning for return
	return(0);
}

bool string_has_prefix(String_Const_u8 s, String_Const_u8 prefix) {
	return (string_match(string_prefix(s, prefix.size), prefix));
}

bool string_has_postfix(String_Const_u8 s, String_Const_u8 postfix) {
	return (string_match(string_postfix(s, postfix.size), postfix));
}

BUFFER_HOOK_SIG(sc_file_save) {
	default_file_save(app, buffer_id);

	Scratch_Block scratch(app);

	String_Const_u8 path = push_buffer_file_name(app, scratch, buffer_id);
	String_Const_u8 name = string_front_of_path(path);

	if (string_has_prefix(name, string_u8_litexpr("theme-")) && string_has_postfix(name, string_u8_litexpr(".4coder"))) {
		Arena* arena = &global_theme_arena;
		Color_Table color_table = make_color_table(app, arena);
		Config* config = theme_parse__buffer(app, scratch, buffer_id, arena, &color_table);
		String_Const_u8 error_text = config_stringize_errors(app, scratch, config);
		print_message(app, error_text);

		active_color_table = color_table;

	}
	else if (string_match(name, string_u8_litexpr("config.4coder"))) {
		View_ID view = get_active_view(app, Access_Always);
		view_enqueue_command_function(app, view, reload_config);
	}
	else if (string_match(name, string_u8_litexpr("bindings.4coder"))) {
		View_ID view = get_active_view(app, Access_Always);
		view_enqueue_command_function(app, view, reload_bindings);
	}

	return(0);
}



function void
draw_error_annotations(Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout, Rect_f32 view_rect, Buffer_ID comp_buffer) {

	Locked_Jump_State jump_state = get_locked_jump_state(app, &global_heap);
	if (!jump_state.view)
		return;

	Scratch_Block scratch(app);

	Managed_Scope scopes[] = {
		buffer_get_managed_scope(app, comp_buffer),
		buffer_get_managed_scope(app, buffer)
	};

	Managed_Scope comp_scope = get_managed_scope_with_multiple_dependencies(app, scopes, ArrayCount(scopes));
	Managed_Object* markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
	if (!markers_object)
		return;

	i32 marker_count = managed_object_get_item_count(app, *markers_object);
	Marker* markers = push_array(scratch, Marker, marker_count);
	managed_object_load_data(app, *markers_object, 0, marker_count, markers);

	Face_ID face = get_view_face_id(app, view);
	Face_Metrics metrics = get_face_metrics(app, face);

	i64 last_error_line = -1;
	for (i32 i = 0; i < marker_count; i++) {
		i64 error_line = get_line_from_list(app, jump_state.list, i);
		i64 line_number = get_line_number_from_pos(app, buffer, markers[i].pos);

		if (line_number == last_error_line)
			continue;
		last_error_line = line_number;

		b32 is_warning = false;
		String_Const_u8 line = push_buffer_line(app, scratch, comp_buffer, error_line);
		u64 msg_start = string_find_first(line, string_u8_litexpr("error"), StringMatch_CaseInsensitive);
		if (msg_start < line.size) {
			msg_start += 6;
		}
		else {
			msg_start = string_find_first(line, string_u8_litexpr("warning"), StringMatch_CaseInsensitive);
			if (msg_start < line.size) {
				msg_start += 8;
				is_warning = true;
			}
			else {
				msg_start = 0;
			}
		}
		line.str += msg_start;
		line.size -= msg_start;

		Rect_f32 last_char = text_layout_character_on_screen(app, layout, get_line_end_pos(app, buffer, line_number) - 1);
		Vec2_f32 position = V2f32(last_char.x1 + metrics.normal_advance * 3, last_char.y0);

		if (rect_width(last_char)) {
			/*
			f32 max_x = view_rect.x1;
			f32 line_advance = get_string_advance(app, face, line);
			if (position.x + line_advance + metrics.max_advance * 2 >= max_x) {
				position.x = max_x - metrics.max_advance * 3 - line_advance;
				position.y += metrics.line_height;
			}
			*/

			ARGB_Color error_color = fcolor_resolve(fcolor_id(is_warning ? defcolor_warning : defcolor_error));
			if (!error_color)
				error_color = is_warning ? 0xFFFFCF4F : 0xFFFF7F7F;

			Rect_f32 square = Rf32(position.x, position.y + 0.5f * (rect_height(last_char) - metrics.max_advance),
				position.x + metrics.max_advance, position.y + 0.5f * (rect_height(last_char) + metrics.max_advance));
			draw_rectangle(app, square, 0, error_color);
			position.x += metrics.max_advance * 2;

			draw_string(app, face, line, position, error_color);
		}
	}
}

function void
render_buffer(Application_Links* app, View_ID view_id, Face_ID face_id,
	Buffer_ID buffer, Text_Layout_ID text_layout_id,
	Rect_f32 rect, f32 file_roundness, Rect_f32 file_rect) {
	ProfileScope(app, "render buffer");

	View_ID active_view = get_active_view(app, Access_Always);
	b32 is_active_view = (active_view == view_id);
	Rect_f32 prev_clip = draw_set_clip(app, rect);

	Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);

	// NOTE(allen): Cursor shape
	Face_Metrics metrics = get_face_metrics(app, face_id);
	u64 cursor_roundness_100 = def_get_config_u64(app, vars_save_string_lit("cursor_roundness"));
	f32 cursor_roundness = metrics.normal_advance * cursor_roundness_100 * 0.01f;
	f32 mark_thickness = (f32)def_get_config_u64(app, vars_save_string_lit("mark_thickness"));

	// NOTE(allen): Token colorizing
	Token_Array token_array = get_token_array_from_buffer(app, buffer);
	if (token_array.tokens != 0) {
		//draw_cpp_token_colors(app, text_layout_id, &token_array);
		F4_SyntaxHighlight(app, text_layout_id, &token_array);

		// NOTE(allen): Scan for TODOs and NOTEs
		b32 use_comment_keyword = def_get_config_b32(vars_save_string_lit("use_comment_keyword"));
		if (use_comment_keyword) {
			Comment_Highlight_Pair pairs[] = {
				{string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
				{string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
			};
			draw_comment_highlights(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
		}

#if 0
		// TODO(allen): Put in 4coder_draw.cpp
		// NOTE(allen): Color functions

		Scratch_Block scratch(app);
		ARGB_Color argb = 0xFFFF00FF;

		Token_Iterator_Array it = token_iterator_pos(0, &token_array, visible_range.first);
		for (;;) {
			if (!token_it_inc_non_whitespace(&it)) {
				break;
			}
			Token* token = token_it_read(&it);
			String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);
			Code_Index_Note* note = code_index_note_from_string(lexeme);
			if (note != 0 && note->note_kind == CodeIndexNote_Function) {
				paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
			}
		}
#endif
	}
	else {
		paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
	}

	i64 cursor_pos = view_correct_cursor(app, view_id);
	view_correct_mark(app, view_id);

	// NOTE(allen): Scope highlight
	b32 use_scope_highlight = def_get_config_b32(vars_save_string_lit("use_scope_highlight"));
	if (use_scope_highlight) {
		Color_Array colors = finalize_color_array(defcolor_back_cycle);
		draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count, file_roundness, file_rect);
	}

	b32 use_error_highlight = def_get_config_b32(vars_save_string_lit("use_error_highlight"));
	b32 use_jump_highlight = def_get_config_b32(vars_save_string_lit("use_jump_highlight"));
	if (use_error_highlight || use_jump_highlight) {
		// NOTE(allen): Error highlight
		String_Const_u8 name = string_u8_litexpr("*compilation*");
		Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
		if (use_error_highlight) {
			draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer, fcolor_id(defcolor_highlight_junk), file_roundness, file_rect);
		}

		// NOTE(allen): Search highlight
		if (use_jump_highlight) {
			Buffer_ID jump_buffer = get_locked_jump_buffer(app);
			if (jump_buffer != compilation_buffer) {
				draw_jump_highlights(app, buffer, text_layout_id, jump_buffer, fcolor_id(defcolor_highlight_white));
			}
		}
	}

	// NOTE(allen): Line highlight
	b32 highlight_line_at_cursor = def_get_config_b32(vars_save_string_lit("highlight_line_at_cursor"));
	if (highlight_line_at_cursor && is_active_view) {
		i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
		draw_line_highlight(app, buffer, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line), file_roundness, file_rect);
	}

	// NOTE(allen): Whitespace highlight
	b64 show_whitespace = false;
	view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
	if (show_whitespace) {
		if (token_array.tokens == 0) {
			draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
		}
		else {
			draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
		}
	}

	// error annotations
	b32 enable_error_annotations = def_get_config_b32(vars_save_string_lit("enable_error_annotations"), true);
	if (enable_error_annotations) {
		Buffer_ID compilation_buffer = get_buffer_by_name(app, string_u8_litexpr("*compilation*"), Access_Always);
		draw_error_annotations(app, view_id, buffer, text_layout_id, rect, compilation_buffer);
	}

	// brace lines
	b32 brace_lines = def_get_config_b32(vars_save_string_lit("draw_brace_lines"), true);
	if (brace_lines) {
		Color_Array colors = finalize_color_array(defcolor_brace_line);
		draw_brace_lines(app, buffer, text_layout_id, rect, metrics, cursor_pos, colors.vals, colors.count);
	}

	// token occurance
	b32 enable_highlight_hovered_symbol = def_get_config_b32(vars_save_string_lit("enable_highlight_hovered_symbol"), true);
	if (enable_highlight_hovered_symbol) {
		ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_symbol_highlight));
		highlight_hovered_symbol(app, view_id, buffer, text_layout_id, cursor_pos, &token_array, color);
	}

	// Color braces
	b32 use_brace_helper = def_get_config_b32(vars_save_string_lit("use_brace_helper"), true);
	if (use_brace_helper) {
		Color_Array colors = finalize_color_array(defcolor_brace_highlight);
		highlight_enclosure_characters(app, view_id, buffer, text_layout_id, cursor_pos,
			TokenBaseKind_ScopeOpen, TokenBaseKind_ScopeClose, FindNest_Scope,
			colors.vals, colors.count);
	}

	// NOTE(allen): Color parens
	b32 use_paren_helper = def_get_config_b32(vars_save_string_lit("use_paren_helper"));
	if (use_paren_helper) {
		Color_Array colors = finalize_color_array(defcolor_brace_highlight);
		highlight_enclosure_characters(app, view_id, buffer, text_layout_id, cursor_pos,
			TokenBaseKind_ParentheticalOpen, TokenBaseKind_ParentheticalClose, FindNest_Paren,
			colors.vals, colors.count);
	}

	// NOTE(allen): Cursor
	switch (fcoder_mode) {
	case FCoderMode_Original:
	{
		Rect_f32 clip = draw_set_clip(app, prev_clip);
		sc_draw_cursor_emacs(app, view_id, is_active_view, buffer, text_layout_id, metrics, cursor_roundness, mark_thickness);
		draw_set_clip(app, clip);
	}break;
	case FCoderMode_NotepadLike:
	{
		// remove clipping rect so the cursor doesn't get clipped by the margin
		// when crossing buffer boundaries
		Rect_f32 clip = draw_set_clip(app, prev_clip);
		sc_draw_cursor_notepad(app, view_id, is_active_view, buffer, text_layout_id, metrics, cursor_roundness, mark_thickness);
		draw_set_clip(app, clip);
	}break;
	}

	// NOTE(allen): Fade ranges
	paint_fade_ranges(app, text_layout_id, buffer);

	// NOTE(allen): put the actual text on the actual screen
	draw_text_layout_default(app, text_layout_id);

	F4_PosContext_Render(app, view_id, buffer, text_layout_id, rect, cursor_pos);

	Managed_Scope scope = view_get_managed_scope(app, active_view);
	Word_Complete_Menu** menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
	Word_Complete_Menu* menu = *menu_ptr;
	if (menu) {
		draw_complete_menu(app, active_view, menu);
	}

	draw_set_clip(app, prev_clip);

	/*
	if (!is_active_view)
	{
		Rect_f32 view_rect = view_get_screen_rect(app, view_id);
		draw_rectangle(app, view_rect, 0.0f, 0x30000000);
	} else
	*/
	if (lister_open && lister_view == view_id) {
		Rect_f32 view_rect = view_get_screen_rect(app, view_id);
		draw_rectangle(app, view_rect, 0.0f, 0x60000000);
	}
}

function void
sc_draw_file_bar(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar) {
	Scratch_Block scratch(app);

	draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));

	FColor base_color = fcolor_id(defcolor_base);
	FColor pop2_color = fcolor_id(defcolor_pop2);

	i64 cursor_position = view_get_cursor_pos(app, view_id);
	Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));

	Face_Metrics metrics = get_face_metrics(app, face_id);

	Fancy_Line list = {};
	String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
	push_fancy_string(scratch, &list, base_color, unique_name);

	u8 space[3];
	{
		Dirty_State dirty = buffer_get_dirty_state(app, buffer);
		String_u8 str = Su8(space, 0, 2);
		if (HasFlag(dirty, DirtyState_UnsavedChanges)) {
			string_append(&str, string_u8_litexpr("*"));
		}
		if (HasFlag(dirty, DirtyState_UnloadedChanges)) {
			string_append(&str, string_u8_litexpr("!"));
		}
		push_fancy_string(scratch, &list, pop2_color, str.string);
	}

	Vec2_f32 p = bar.p0 + V2f32(4, 2 + rect_height(bar) * 0.5f - metrics.line_height * 0.5f);
	draw_fancy_line(app, face_id, fcolor_zero(), &list, p);

	list = {};

	push_fancy_stringf(scratch, &list, base_color, "Line %lld, Col %lld   ", cursor.line, cursor.col);

	Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);

	switch (*eol_setting) {
	case LineEndingKind_Binary:
	{
		push_fancy_string(scratch, &list, base_color, string_u8_litexpr("bin"));
	}break;

	case LineEndingKind_LF:
	{
		push_fancy_string(scratch, &list, base_color, string_u8_litexpr("lf"));
	}break;

	case LineEndingKind_CRLF:
	{
		push_fancy_string(scratch, &list, base_color, string_u8_litexpr("crlf"));
	}break;
	}

	F4_Language* language = F4_LanguageFromBuffer(app, buffer);
	if (language)
	{
		push_fancy_string(scratch, &list, base_color, S8Lit("   "));
		push_fancy_string(scratch, &list, base_color, language->language_name);
	}

	p = V2f32(bar.p1.x - 4 - get_fancy_line_width(app, face_id, &list), bar.p0.y + 2 + rect_height(bar) * 0.5f - metrics.line_height * 0.5f);
	draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

function Rect_f32_Pair
sc_layout_line_numbers(Application_Links* app, Buffer_ID buffer, Rect_f32 rect, f32 digit_advance) {
	i64 line_count = buffer_get_line_count(app, buffer);
	i64 line_count_digit_count = digit_count_from_integer(line_count, 10);
	f32 margin_width = (f32)line_count_digit_count * digit_advance + 5 * digit_advance;
	return(rect_split_left_right(rect, margin_width));
}

function void
sc_draw_line_numbers(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 margin, f32 digit_advance, f32 file_roundness) {
	ProfileScope(app, "draw line number margin");

	Scratch_Block scratch(app);

	Rect_f32 prev_clip = draw_set_clip(app, margin);
	draw_rectangle_fcolor(app, Rf32(margin.p0, V2f32(margin.x1 + file_roundness, margin.y1)), file_roundness, fcolor_id(defcolor_line_numbers_back));

	Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
	i64 line_count = buffer_get_line_count(app, buffer);
	i64 line_count_digit_count = digit_count_from_integer(line_count, 10);

	Fancy_String fstring = {};
	u8* digit_buffer = push_array(scratch, u8, line_count_digit_count);
	String_Const_u8 digit_string = SCu8(digit_buffer, line_count_digit_count);
	for (i32 i = 0; i < line_count_digit_count; i += 1) {
		digit_buffer[i] = ' ';
	}

	//i64 cursor_position = view_get_cursor_pos(app, view_id);
	//Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));

	Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(visible_range.first));
	i64 line_number = cursor.line;

	Buffer_Cursor cursor_opl = view_compute_cursor(app, view_id, seek_pos(visible_range.one_past_last));
	i64 one_past_last_line_number = cursor_opl.line + 1;

	u8* small_digit = digit_buffer + line_count_digit_count - 1;
	{
		u8* ptr = small_digit;
		if (line_number == 0) {
			*ptr = '0';
		}
		else {
			for (u64 X = line_number; X > 0; X /= 10) {
				*ptr = '0' + (X % 10);
				ptr -= 1;
			}
		}
	}

	for (; line_number < one_past_last_line_number &&
		line_number <= line_count;) {
		Range_f32 line_y = text_layout_line_on_screen(app, text_layout_id, line_number);
		Vec2_f32 p = V2f32(margin.x0 + 2 * digit_advance, line_y.min);

		FColor line_color = line_number == cursor.line ? fcolor_id(defcolor_line_numbers_highlight) : fcolor_id(defcolor_line_numbers_text);

		fill_fancy_string(&fstring, 0, line_color, 0, 0, digit_string);
		draw_fancy_string(app, face_id, fcolor_zero(), &fstring, p);

		line_number += 1;
		{
			u8* ptr = small_digit;
			for (;;) {
				if (ptr < digit_buffer) {
					break;
				}
				if (*ptr == ' ') {
					*ptr = '0';
				}
				if (*ptr == '9') {
					*ptr = '0';
					ptr -= 1;
				}
				else {
					*ptr += 1;
					break;
				}
			}
		}
	}

	draw_set_clip(app, prev_clip);
}

function Rect_f32
draw_query_bar(Application_Links* app, Rect_f32 region, View_ID view_id, Face_ID face_id) {
	Face_Metrics face_metrics = get_face_metrics(app, face_id);
	f32 line_height = face_metrics.line_height;

	Query_Bar* space[32];
	Query_Bar_Ptr_Array query_bars = {};
	query_bars.ptrs = space;
	if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)) {
		for (i32 i = 0; i < query_bars.count; i += 1) {
			Rect_f32_Pair pair = layout_query_bar_on_bot(region, line_height, 1);
			draw_rectangle_and_margin(app, pair.max, 0, fcolor_resolve(fcolor_id(defcolor_back)), fcolor_resolve(fcolor_id(defcolor_margin)), 1);
			draw_query_bar(app, query_bars.ptrs[i], face_id, pair.max);
			region = pair.min;
		}
	}
	return(region);
}

function void
sc_render(Application_Links* app, Frame_Info frame_info, View_ID view_id) {
	ProfileScope(app, "default render caller");
	View_ID active_view = get_active_view(app, Access_Always);
	b32 is_active_view = (active_view == view_id);

	Rect_f32 region = view_get_screen_rect(app, view_id);

	Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
	Face_ID face_id = get_face_id(app, buffer);
	Face_Metrics face_metrics = get_face_metrics(app, face_id);
	f32 line_height = face_metrics.line_height;
	f32 digit_advance = face_metrics.decimal_digit_advance;

	// NOTE(allen): file bar
	b64 showing_file_bar = false;
	if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar) {
		Rect_f32_Pair pair = layout_file_bar_on_bot(region, f32_round32(line_height * 1.25f));
		Face_ID file_bar_font = ui_font ? ui_font : face_id;
		sc_draw_file_bar(app, view_id, buffer, file_bar_font, pair.max);
		region = pair.min;
	}

	u64 panel_margin = def_get_config_u64(app, vars_save_string_lit("panel_margin"), 0);
	draw_rectangle(app, region, 0, fcolor_resolve(fcolor_id(defcolor_bar)));
	region = rect_inner(region, f32(panel_margin));

	u64 file_margin = def_get_config_u64(app, vars_save_string_lit("file_margin"), 3);
	u64 file_roundness = def_get_config_u64(app, vars_save_string_lit("file_roundness"), 0);
	FColor margin_color = get_panel_margin_color(is_active_view ? UIHighlight_Active : UIHighlight_None);
	draw_rectangle_and_margin(app, region, f32(file_roundness), fcolor_resolve(fcolor_id(defcolor_back)), fcolor_resolve(margin_color), f32(file_margin));
	region = rect_inner(region, f32(file_margin));
	Rect_f32 file_rect = region;

	Rect_f32 prev_clip = draw_set_clip(app, region);

	Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);

	// NOTE(FS): Scroll animation smoothing with regular dt feels sluggish,
	// so I made the animation go fester
	f32 dt = frame_info.animation_dt * 1.0f;
	Buffer_Point_Delta_Result delta = delta_apply(app, view_id, dt, scroll);
	if (!block_match_struct(&scroll.position, &delta.point)) {
		block_copy_struct(&scroll.position, &delta.point);
		view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
	}
	if (delta.still_animating) {
		animate_in_n_milliseconds(app, 0);
	}

	// NOTE(allen): query bars
	region = draw_query_bar(app, region, view_id, face_id);

	// NOTE(allen): FPS hud
	if (show_fps_hud) {
		Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
		draw_fps_hud(app, frame_info, face_id, pair.max);
		region = pair.min;
		animate_in_n_milliseconds(app, 1000);
	}

	// NOTE(allen): layout line numbers
	b32 show_line_number_margins = def_get_config_b32(vars_save_string_lit("show_line_number_margins"));
	Rect_f32 line_number_rect = {};
	if (show_line_number_margins) {
		Rect_f32_Pair pair = sc_layout_line_numbers(app, buffer, region, digit_advance);
		line_number_rect = pair.min;
		region = pair.max;
	}

	// NOTE(allen): begin buffer render
	Buffer_Point buffer_point = scroll.position;
	Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);

	// NOTE(allen): draw line numbers
	if (show_line_number_margins) {
		sc_draw_line_numbers(app, view_id, buffer, face_id, text_layout_id, line_number_rect, digit_advance, Max(f32(file_roundness) - f32(file_margin), 0));
	}

	// NOTE(allen): draw the buffer
	render_buffer(app, view_id, face_id, buffer, text_layout_id, region, Max(f32(file_roundness) - f32(file_margin), 0), file_rect);

	text_layout_free(app, text_layout_id);
	draw_set_clip(app, prev_clip);
}

function Rect_f32
sc_buffer_region(Application_Links* app, View_ID view_id, Rect_f32 region) {
	Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
	Face_ID face_id = get_face_id(app, buffer);
	Face_Metrics metrics = get_face_metrics(app, face_id);
	f32 line_height = metrics.line_height;
	f32 digit_advance = metrics.decimal_digit_advance;

	// NOTE(allen): file bar
	b64 showing_file_bar = false;
	if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) &&
		showing_file_bar) {
		Rect_f32_Pair pair = layout_file_bar_on_bot(region, f32_round32(line_height * 1.25f));
		region = pair.min;
	}

	// NOTE(allen): margins
	u64 panel_margin = def_get_config_u64(app, vars_save_string_lit("panel_margin"), 0);
	region = rect_inner(region, f32(panel_margin));

	u64 file_margin = def_get_config_u64(app, vars_save_string_lit("file_margin"), 3);
	region = rect_inner(region, f32(file_margin));

	// NOTE(allen): query bars
	{
		Query_Bar* space[32];
		Query_Bar_Ptr_Array query_bars = {};
		query_bars.ptrs = space;
		if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)) {
			Rect_f32_Pair pair = layout_query_bar_on_bot(region, line_height, query_bars.count);
			region = pair.min;
		}
	}

	// NOTE(allen): FPS hud
	if (show_fps_hud) {
		Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
		region = pair.min;
	}

	// NOTE(allen): line numbers
	b32 show_line_number_margins = def_get_config_b32(vars_save_string_lit("show_line_number_margins"));
	if (show_line_number_margins) {
		Rect_f32_Pair pair = sc_layout_line_numbers(app, buffer, region, digit_advance);
		region = pair.max;
	}

	return(region);
}

// BOTTOM

