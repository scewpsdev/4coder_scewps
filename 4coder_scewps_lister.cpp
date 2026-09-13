/*
 * Lister base
 */

 // TOP

function void
draw_rectangle_and_margin(Application_Links* app, Rect_f32 rect, f32 roundness, ARGB_Color color, ARGB_Color margin_color, f32 margin) {
	draw_rectangle(app, rect, roundness + margin, margin_color);
	draw_rectangle(app, rect_inner(rect, margin), roundness, color);
}

function void
sc_lister_render(Application_Links* app, Frame_Info frame_info, View_ID view) {
	Render_Caller_Function* custom_render = (Render_Caller_Function*)get_custom_hook(app, HookID_RenderCaller);
	custom_render(app, frame_info, view);

	Scratch_Block scratch(app);

	Lister* lister = view_get_lister(app, view);
	if (lister == 0) {
		return;
	}

	b32 is_active_view = view == get_active_view(app, Access_Always);

	Face_ID face_id = get_face_id(app, 0);
	Face_Metrics metrics = get_face_metrics(app, face_id);
	f32 line_height = metrics.line_height;

	f32 item_height = (f32)def_get_config_u64(app, vars_save_string_lit("lister_item_height"), 150);
	f32 block_height = line_height * (item_height / 100.0f);
	f32 text_field_height = lister_get_text_field_height(line_height);

	f32 margin_width = (f32)def_get_config_u64(app, vars_save_string_lit("margin_width"), 3);

	Rect_f32 region = view_get_screen_rect(app, view);
	region = rect_inner(region, margin_width);
	region = layout_file_bar_on_bot(region, line_height).min;

	f32 lister_width = (f32)def_get_config_u64(app, vars_save_string_lit("lister_width"), 80);
	lister_width = lister_width / 100.0f * rect_width(region);
	lister_width = Min(Max(lister_width, line_height * 40), rect_width(region) - margin_width * 2);

	f32 lister_height = (f32)def_get_config_u64(app, vars_save_string_lit("lister_height"), 90);
	lister_height = lister_height / 100.0f * rect_height(region);
	lister_height = Min(lister_height, rect_height(region) - margin_width * 2);
	
	f32 margin_x = (region.x1 - region.x0 - lister_width) * 0.5f;
	f32 margin_y = (region.y1 - region.y0 - lister_height) * 0.5f;
	region.x0 += margin_x;
	region.x1 -= margin_x;
	region.y0 += margin_y;
	region.y1 -= margin_y;

	f32 roundness = (f32)def_get_config_u64(app, vars_save_string_lit("lister_panel_roundness"), 8);

	ARGB_Color margin_color = fcolor_resolve(get_panel_margin_color(is_active_view ? UIHighlight_Active : UIHighlight_None));
	ARGB_Color back_color = fcolor_resolve(fcolor_id(defcolor_list_item, 0));

	draw_rectangle_and_margin(app, region, roundness, back_color, margin_color, margin_width);

	region = rect_inner(region, margin_width);

	Rect_f32 prev_clip = draw_set_clip(app, region);

	Mouse_State mouse = get_mouse_state(app);
	Vec2_f32 m_p = V2f32(mouse.p);

	lister->visible_count = (i32)((rect_height(region) / block_height)) - 3;
	lister->visible_count = clamp_bot(1, lister->visible_count);

	Rect_f32 text_field_rect = {};
	Rect_f32 list_rect = {};
	{
		Rect_f32_Pair pair = lister_get_top_level_layout(region, text_field_height);
		text_field_rect = pair.min;
		list_rect = pair.max;

		list_rect = rect_inner(list_rect, margin_width);
	}

	{
		Vec2_f32 p = V2f32(text_field_rect.x0 + 3.f, text_field_rect.y0);
		Fancy_Line text_field = {};
		push_fancy_string(scratch, &text_field, fcolor_id(defcolor_pop1),
			lister->query.string);
		push_fancy_stringf(scratch, &text_field, " ");
		p = draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);

		// TODO(allen): This is a bit of a hack. Maybe an upgrade to fancy to focus
		// more on being good at this and less on overriding everything 10 ways to sunday
		// would be good.
		block_zero_struct(&text_field);
		push_fancy_string(scratch, &text_field, fcolor_id(defcolor_text_default),
			lister->text_field.string);
		f32 width = get_fancy_line_width(app, face_id, &text_field);
		f32 cap_width = text_field_rect.x1 - p.x - 6.f;
		if (cap_width < width) {
			Rect_f32 prect = draw_set_clip(app, Rf32(p.x, text_field_rect.y0, p.x + cap_width, text_field_rect.y1));
			p.x += cap_width - width;
			draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
			draw_set_clip(app, prect);
		}
		else {
			draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
		}
	}


	Range_f32 x = rect_range_x(list_rect);
	draw_set_clip(app, list_rect);

	// NOTE(allen): auto scroll to the item if the flag is set.
	f32 scroll_y = lister->scroll.position.y;

	if (lister->set_vertical_focus_to_item) {
		lister->set_vertical_focus_to_item = false;
		Range_f32 item_y = If32_size(lister->item_index * block_height, block_height);
		f32 view_h = rect_height(list_rect);
		Range_f32 view_y = If32_size(scroll_y, view_h);
		if (view_y.min > item_y.min || item_y.max > view_y.max) {
			f32 item_center = (item_y.min + item_y.max) * 0.5f;
			f32 view_center = (view_y.min + view_y.max) * 0.5f;
			f32 margin = view_h * .3f;
			margin = clamp_top(margin, block_height * 3.f);
			if (item_center < view_center) {
				lister->scroll.target.y = item_y.min /*- margin*/;
			}
			else {
				f32 target_bot = item_y.max /*+ margin*/;
				lister->scroll.target.y = target_bot - view_h;
			}
		}
	}

	// NOTE(allen): clamp scroll target and position; smooth scroll rule
	i32 count = lister->filtered.count;
	Range_f32 scroll_range = If32(0.f, clamp_bot(0.f, count * block_height - block_height));
	lister->scroll.target.y = clamp_range(scroll_range, lister->scroll.target.y);
	lister->scroll.target.x = 0.f;

	Vec2_f32_Delta_Result delta = delta_apply(app, view,
		frame_info.animation_dt, lister->scroll);
	lister->scroll.position = delta.p;
	if (delta.still_animating) {
		animate_in_n_milliseconds(app, 0);
	}

	lister->scroll.position.y = clamp_range(scroll_range, lister->scroll.position.y);
	lister->scroll.position.x = 0.f;
	lister->hovered_index = -1;

	scroll_y = lister->scroll.position.y;
	f32 y_pos = list_rect.y0 - scroll_y;

	i32 first_index = (i32)(scroll_y / block_height);
	y_pos += first_index * block_height;

	for (i32 i = first_index; i < count; i += 1) {
		Lister_Node* node = lister->filtered.node_ptrs[i];

		Range_f32 y = If32(y_pos, y_pos + block_height);
		y_pos = y.max;

		Rect_f32 item_rect = Rf32(x, y);
		if (item_rect.y0 > region.y1) { break; }
		Rect_f32 item_inner = rect_inner(item_rect, margin_width);

		b32 hovered = rect_contains_point(item_rect, m_p);
		if (hovered) lister->hovered_index = node->raw_index;

		UI_Highlight_Level highlight = UIHighlight_None;
		if (node == lister->highlighted_node) {
			highlight = UIHighlight_Active;
		}
		else if (node->user_data == lister->hot_user_data) {
			if (hovered) {
				highlight = UIHighlight_Active;
			}
			else {
				highlight = UIHighlight_Hover;
			}
		}
		else if (hovered) {
			highlight = UIHighlight_Hover;
		}

		f32 item_roundness = (f32)def_get_config_u64(app, vars_save_string_lit("lister_item_roundness"), 8);
		draw_rectangle_fcolor(app, item_rect, item_roundness, get_item_margin_color(highlight));
		draw_rectangle_fcolor(app, item_inner, item_roundness - margin_width, get_item_margin_color(highlight, 1));

		Fancy_Line line = {};
		push_fancy_string(scratch, &line, fcolor_id(defcolor_text_default), node->string);
		push_fancy_stringf(scratch, &line, " ");
		push_fancy_string(scratch, &line, fcolor_id(defcolor_pop2), node->status);

		Vec2_f32 p = item_inner.p0 + V2f32(3.f, (block_height - line_height) * 0.5f);
		draw_fancy_line(app, face_id, fcolor_zero(), &line, p);
	}

	draw_set_clip(app, prev_clip);
}

