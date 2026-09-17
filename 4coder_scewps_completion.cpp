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
draw_completion_dropdown(Application_Links* app, Face_ID face, Fancy_Block* block,
    Vec2_f32 p, Rect_f32 region, Vec2_f32 padding,
    FColor outline_color, FColor back_color, FColor highlight_color) {
    Rect_f32 box = Rf32(p, p);
    if (block->line_count > 0) {
        Vec2_f32 dims = get_fancy_block_dim(app, face, block);
        dims += padding * 2;
        box = get_contained_box_near_point(region, p, dims);
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
    return(box);
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

// BOTTOM

