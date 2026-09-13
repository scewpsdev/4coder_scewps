/*
4coder_config.cpp - Parsing *.4coder files.
*/

// TOP

function void
copy_color_table(Color_Table src, Color_Table dst) {
	for (i64 i = 0; i < src.count; i++) {
		dst.arrays[i].count = src.arrays[i].count;
		block_copy(dst.arrays[i].vals, src.arrays[i].vals, src.arrays[i].count * sizeof(ARGB_Color));
	}
}

function Color_Table
init_color_table(Application_Links* app) {
	Color_Table color_table = make_color_table(app, &global_theme_arena);
	for (i64 i = 0; i < color_table.count; i++) {
		color_table.arrays[i].vals = push_array(&global_theme_arena, ARGB_Color, 16);
		color_table.arrays[i].count = 16;
	}
	copy_color_table(active_color_table, color_table);
	return color_table;
}

function Color_Table*
get_color_table_from_index(Color_Table_List* list, i32 index) {
    i32 i = 0;
    for (Color_Table_Node* node = global_theme_list.first; node; node = node->next) {
        if (i == index) {
            return &node->table;
        }
        i++;
    }
    return nullptr;
}

function b32
compare_color_tables(Color_Table a, Color_Table b) {
    if (a.count != b.count) {
        return false;
    }
    for (i32 i = 0; i < a.count; i++) {
        if (a.arrays[i].count != b.arrays[i].count) {
            return false;
        }
        for (i32 j = 0; j < a.arrays[i].count; j++) {
            if (a.arrays[i].vals[j] != b.arrays[i].vals[j]) {
                return false;
            }
        }
    }
    return true;
}

function i32
get_theme_index(Color_Table color_table) {
    i32 i = 0;
    for (Color_Table_Node* node = global_theme_list.first; node; node = node->next) {
        // compare themes
        if (compare_color_tables(node->table, color_table)) {
            return i;
        }
        i++;
    }
    return -1;
}

function void
navigate_theme_lister(Application_Links* app, View_ID view, Lister* lister, i32 delta) {
    lister__navigate__default(app, view, lister, delta);

    Color_Table* color_table = get_color_table_from_index(&global_theme_list, lister->item_index);
    if (color_table) {
        next_color_table = *color_table;
    }
}

function Color_Table*
sc_get_color_table_from_user(Application_Links* app, String_Const_u8 query, Color_Table_List* color_table_list) {
    if (color_table_list == 0) {
        color_table_list = &global_theme_list;
    }

    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);

    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.navigate = navigate_theme_lister;
    lister_set_handlers(lister, &handlers);

    i32 prev_theme_index = get_theme_index(next_color_table);
    lister.lister.current->item_index = prev_theme_index;

    /*
    lister_add_item(lister, string_u8_litexpr("4coder"), string_u8_litexpr(""),
        (void*)&default_color_table, 0);
    */

    for (Color_Table_Node* node = color_table_list->first;
        node != 0;
        node = node->next) {
        lister_add_item(lister, node->name, string_u8_litexpr(""),
            (void*)&node->table, 0);
    }

    Lister_Result l_result = run_lister(app, lister);

    if (!l_result.canceled) {
        return (Color_Table*)l_result.user_data;
    } else if (prev_theme_index != -1) {
        return get_color_table_from_index(&global_theme_list, prev_theme_index);
    } else {
        return nullptr;
    }
}

function Color_Table*
sc_get_color_table_from_user(Application_Links* app) {
    return(sc_get_color_table_from_user(app, string_u8_litexpr("Theme:"), 0));
}

// BOTTOM

