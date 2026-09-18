/*
4coder_draw.cpp - Layout and rendering implementation of standard UI pieces (including buffers)
*/

// TOP

function void
sc_draw_cursor(Application_Links* app, View_ID view_id, b32 is_active_view,
	Buffer_ID buffer, Text_Layout_ID text_layout_id, Face_Metrics metrics,
	f32 roundness, f32 outline_thickness) {
	b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);

	i64 cursor_pos = view_get_cursor_pos(app, view_id);
	if (is_active_view) {
		next_cursor_rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
	}

	if (!has_highlight_range) {
		i32 cursor_sub_id = default_cursor_sub_id();
		i64 mark_pos = view_get_mark_pos(app, view_id);

		if (cursor_pos != mark_pos) {
			Range_i64 range = Ii64(cursor_pos, mark_pos);
			draw_character_block(app, text_layout_id, range, roundness, fcolor_id(defcolor_highlight));
			paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
		}

		ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_cursor, cursor_sub_id));

		if (cursor_blink_state) {
			Rect_f32 rect = current_cursor_rect;
			draw_rectangle(app, rect, roundness, color);
		}

		if (is_active_view) {
			if (rect_overlap(next_cursor_rect, current_cursor_rect) && cursor_blink_state) {
				paint_text_color_pos(app, text_layout_id, cursor_pos,
					fcolor_id(defcolor_at_cursor));
			}
		} else {
			Rect_f32 rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
			draw_rectangle_outline(app, rect, roundness, outline_thickness, color);
		}
	}
}

function void
sc_draw_line_highlight(Application_Links* app, Text_Layout_ID layout, Face_Metrics metrics, i64 line, FColor color) {
	ARGB_Color argb = fcolor_resolve(color);
	Range_f32 y = text_layout_line_on_screen(app, layout, line);
	if (range_size(y) > 0.f) {
		y.min -= metrics.line_skip / 2;
		y.max += metrics.line_skip / 2;

		Rect_f32 region = text_layout_region(app, layout);
		draw_rectangle(app, Rf32(rect_range_x(region), y), 0.f, argb);
	}
}

function void
highlight_enclosure_characters(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout, i64 pos, Token_Base_Kind openKind, Token_Base_Kind closeKind, Find_Nest_Flag findNestFlag, ARGB_Color* colors, i32 color_count) {
	Token_Array token_array = get_token_array_from_buffer(app, buffer);
	if (token_array.tokens != 0) {
		Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
		Token* token = token_it_read(&it);
		if (token != 0 && token->kind == openKind) {
			pos = token->pos + token->size;
		}
		else {
			if (token_it_dec_all(&it)) {
				token = token_it_read(&it);
				if (token->kind == closeKind &&
					pos == token->pos + token->size) {
					pos = token->pos;
				}
			}
		}
	}
	draw_enclosures(app, layout, buffer,
		pos, findNestFlag, RangeHighlightKind_CharacterHighlight,
		colors, color_count, 0, 0);
}

function void
draw_brace_lines(Application_Links* app, Buffer_ID buffer, Text_Layout_ID layout, Rect_f32 region, Face_Metrics metrics, i64 pos, ARGB_Color* colors, i32 color_count) {
	Token_Array token_array = get_token_array_from_buffer(app, buffer);
	if (token_array.tokens != 0) {
		Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
		Token* token = token_it_read(&it);
		if (token != 0 && token->kind == TokenBaseKind_ScopeOpen) {
			pos = token->pos + token->size;
		}
		else {
			if (token_it_dec_all(&it)) {
				token = token_it_read(&it);
				if (token->kind == TokenBaseKind_ScopeClose &&
					pos == token->pos + token->size) {
					pos = token->pos;
				}
			}
		}
	}

	Scratch_Block scratch(app);
	Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, FindNest_Scope);

	i32 color_index = 0;
	for (i32 i = ranges.count - 1; i >= 0; i -= 1) {
		Range_i64 range = ranges.ranges[i];
		Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);

		f32 y_start = text_layout_line_on_screen(app, layout, line_range.min).max + metrics.line_skip;
		f32 y_end = text_layout_line_on_screen(app, layout, line_range.max).min - metrics.line_skip;

		u64 indentation_width = def_get_config_u64(app, vars_save_string_lit("virtual_whitespace_regular_indent"));

		f32 xoffset = (ranges.count - i - 1) * (indentation_width * metrics.space_advance) + 0.25f * metrics.space_advance;

		Rect_f32 brace_line = {};
		brace_line.x0 = region.x0 + xoffset;
		brace_line.x1 = region.x0 + xoffset + 1;
		brace_line.y0 = y_start;
		brace_line.y1 = y_end;

		ARGB_Color color = colors[i % color_count];

		draw_rectangle(app, brace_line, 0, color);

		color_index += 1;
	}
}

