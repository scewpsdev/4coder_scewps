/*
4coder_config.cpp - Parsing *.4coder files.
*/

// TOP

function void
sc_setup_essential_mapping(Mapping* mapping, i64 global_id, i64 file_id, i64 code_id) {
    MappingScope();
    SelectMapping(mapping);

    SelectMap(global_id);
    BindCore(sc_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);

    SelectMap(file_id);
    ParentMap(global_id);
    BindTextInput(write_text_input);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);

    SelectMap(code_id);
    ParentMap(file_id);
    BindTextInput(write_text_and_auto_indent);
}

////////////////////////////////
// NOTE(allen): Config Variables Read

function b32
def_get_config_b32(String_ID key, b32 default_value) {
    Variable_Handle var = def_get_config_var(key);
    if (vars_is_nil(var)) {
        return default_value;
    } else {
        String_ID val = vars_string_id_from_var(var);
        b32 result = (val != 0 && val != vars_save_string_lit("false"));
        return(result);
    }
}

function u64
def_get_config_u64(Application_Links *app, String_ID key, u64 default_value) {
    Scratch_Block scratch(app);
    Variable_Handle var = def_get_config_var(key);
    u64 result = vars_is_nil(var) ? default_value : vars_u64_from_var(app, var);
    return(result);
}

function Token_Array
sc_token_array_from_text(Application_Links* app, Arena* arena, Buffer_ID src_buffer, String_Const_u8 data) {
    ProfileScope(app, "token array from text");

    F4_Language* language = F4_LanguageFromBuffer(app, src_buffer);
    if (!language) {
        language = F4_LanguageFromString(S8Lit("cpp"));
    }

    Token_List list = {};
    void* lexing_state = push_array_zero(arena, u8, language->lex_state_size);
    language->LexInit(lexing_state, data);

    i32 limit_factor = 10000;
    language->LexFullInput(arena, &list, lexing_state, limit_factor);

    //Token_List list = lex_full_input_cpp(arena, data);
    return(token_array_from_list(arena, &list));
}

// BOTTOM

