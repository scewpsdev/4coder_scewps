/*
4coder_config.cpp - Parsing *.4coder files.
*/

// TOP

function void
copy_color_table(Color_Table src, Color_Table dst) {
    Assert(src.count == dst.count);
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

// BOTTOM