function void
draw_symbol_highlight(Application_Links* app, View_ID view, Text_Layout_ID layout, Range_i64 range, ARGB_Color color) {
	Rect_f32 start_rect = text_layout_character_on_screen(app, layout, range.start);
	Rect_f32 end_rect = text_layout_character_on_screen(app, layout, range.end - 1);

	Rect_f32 rect = {};
	rect.x0 = start_rect.x0;
	rect.x1 = end_rect.x1;
	rect.y0 = start_rect.y1 - 1;
	rect.y1 = end_rect.y1 + 1;

	draw_rectangle(app, rect, 0, color);
}

function void
highlight_hovered_symbol(Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout, i64 pos, Token_Array* token_array, ARGB_Color color) {
	Scratch_Block scratch(app);

	Token* cursor_token = get_token_from_pos(app, buffer, pos);
	if (cursor_token != 0 && cursor_token->size > 0 && cursor_token->kind == TokenBaseKind_Identifier) {
		Range_i64 range = Ii64(cursor_token);
		String_Const_u8 symbol_name = push_buffer_range(app, scratch, buffer, range);

		Range_i64 visible_range = text_layout_get_visible_range(app, layout);
		i64 first_index = token_index_from_pos(token_array, visible_range.first);
		Token_Iterator_Array it = token_iterator_index(0, token_array, first_index);
		for (;;)
		{
			Token* token = token_it_read(&it);
			if (!token || token->pos >= visible_range.one_past_last)
			{
				break;
			}

			if (token->kind == TokenBaseKind_Identifier)
			{
				Range_i64 token_range = Ii64(token);
				String_Const_u8 token_string = push_buffer_range(app, scratch, buffer, token_range);

				if (string_match(token_string, symbol_name))
				{
					draw_symbol_highlight(app, view, layout, token_range, color);
				}
			}

			if (!token_it_inc_non_whitespace(&it))
			{
				break;
			}
		}
	}
}

function void draw_error_annotations(Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID layout, Buffer_ID comp_buffer) {

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

	i32 marker_count = managed_object_get_item_count(app, *markers_object);
	Marker* markers = push_array(scratch, Marker, marker_count);
	managed_object_load_data(app, *markers_object, 0, marker_count, markers);

	Face_ID face = get_view_face_id(app, view);
	Face_Metrics metrics = get_face_metrics(app, face);

	for (i32 i = 0; i < marker_count; i++) {
		i64 error_line = get_line_from_list(app, jump_state.list, i);
		i64 line_number = get_line_number_from_pos(app, buffer, markers[i].pos);

		b32 is_warning = false;
		String_Const_u8 line = push_buffer_line(app, scratch, comp_buffer, error_line);
		u64 msg_start = string_find_first(line, string_u8_litexpr("error"), StringMatch_CaseInsensitive);
		if (msg_start < line.size) {
			msg_start += 6;
		} else {
			msg_start = string_find_first(line, string_u8_litexpr("warning"), StringMatch_CaseInsensitive);
			if (msg_start < line.size) {
				msg_start += 8;
				is_warning = true;
			} else {
				msg_start = 0;
			}
		}
		line.str += msg_start;
		line.size -= msg_start;

		Rect_f32 last_char = text_layout_character_on_screen(app, layout, get_line_end_pos(app, buffer, line_number) - 1);
		Vec2_f32 position = V2f32(last_char.x1 + metrics.max_advance * 5, last_char.y0);

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

function void
sc_render_buffer(Application_Links* app, View_ID view_id, Face_ID face_id,
	Buffer_ID buffer, Text_Layout_ID text_layout_id,
	Rect_f32 rect) {
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
		draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
	}

	b32 use_error_highlight = def_get_config_b32(vars_save_string_lit("use_error_highlight"));
	b32 use_jump_highlight = def_get_config_b32(vars_save_string_lit("use_jump_highlight"));
	if (use_error_highlight || use_jump_highlight) {
		// NOTE(allen): Error highlight
		String_Const_u8 name = string_u8_litexpr("*compilation*");
		Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
		if (use_error_highlight) {
			draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
				fcolor_id(defcolor_highlight_junk));
		}

		// NOTE(allen): Search highlight
		if (use_jump_highlight) {
			Buffer_ID jump_buffer = get_locked_jump_buffer(app);
			if (jump_buffer != compilation_buffer) {
				draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
					fcolor_id(defcolor_highlight_white));
			}
		}
	}

	// NOTE(allen): Line highlight
	b32 highlight_line_at_cursor = def_get_config_b32(vars_save_string_lit("highlight_line_at_cursor"));
	if (highlight_line_at_cursor && is_active_view) {
		i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
		sc_draw_line_highlight(app, text_layout_id, metrics, line_number, fcolor_id(defcolor_highlight_cursor_line));
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
		draw_error_annotations(app, view_id, buffer, text_layout_id, compilation_buffer);
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
		highlight_enclosure_characters(app, buffer, text_layout_id, cursor_pos,
			TokenBaseKind_ScopeOpen, TokenBaseKind_ScopeClose, FindNest_Scope,
			colors.vals, colors.count);
	}

	// NOTE(allen): Color parens
	b32 use_paren_helper = def_get_config_b32(vars_save_string_lit("use_paren_helper"));
	if (use_paren_helper) {
		Color_Array colors = finalize_color_array(defcolor_text_cycle);
		highlight_enclosure_characters(app, buffer, text_layout_id, cursor_pos, 
			TokenBaseKind_ParentheticalOpen, TokenBaseKind_ParentheticalClose, FindNest_Paren,
			colors.vals, colors.count);
	}

	// NOTE(allen): Cursor
	switch (fcoder_mode) {
	case FCoderMode_Original:
	{
		draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
	}break;
	case FCoderMode_NotepadLike:
	{
		// remove clipping rect so the cursor doesn't get clipped by the margin
		// when crossing buffer boundaries
		Rect_f32 clip = draw_set_clip(app, prev_clip);
		sc_draw_cursor(app, view_id, is_active_view, buffer, text_layout_id, metrics, cursor_roundness, mark_thickness);
		draw_set_clip(app, clip);
	}break;
	}

	// NOTE(allen): Fade ranges
	paint_fade_ranges(app, text_layout_id, buffer);

	// NOTE(allen): put the actual text on the actual screen
	draw_text_layout_default(app, text_layout_id);

	Managed_Scope scope = view_get_managed_scope(app, active_view);
	Word_Complete_Menu** menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
	Word_Complete_Menu* menu = *menu_ptr;
	if (menu) {
		draw_complete_menu(app, active_view, menu);
	}

	F4_PosContext_Render(app, view_id, buffer, text_layout_id, cursor_pos);

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
	f32 margin_width = (f32)line_count_digit_count * digit_advance + 4 * digit_advance;
	return(rect_split_left_right(rect, margin_width));
}