function Lister_Result
run_lister(Application_Links* app, Lister* lister) {
	lister->filter_restore_point = begin_temp(lister->arena);
	lister_update_filtered_list(app, lister);

	View_ID view = get_this_ctx_view(app, Access_Always);
	View_Context ctx = view_current_context(app, view);
	ctx.render_caller = sc_lister_render;
	ctx.hides_buffer = true;
	View_Context_Block ctx_block(app, view, &ctx);

	lister_open = true;
	lister_view = view;

	for (;;) {
		User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
		if (in.abort) {
			block_zero_struct(&lister->out);
			lister->out.canceled = true;
			break;
		}

		Lister_Activation_Code result = ListerActivation_Continue;
		b32 handled = true;
		switch (in.event.kind) {
		case InputEventKind_TextInsert:
		{
			if (lister->handlers.write_character != 0) {
				result = lister->handlers.write_character(app);
			}
		}break;

		case InputEventKind_KeyStroke:
		{
			switch (in.event.key.code) {
			case KeyCode_Return:
			case KeyCode_Tab:
			{
				void* user_data = 0;
				if (0 <= lister->raw_item_index &&
					lister->raw_item_index < lister->options.count) {
					user_data = lister_get_user_data(lister, lister->raw_item_index);
				}
				lister_activate(app, lister, user_data, false);
				result = ListerActivation_Finished;
			}break;

			case KeyCode_Backspace:
			{
				if (lister->handlers.backspace != 0) {
					lister->handlers.backspace(app);
				}
				else if (lister->handlers.key_stroke != 0) {
					result = lister->handlers.key_stroke(app);
				}
				else {
					handled = false;
				}
			}break;

			case KeyCode_Up:
			{
				if (lister->handlers.navigate != 0) {
					lister->handlers.navigate(app, view, lister, -1);
				}
				else if (lister->handlers.key_stroke != 0) {
					result = lister->handlers.key_stroke(app);
				}
				else {
					handled = false;
				}
			}break;

			case KeyCode_Down:
			{
				if (lister->handlers.navigate != 0) {
					lister->handlers.navigate(app, view, lister, 1);
				}
				else if (lister->handlers.key_stroke != 0) {
					result = lister->handlers.key_stroke(app);
				}
				else {
					handled = false;
				}
			}break;

			case KeyCode_PageUp:
			{
				if (lister->handlers.navigate != 0) {
					lister->handlers.navigate(app, view, lister,
						-lister->visible_count);
				}
				else if (lister->handlers.key_stroke != 0) {
					result = lister->handlers.key_stroke(app);
				}
				else {
					handled = false;
				}
			}break;

			case KeyCode_PageDown:
			{
				if (lister->handlers.navigate != 0) {
					lister->handlers.navigate(app, view, lister,
						lister->visible_count);
				}
				else if (lister->handlers.key_stroke != 0) {
					result = lister->handlers.key_stroke(app);
				}
				else {
					handled = false;
				}
			}break;

			default:
			{
				if (lister->handlers.key_stroke != 0) {
					result = lister->handlers.key_stroke(app);
				}
				else {
					handled = false;
				}
			}break;
			}
		}break;

		case InputEventKind_MouseButton:
		{
			switch (in.event.mouse.code) {
			case MouseCode_Left:
			{
				if (0 <= lister->hovered_index &&
					lister->hovered_index < lister->options.count) {
					lister->hot_user_data = lister_get_user_data(lister, lister->hovered_index);
				}
			}break;

			default:
			{
				handled = false;
			}break;
			}
		}break;

		case InputEventKind_MouseButtonRelease:
		{
			switch (in.event.mouse.code) {
			case MouseCode_Left:
			{
				if (lister->hot_user_data != 0) {
					if (0 <= lister->hovered_index &&
						lister->hovered_index < lister->options.count) {
						void* clicked = lister_get_user_data(lister, lister->hovered_index);
						if (lister->hot_user_data == clicked) {
							lister_activate(app, lister, clicked, true);
							result = ListerActivation_Finished;
						}
					}
				}
				lister->hot_user_data = 0;
			}break;

			default:
			{
				handled = false;
			}break;
			}
		}break;

		case InputEventKind_MouseWheel:
		{
			Mouse_State mouse = get_mouse_state(app);
			lister->scroll.target.y += mouse.wheel.y;
			lister_update_filtered_list(app, lister);
		}break;

		case InputEventKind_MouseMove:
		{
			lister_update_filtered_list(app, lister);
		}break;

		case InputEventKind_Core:
		{
			switch (in.event.core.code) {
			case CoreCode_Animate:
			{
				lister_update_filtered_list(app, lister);
			}break;

			default:
			{
				handled = false;
			}break;
			}
		}break;

		default:
		{
			handled = false;
		}break;
		}

		if (result == ListerActivation_Finished) {
			break;
		}

		if (!handled) {
			Mapping* mapping = lister->mapping;
			Command_Map* map = lister->map;

			Fallback_Dispatch_Result disp_result =
				fallback_command_dispatch(app, mapping, map, &in);
			if (disp_result.code == FallbackDispatch_DelayedUICall) {
				call_after_ctx_shutdown(app, view, disp_result.func);
				break;
			}
			if (disp_result.code == FallbackDispatch_Unhandled) {
				leave_current_input_unhandled(app);
			}
			else {
				lister_call_refresh_handler(app, lister);
			}
		}
	}

	lister_open = false;
	lister_view = 0;

	return(lister->out);
}

