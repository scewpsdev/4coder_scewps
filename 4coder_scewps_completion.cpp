/*
4coder_scewps_completion.cpp
*/

// TOP

CUSTOM_COMMAND_SIG(open_completion_list)
{
    View_ID view = get_this_ctx_view(app, Access_Always);
    View_Context ctx = view_current_context(app, view);
    Render_Caller_Function* prev_render_caller = ctx.render_caller;

    Word_Complete_Iterator* it = word_complete_get_shared_iter(app);

    i64 pos = view_get_cursor_pos(app, view);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_word_complete_needle_range(app, buffer, pos);
    if (range_size(range) != 0) {
        word_complete_iter_init(buffer, range, it);
        complete_menu = make_word_complete_menu(prev_render_caller, it);
        word_complete_menu_next(&complete_menu);

        Managed_Scope scope = view_get_managed_scope(app, view);
        Word_Complete_Menu** menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
        *menu_ptr = &complete_menu;

#if 0
        b32 keep_looping_menu = true;
        for (; keep_looping_menu;) {
            User_Input in = get_next_input(app, EventPropertyGroup_Any,
                EventProperty_Escape);
            if (in.abort) {
                break;
            }

            switch (in.event.kind) {
            case InputEventKind_TextInsert:
            {
                pos = view_get_cursor_pos(app, view);
                range = get_word_complete_needle_range(app, buffer, pos);
                if (range_size(range) == 0) {
                    keep_looping_menu = false;
                }
                else {
                    word_complete_iter_init(buffer, range, it);
                    menu = make_word_complete_menu(prev_render_caller, it);
                    word_complete_menu_next(&menu);
                    if (menu.count == 0) {
                        keep_looping_menu = false;
                    }
                }
            }break;

            case InputEventKind_KeyStroke:
            {
                switch (in.event.key.code) {
                case KeyCode_Return:
                {
                    keep_looping_menu = false;
                }break;

                case KeyCode_Tab:
                {
                    word_complete_menu_next(&menu);
                }break;

                case KeyCode_F1:
                case KeyCode_F2:
                case KeyCode_F3:
                case KeyCode_F4:
                case KeyCode_F5:
                case KeyCode_F6:
                case KeyCode_F7:
                case KeyCode_F8:
                {
                    keep_looping_menu = false;
                }break;

                case KeyCode_Backspace:
                {
                    pos = view_get_cursor_pos(app, view);
                    range = get_word_complete_needle_range(app, buffer, pos);
                    if (range_size(range) == 0) {
                        keep_looping_menu = false;
                    }
                    else {
                        word_complete_iter_init(buffer, range, it);
                        menu = make_word_complete_menu(prev_render_caller, it);
                        word_complete_menu_next(&menu);
                        if (menu.count == 0) {
                            keep_looping_menu = false;
                        }
                    }
                }break;

                default:
                {
                }break;
                }
            }break;

            case InputEventKind_MouseButton:
            {
                keep_looping_menu = false;
            }break;

            default:
            {
            }break;
            }

            leave_current_input_unhandled(app);
        }
#endif
    }
}

function void
close_completion_list(Application_Links* app, View_ID view) {
    Managed_Scope scope = view_get_managed_scope(app, view);
    Word_Complete_Menu** menu_ptr = scope_attachment(app, scope, view_word_complete_menu, Word_Complete_Menu*);
    *menu_ptr = 0;
    complete_menu = {};
}