function void
sc_draw_line_numbers(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 margin, f32 digit_advance) {
	ProfileScope(app, "draw line number margin");

	Scratch_Block scratch(app);

	Rect_f32 prev_clip = draw_set_clip(app, margin);
	draw_rectangle_fcolor(app, margin, 0.f, fcolor_id(defcolor_line_numbers_back));

	Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
	i64 line_count = buffer_get_line_count(app, buffer);
	i64 line_count_digit_count = digit_count_from_integer(line_count, 10);

	Fancy_String fstring = {};
	u8* digit_buffer = push_array(scratch, u8, line_count_digit_count);
	String_Const_u8 digit_string = SCu8(digit_buffer, line_count_digit_count);
	for (i32 i = 0; i < line_count_digit_count; i += 1) {
		digit_buffer[i] = ' ';
	}

	i64 cursor_position = view_get_cursor_pos(app, view_id);
	Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));

	i64 line_number = view_compute_cursor(app, view_id, seek_pos(visible_range.first)).line;

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
		line_number < line_count;) {
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

function void
sc_render(Application_Links* app, Frame_Info frame_info, View_ID view_id) {
	ProfileScope(app, "default render caller");
	View_ID active_view = get_active_view(app, Access_Always);
	b32 is_active_view = (active_view == view_id);

	u64 margin_width = def_get_config_u64(app, vars_save_string_lit("file_margin"), 3);
	FColor margin_color = get_panel_margin_color(is_active_view ? UIHighlight_Active : UIHighlight_None);
	Rect_f32 region = draw_background_and_margin(app, view_id, margin_color, fcolor_id(defcolor_back), f32(margin_width));
	Rect_f32 prev_clip = draw_set_clip(app, region);

	Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
	Face_ID face_id = get_face_id(app, buffer);
	Face_Metrics face_metrics = get_face_metrics(app, face_id);
	f32 line_height = face_metrics.line_height;
	f32 digit_advance = face_metrics.decimal_digit_advance;

	// NOTE(allen): file bar
	b64 showing_file_bar = false;
	if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar) {
		Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height * 1.25f);
		Face_ID file_bar_font = ui_font ? ui_font : face_id;
		sc_draw_file_bar(app, view_id, buffer, file_bar_font, pair.max);
		region = pair.min;
	}

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
	region = default_draw_query_bars(app, region, view_id, face_id);

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
		sc_draw_line_numbers(app, view_id, buffer, face_id, text_layout_id, line_number_rect, digit_advance);
	}

	// NOTE(allen): draw the buffer
	sc_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);

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

	// NOTE(allen): margins
	u64 margin_width = def_get_config_u64(app, vars_save_string_lit("file_margin"), 3);
	region = rect_inner(region, f32(margin_width));

	// NOTE(allen): file bar
	b64 showing_file_bar = false;
	if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) &&
		showing_file_bar) {
		Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height * 1.25f);
		region = pair.min;
	}

	// NOTE(allen): query bars
	{
		Query_Bar* space[32];
		Query_Bar_Ptr_Array query_bars = {};
		query_bars.ptrs = space;
		if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)) {
			Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, query_bars.count);
			region = pair.max;
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

