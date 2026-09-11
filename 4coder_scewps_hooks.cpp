/*
4coder_default_hooks.cpp - Sets up the hooks for the default framework.
*/

// TOP

#define lerp(a, b, dt, rate) ((a) + ((b) - (a)) * (1 - powf(rate, dt)))

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
            cursor_blink_state = 2;
            animate_in_n_milliseconds(app, 0);
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
            continue;
        }

        // NOTE(allen): Run the command and pre/post command stuff
        default_pre_command(app, scope);
        ProfileCloseNow(view_input_profile);
        map_result.command(app);
        ProfileScope(app, "after view input");
        default_post_command(app, scope);
    }
}

function void
sc_tick(Application_Links* app, Frame_Info frame_info) {
    default_tick(app, frame_info);
    
    b32 interpolate_cursor = def_get_config_b32(vars_save_string_lit("interpolate_cursor"));
    if (interpolate_cursor) {
        Vec2_f32 cursor_size = next_cursor_rect.p1 - next_cursor_rect.p0;
        current_cursor_rect.p0 = lerp(current_cursor_rect.p0, next_cursor_rect.p0, frame_info.animation_dt, 1e-14f);
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
}

function void
sc_whole_screen_render_caller(Application_Links *app, Frame_Info frame_info){
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
                         0, V2f32(cos_f32(pi_f32*.333f), sin_f32(pi_f32*.333f)));
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

BUFFER_HOOK_SIG(sc_begin_buffer){
    ProfileScope(app, "begin buffer");
    
    Scratch_Block scratch(app);
    
    b32 treat_as_code = false;
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
    if (file_name.size > 0){
        String_Const_u8 treat_as_code_string = def_get_config_string(scratch, vars_save_string_lit("treat_as_code"));
        String_Const_u8_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
        String_Const_u8 ext = string_file_extension(file_name);
        for (i32 i = 0; i < extensions.count; ++i){
            if (string_match(ext, extensions.strings[i])){
                treat_as_code = true;

                #if 0
                if (string_match(ext, string_u8_litexpr("cs"))){
                    if (parse_context_language_cs == 0){
                        init_language_cs(app);
                    }
                    parse_context_id = parse_context_language_cs;
                }
                
                if (string_match(ext, string_u8_litexpr("java"))){
                    if (parse_context_language_java == 0){
                        init_language_java(app);
                    }
                    parse_context_id = parse_context_language_java;
                }
                
                if (string_match(ext, string_u8_litexpr("rs"))){
                    if (parse_context_language_rust == 0){
                        init_language_rust(app);
                    }
                    parse_context_id = parse_context_language_rust;
                }
                
                if (string_match(ext, string_u8_litexpr("cpp")) ||
                    string_match(ext, string_u8_litexpr("h")) ||
                    string_match(ext, string_u8_litexpr("c")) ||
                    string_match(ext, string_u8_litexpr("hpp")) ||
                    string_match(ext, string_u8_litexpr("cc"))){
                    if (parse_context_language_cpp == 0){
                        init_language_cpp(app);
                    }
                    parse_context_id = parse_context_language_cpp;
                }
                
                // TODO(NAME): Real GLSL highlighting
                if (string_match(ext, string_u8_litexpr("glsl"))){
                    if (parse_context_language_cpp == 0){
                        init_language_cpp(app);
                    }
                    parse_context_id = parse_context_language_cpp;
                }
                
                // TODO(NAME): Real Objective-C highlighting
                if (string_match(ext, string_u8_litexpr("m"))){
                    if (parse_context_language_cpp == 0){
                        init_language_cpp(app);
                    }
                    parse_context_id = parse_context_language_cpp;
                }
                #endif

                break;
            }
        }
    }
    
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
    
    Command_Map_ID map_id = (treat_as_code)?(code_map_id):(file_map_id);
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr = map_id;
    
    Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    *eol_setting = setting;
    
    // NOTE(allen): Decide buffer settings
    b32 wrap_lines = true;
    b32 use_lexer = false;
    if (treat_as_code){
        wrap_lines = def_get_config_b32(vars_save_string_lit("enable_code_wrapping"));
        use_lexer = true;
    }
    
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
    if (buffer_name.size > 0 && buffer_name.str[0] == '*' && buffer_name.str[buffer_name.size - 1] == '*'){
        wrap_lines = def_get_config_b32(vars_save_string_lit("enable_output_wrapping"));
    }
    
    if (use_lexer){
        ProfileBlock(app, "begin buffer kick off lexer");
        Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
        *lex_task_ptr = async_task_no_dep(&global_async_system, do_full_lex_async, make_data_struct(&buffer_id));
    }
    
    {
        b32 *wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
        *wrap_lines_ptr = wrap_lines;
    }
    
    if (use_lexer){
        buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
    }
    else{
        if (treat_as_code){
            buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
        }
        else{
            buffer_set_layout(app, buffer_id, layout_generic);
        }
    }
    
    // no meaning for return
    return(0);
}

BUFFER_HOOK_SIG(sc_file_save){
    // buffer_id
    ProfileScope(app, "default file save");
    
    b32 auto_indent = def_get_config_b32(vars_save_string_lit("automatically_indent_text_on_save"));
    b32 is_virtual = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
    if (auto_indent && is_virtual){
        auto_indent_buffer(app, buffer_id, buffer_range(app, buffer_id));
    }
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Line_Ending_Kind *eol = scope_attachment(app, scope, buffer_eol_setting,
                                             Line_Ending_Kind);
    switch (*eol){
        case LineEndingKind_LF:
        {
            rewrite_lines_to_lf(app, buffer_id);
        }break;
        case LineEndingKind_CRLF:
        {
            rewrite_lines_to_crlf(app, buffer_id);
        }break;
    }
    
    // no meaning for return
    return(0);
}

// BOTTOM

