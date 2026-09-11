/*
4coder_draw.cpp - Layout and rendering implementation of standard UI pieces (including buffers)
*/

// TOP

function void
sc_draw_cursor(Application_Links *app, View_ID view_id, b32 is_active_view,
                                    Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                    f32 roundness, f32 outline_thickness){
    b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);

    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    if (is_active_view) {
        next_cursor_rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
    }

    if (!has_highlight_range) {
        i32 cursor_sub_id = default_cursor_sub_id();
        ARGB_Color color = fcolor_resolve(fcolor_id(defcolor_cursor, cursor_sub_id));

        if (cursor_blink_state) {
            draw_rectangle(app, current_cursor_rect, roundness, color);
        }

        if (is_active_view) {
            if (rect_overlap(next_cursor_rect, current_cursor_rect) && cursor_blink_state) {
                paint_text_color_pos(app, text_layout_id, cursor_pos,
                    fcolor_id(defcolor_at_cursor));
            }
        }
        else {
            draw_character_wire_frame(app, text_layout_id, cursor_pos,
                roundness, outline_thickness,
                color);
        }
    }
}

function void
sc_draw_line_highlight(Application_Links* app, Text_Layout_ID layout, Face_ID face_id, i64 line, FColor color) {
    ARGB_Color argb = fcolor_resolve(color);
    Range_f32 y = text_layout_line_on_screen(app, layout, line);
    if (range_size(y) > 0.f) {
        Face_Metrics metrics = get_face_metrics(app, face_id);
        y.min -= metrics.line_skip / 2;
        y.max += metrics.line_skip / 2;

        Rect_f32 region = text_layout_region(app, layout);
        draw_rectangle(app, Rf32(rect_range_x(region), y), 0.f, argb);
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
        draw_cpp_token_colors(app, text_layout_id, &token_array);

        // NOTE(allen): Scan for TODOs and NOTEs
        b32 use_comment_keyword = def_get_config_b32(vars_save_string_lit("use_comment_keyword"));
        if (use_comment_keyword) {
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
        }

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
    }
    else {
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }

    i64 cursor_pos = view_get_cursor_pos(app, view_id);
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

    // NOTE(allen): Color parens
    b32 use_paren_helper = def_get_config_b32(vars_save_string_lit("use_paren_helper"));
    if (use_paren_helper) {
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }

    // NOTE(allen): Line highlight
    b32 highlight_line_at_cursor = def_get_config_b32(vars_save_string_lit("highlight_line_at_cursor"));
    if (highlight_line_at_cursor && is_active_view) {
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        sc_draw_line_highlight(app, text_layout_id, face_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
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
        sc_draw_cursor(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        draw_set_clip(app, clip);
    }break;
    }

    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer);

    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);

    draw_set_clip(app, prev_clip);
}

function void
sc_draw_file_bar(Application_Links* app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar) {
    Scratch_Block scratch(app);

    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));

    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);

    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));

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

    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
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

    /*
    F4_Language* language = F4_LanguageFromBuffer(app, buffer);
    if (language)
    {
        push_fancy_string(scratch, &list, base_color, S8Lit("   "));
        push_fancy_string(scratch, &list, base_color, language->name);
    }
    */

    p = V2f32(bar.p1.x - 2 - get_fancy_line_width(app, face_id, &list), bar.p0.y + 2);
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
    draw_rectangle_fcolor(app, margin, 0.f, fcolor_id(defcolor_back));

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

        FColor line_color = line_number == cursor.line ? fcolor_id(defcolor_bar) : fcolor_id(defcolor_line_numbers_text);

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

    u64 margin_width = def_get_config_u64(app, vars_save_string_lit("margin_width"), 3);
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
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        sc_draw_file_bar(app, view_id, buffer, face_id, pair.max);
        region = pair.min;
    }

    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);

    // NOTE(FS): Scroll animation smoothing with regular dt feels sluggish,
    // so I made the animation go fester
    f32 dt = frame_info.animation_dt * 3.f;
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
    u64 margin_width = def_get_config_u64(app, vars_save_string_lit("margin_width"), 3);
    region = rect_inner(region, f32(margin_width));

    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar) {
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
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