function Lister_Choice*
get_choice_from_user(Application_Links* app, String_Const_u8 query,
	Lister_Choice_List list) {
	Scratch_Block scratch(app);
	Lister_Block lister(app, scratch);
	for (Lister_Choice* choice = list.first;
		choice != 0;
		choice = choice->next) {
		u64 code_size = sizeof(choice->key_code);
		void* extra = lister_add_item(lister, choice->string, choice->status,
			choice, code_size);
		block_copy(extra, &choice->key_code, code_size);
	}
	lister_set_query(lister, query);
	Lister_Handlers handlers = {};
	handlers.navigate = lister__navigate__default;
	handlers.key_stroke = lister__key_stroke__choice_list;
	lister_set_handlers(lister, &handlers);

	Lister_Result l_result = run_lister(app, lister);
	Lister_Choice* result = 0;
	if (!l_result.canceled) {
		result = (Lister_Choice*)l_result.user_data;
	}
	return(result);
}

function Lister_Choice*
get_choice_from_user(Application_Links* app, char* query, Lister_Choice_List list) {
	return(get_choice_from_user(app, SCu8(query), list));
}

function Lister_Result
run_lister_with_refresh_handler(Application_Links* app, Arena* arena, String_Const_u8 query, Lister_Handlers handlers) {
	Lister_Result result = {};
	if (handlers.refresh != 0) {
		Lister_Block lister(app, arena);
		lister_set_query(lister, query);
		lister_set_handlers(lister, &handlers);
		handlers.refresh(app, lister);
		result = run_lister(app, lister);
	}
	else {
#define M "ERROR: No refresh handler specified for lister (query_string = \"%.*s\")\n"
		String_Const_u8 str = push_u8_stringf(arena, M, string_expand(query));
#undef M
		print_message(app, str);
		result.canceled = true;
	}
	return(result);
}

function Lister_Result
run_lister_with_refresh_handler(Application_Links* app, String_Const_u8 query, Lister_Handlers handlers) {
	Scratch_Block scratch(app);
	return(run_lister_with_refresh_handler(app, scratch, query, handlers));
}

function Lister_Result
run_lister_with_refresh_handler(Application_Links* app, Arena* arena, char* query, Lister_Handlers handlers) {
	return(run_lister_with_refresh_handler(app, arena, SCu8(query), handlers));
}

function Lister_Result
run_lister_with_refresh_handler(Application_Links* app, char* query, Lister_Handlers handlers) {
	return(run_lister_with_refresh_handler(app, SCu8(query), handlers));
}

// BOTTOM