function void
completion_list_on_event(Application_Links* app, View_ID view, Word_Complete_Menu* menu, User_Input input) {
    b32 closed = false;

    switch (input.event.kind) {
    case InputEventKind_TextInsert:
    {
        i64 pos = view_get_cursor_pos(app, view);
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        Range_i64 range = get_word_complete_needle_range(app, buffer, pos);
        if (range_size(range) == 0) {
            closed = true;
        } else {
            View_Context ctx = view_current_context(app, view);
            Render_Caller_Function* prev_render_caller = ctx.render_caller;

            Word_Complete_Iterator* it = word_complete_get_shared_iter(app);

            word_complete_iter_init(buffer, range, it);
            complete_menu = make_word_complete_menu(prev_render_caller, it);
            word_complete_menu_next(&complete_menu);
            if (complete_menu.count == 0) {
                closed = true;
            }
        }
    }break;

    case InputEventKind_KeyStroke:
    {
        switch (input.event.key.code) {
        case KeyCode_Backspace:
        {
            i64 pos = view_get_cursor_pos(app, view);
            Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
            Range_i64 range = get_word_complete_needle_range(app, buffer, pos);
            if (range_size(range) == 0) {
                closed = true;
            }
            else {
                View_Context ctx = view_current_context(app, view);
                Render_Caller_Function* prev_render_caller = ctx.render_caller;

                Word_Complete_Iterator* it = word_complete_get_shared_iter(app);

                word_complete_iter_init(buffer, range, it);
                complete_menu = make_word_complete_menu(prev_render_caller, it);
                word_complete_menu_next(&complete_menu);
                if (complete_menu.count == 0) {
                    closed = true;
                }
            }
        }break;

        case KeyCode_Shift:
        case KeyCode_Control:
        case KeyCode_Alt:
        case KeyCode_Command:
        {
        } break;

        default:
        {
            closed = true;
        }break;
        }

    }break;

    case InputEventKind_MouseButton:
    {
        closed = true;
    }break;

    default:
    {
    }break;
    }

    if (closed) {
        close_completion_list(app, view);
    }
}

function Rect_f32
get_contained_box_near_point(Application_Links* app, Rect_f32 container, Vec2_f32 p, Vec2_f32 box_dims, Face_ID face) {
    Vec2_f32 container_dims = rect_dim(container);
    box_dims.x = clamp_top(box_dims.x, container_dims.x);
    box_dims.y = clamp_top(box_dims.y, container_dims.y);

    Face_Metrics metrics = get_face_metrics(app, face);

    Vec2_f32 q = p + V2f32(metrics.max_advance * -0.5f, metrics.line_height);
    if (q.x + box_dims.x > container.x1) {
        q.x = container.x1 - box_dims.x;
    }
    if (q.y + box_dims.y > container.y1) {
        q.y = p.y - box_dims.y - metrics.line_height;
        if (q.y < container.y0) {
            q.y = (container.y0 + container.y1 - box_dims.y) * 0.5f;
        }
    }
    return(Rf32_xy_wh(q, box_dims));
}

function void
draw_completion_dropdown(Application_Links* app, Face_ID face, Fancy_Block* block,
    Vec2_f32 p, Rect_f32 region, Vec2_f32 padding,
    FColor outline_color, FColor back_color, FColor highlight_color) {
    Rect_f32 box = Rf32(p, p);
    if (block->line_count > 0) {
        Vec2_f32 dims = get_fancy_block_dim(app, face, block);
        dims += padding * 2;
        box = get_contained_box_near_point(app, region, p, dims, face);
        box.x0 = f32_round32(box.x0);
        box.y0 = f32_round32(box.y0);
        box.x1 = f32_round32(box.x1);
        box.y1 = f32_round32(box.y1);

        Rect_f32 prev_clip = draw_set_clip(app, box);
        draw_rectangle_fcolor(app, box, 0.f, outline_color);
        box = rect_inner(box, 1.f);
        draw_rectangle_fcolor(app, box, 0.f, back_color);

        f32 element_height = get_fancy_line_height(app, face, block->first);
        draw_rectangle_fcolor(app, Rf32(box.x0, box.y0, box.x1, box.y0 + element_height), 0, highlight_color);

        draw_fancy_block(app, face, fcolor_zero(), block,
            box.p0 + padding);

        draw_set_clip(app, prev_clip);
    }
}

