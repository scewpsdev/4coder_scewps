/*
 * Miscellaneous helpers for common operations.
 */

 // TOP

CUSTOM_COMMAND_SIG(sc_seek_beginning_of_line)
CUSTOM_DOC("Seeks the cursor to the beginning of the visual line.")
{
    seek_pos_of_visual_line(app, Side_Min);
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.pixel_shift.x = 0.0f;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

// BOTTOM