function void
draw_complete_menu(Application_Links* app, View_ID view, Word_Complete_Menu* menu) {
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Face_ID face = get_face_id(app, buffer);

    Scratch_Block scratch(app);

    Fancy_Block block = {};
    for (i32 i = 0; i < menu->count; i += 1) {
        if (menu->options[i].size > 0) {
            Fancy_Line* line = push_fancy_line(scratch, &block, face);
            push_fancy_string(scratch, line, fcolor_id(defcolor_text_default), menu->options[i]);
        }
    }

    Rect_f32 region = view_get_buffer_region(app, view);

    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    Buffer_Point buffer_point = scroll.position;
    i64 pos = view_get_cursor_pos(app, view);
    Vec2_f32 cursor_p = view_relative_xy_of_pos(app, view, buffer_point.line_number, pos);
    cursor_p -= buffer_point.pixel_shift;
    cursor_p += region.p0;

    Face_Metrics metrics = get_face_metrics(app, face);
    f32 x_padding = metrics.normal_advance;

    draw_completion_dropdown(app, face, &block, cursor_p, region, V2f32(x_padding, 2),
        fcolor_id(defcolor_margin_hover), fcolor_id(defcolor_back), fcolor_id(defcolor_highlight_cursor_line));
}









function Rect_f32
draw_tooltip(Application_Links* app, Rect_f32 region, Vec2_f32 p, Vec2_f32 size, FColor outline_color, FColor back_color) {
    Rect_f32 rect = get_contained_box_near_point(region, p, size);
    rect.x0 = f32_round32(rect.x0);
    rect.y0 = f32_round32(rect.y0);
    rect.x1 = f32_round32(rect.x1);
    rect.y1 = f32_round32(rect.y1);

    draw_rectangle_fcolor(app, rect, 0.f, outline_color);
    rect = rect_inner(rect, 1.f);
    draw_rectangle_fcolor(app, rect, 0.f, back_color);

    return rect;
}

internal Vec2_f32
_F4_PosContext_RenderDefinitionTokens(Application_Links* app, Face_ID face,
    String_Const_u8 backing_string,
    Token_Array tokens, b32 do_render,
    int highlight_arg, Vec2_f32 text_position,
    f32 max_x)
{
    Scratch_Block scratch(app);
    Vec2_f32 starting_text_pos = text_position;

    Vec2_f32 arg_start = starting_text_pos;
    Vec2_f32 arg_end = arg_start;

    Face_Metrics metrics = get_face_metrics(app, face);

    Token_Iterator_Array it = token_iterator_pos(0, &tokens, 0);
    b32 found_first_open_paren = 0;
    for (int arg_idx = 0;;)
    {
        Token* token = token_it_read(&it);
        if (token == 0) { break; }

        String_Const_u8 token_string = string_substring(backing_string, Ii64(token));
        f32 advance = get_string_advance(app, face, token_string);

        if (token->kind == TokenBaseKind_Whitespace)
        {
            b32 after_arg_start = false;
            if (arg_start == text_position)
                after_arg_start = true;
            text_position.x += advance;
            if (after_arg_start)
                arg_start = text_position;
        }
        else
        {
            b32 highlight = 0;

            ARGB_Color color = finalize_color(defcolor_text_default, 0);
            if (token->kind == TokenBaseKind_Identifier && token == it.tokens) {
                color = finalize_color(fleury_color_index_function, 0);
            } else if (token->kind == TokenBaseKind_Keyword) {
                color = finalize_color(defcolor_keyword, 0);
            }
            else if (token->kind >= TokenBaseKind_ScopeOpen && token->kind <= TokenBaseKind_ParentheticalClose) {
                color = finalize_color(fleury_color_syntax_crap, 0);
            } else if (token->kind == TokenBaseKind_Operator) {
                color = finalize_color(fleury_color_operators, 0);
            }

            if (token->kind == TokenBaseKind_StatementClose) {
                if (string_match(token_string, S8Lit(",")))
                {
                    if (arg_idx == highlight_arg) {
                        highlight = true;
                    }
                    arg_end = text_position;
                    arg_idx++;
                }
            } else if (token->kind == TokenBaseKind_ParentheticalOpen) {
                if (!found_first_open_paren) {
                    found_first_open_paren = true;
                    arg_start = text_position + V2f32(advance, 0);
                }
            } else if (token->kind == TokenBaseKind_ParentheticalClose) {
                if (arg_idx == highlight_arg) {
                    highlight = true;
                    arg_end = text_position;
                }
            }

            if (text_position.x + advance >= max_x)
            {
                text_position.x = starting_text_pos.x;
                text_position.y += metrics.line_height;
            }
            if (do_render)
            {
                draw_string(app, face, token_string, text_position, color);
            }

            text_position.x += advance;

            if (highlight && do_render)
            {
                if (arg_end.y > arg_start.y) {
                    draw_rectangle(app, Rf32(arg_start.x, arg_start.y + metrics.line_height,
                        max_x, arg_start.y + metrics.line_height + 2.f),
                        0, finalize_color(defcolor_symbol_highlight, 0));
                    i32 inbetween_lines = (i32)f32_round32((arg_end.y - arg_start.y) / metrics.line_height) - 1;
                    for (i32 i = 0; i < inbetween_lines; i++) {
                        draw_rectangle(app, Rf32(starting_text_pos.x, arg_start.y + (i + 2) * metrics.line_height,
                            max_x, arg_start.y + (i + 2) * metrics.line_height + 2.f),
                            0, finalize_color(defcolor_symbol_highlight, 0));
                    }
                    draw_rectangle(app, Rf32(starting_text_pos.x, arg_end.y + metrics.line_height,
                        arg_end.x, arg_end.y + metrics.line_height + 2.f),
                        0, finalize_color(defcolor_symbol_highlight, 0));
                } else {
                    draw_rectangle(app, Rf32(arg_start.x, arg_start.y + metrics.line_height,
                        arg_end.x, arg_start.y + metrics.line_height + 2.f),
                        0, finalize_color(defcolor_symbol_highlight, 0));
                }
            }

            if (token->kind == TokenBaseKind_StatementClose) {
                if (string_match(token_string, S8Lit(",")))
                {
                    arg_start = text_position;
                }
            }
        }

        if (token_it_inc_all(&it) == 0)
        {
            break;
        }
    }
    return text_position;
}

internal void
F4_PosContext_Render(Application_Links* app, View_ID view, Buffer_ID buffer, Text_Layout_ID text_layout_id, Rect_f32 view_rect, i64 pos)
{
    if (def_get_config_b32(vars_save_string_lit("f4_disable_poscontext")))
    {
        return;
    }

    ProfileScope(app, "[F4] Pos Context Rendering");
    Scratch_Block scratch(app);

    Rect_f32 cursor_rect = text_layout_character_on_screen(app, text_layout_id, pos);
    Face_ID face = small_code_font ? small_code_font : get_view_face_id(app, view);
    Face_Metrics metrics = get_face_metrics(app, face);
    F4_Language* language = F4_LanguageFromBuffer(app, buffer);
    f32 padding = 4.f;

    if (language != 0)
    {

        b32 render_at_cursor = 1;
        if (def_get_config_b32(vars_save_string_lit("f4_poscontext_draw_at_bottom_of_buffer")))
        {
            render_at_cursor = 0;
        }

        Vec2_f32 tooltip_position =
        {
            cursor_rect.x0,
            cursor_rect.y0,
        };

        F4_Language_PosContextData* ctx_list = language->PosContext(app, scratch, buffer, pos);
        if (render_at_cursor == 0)
        {
            f32 height = 0;
            for (F4_Language_PosContextData* ctx = ctx_list; ctx; ctx = ctx->next)
            {
                height += metrics.line_height + 2 * padding;
            }
            tooltip_position = V2f32(view_rect.x0, view_rect.y1 - height);
        }

        for (F4_Language_PosContextData* ctx = ctx_list; ctx; ctx = ctx->next)
        {
            F4_Index_Note* note = ctx->relevant_note;
            if (note != 0 && note->file != 0)
            {

                //~ NOTE(rjf): Function arguments.
                if (note->kind == F4_Index_NoteKind_Function ||
                    note->kind == F4_Index_NoteKind_Macro)
                {

                    // NOTE(rjf): Find range of definition + params
                    Range_i64 definition_range = note->range;
                    {
                        Token_Array defbuffer_tokens = get_token_array_from_buffer(app, note->file->buffer);
                        Token_Iterator_Array it = token_iterator_pos(0, &defbuffer_tokens, note->range.min);
                        int paren_nest = 0;
                        for (; token_it_inc_all(&it);)
                        {
                            Token* token = token_it_read(&it);
                            if (token)
                            {
                                if (token->kind == TokenBaseKind_ParentheticalOpen)
                                {
                                    paren_nest += 1;
                                }
                                if (token->kind == TokenBaseKind_ParentheticalClose)
                                {
                                    paren_nest -= 1;
                                }
                                if (token->kind == TokenBaseKind_ScopeOpen || token->kind == TokenBaseKind_StatementClose) {
                                    if (paren_nest == 0)
                                    {
                                        definition_range.max = token->pos + token->size;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    String_Const_u8 definition_string = push_buffer_range(app, scratch, note->file->buffer, definition_range);
                    Token_Array definition_tokens = sc_token_array_from_text(app, scratch, note->file->buffer, definition_string);

                    // NOTE(rjf): Calculate needed size for this tooltip.
                    f32 max_x = view_rect.x1; // - view_rect.x0;
                    Vec2_f32 end_draw_position = _F4_PosContext_RenderDefinitionTokens(app, face, definition_string, definition_tokens,
                        false, 0, V2f32(0, 0), max_x);
                    Vec2_f32 needed_size =
                    {
                        (end_draw_position.y > 0) ? max_x : end_draw_position.x,
                        end_draw_position.y,
                    };

                    needed_size.x += 2 * padding;
                    needed_size.y += metrics.line_height + 2 * padding;

                    Rect_f32 draw_rect = draw_tooltip(app, view_rect, tooltip_position, needed_size, fcolor_id(defcolor_margin_hover), fcolor_id(defcolor_back));
                    //F4_DrawTooltipRect(app, draw_rect);

                    // NOTE(rjf): Render tokens of definition
                    {
                        Vec2_f32 text_position =
                        {
                            draw_rect.x0 + padding,
                            draw_rect.y0 + padding,
                        };

                        _F4_PosContext_RenderDefinitionTokens(app, face, definition_string, definition_tokens,
                            true, ctx->argument_index,
                            text_position,
                            view_rect.x1);
                    }

                    f32 advance = draw_rect.y1 - draw_rect.y0;
                    tooltip_position.y += advance;
                }
                else if (note->kind == F4_Index_NoteKind_Type)
                {
                    Token_Array defbuffer_tokens = get_token_array_from_buffer(app, note->file->buffer);
                    for (F4_Index_Note* member = note->first_child; member; member = member->next_sibling)
                    {

                        Range_i64 member_range = member->range;
                        Token_Iterator_Array it = token_iterator_pos(0, &defbuffer_tokens, member->range.min);
                        for (;;)
                        {
                            Token* token = token_it_read(&it);
                            if (token)
                            {
                                if (token->kind == TokenBaseKind_StatementClose)
                                {
                                    member_range.max = token->pos;
                                    break;
                                }
                            }
                            else { break; }
                            if (!token_it_inc_non_whitespace(&it))
                            {
                                break;
                            }
                        }

                        String_Const_u8 member_string = push_buffer_range(app, scratch, note->file->buffer, member_range);

                        Vec2_f32 needed_size = { get_string_advance(app, face, member_string), 0, };

                        needed_size.x += 2 * padding;
                        needed_size.y += metrics.line_height + 2 * padding;

                        Rect_f32 draw_rect = draw_tooltip(app, view_rect, tooltip_position, needed_size, fcolor_id(defcolor_margin_hover), fcolor_id(defcolor_back));

                        Vec2_f32 text_position =
                        {
                            draw_rect.x0 + padding,
                            draw_rect.y0 + padding,
                        };

                        draw_string(app, face, member_string, text_position, finalize_color(defcolor_text_default, 0));

                        f32 advance = draw_rect.y1 - draw_rect.y0;
                        tooltip_position.y += advance;
                    }
                }
            }
        }
    }
}

// BOTTOM

