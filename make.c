#include <yed/plugin.h>
#include <yed/highlight.h>

#define ARRAY_LOOP(a) for (__typeof((a)[0]) *it = (a); it < (a) + (sizeof(a) / sizeof((a)[0])); ++it)

highlight_info hinfo;

void unload(yed_plugin *self);
void syntax_sh_line_handler(yed_event *event);

int yed_plugin_boot(yed_plugin *self) {
    YED_PLUG_VERSION_CHECK();

    yed_event_handler line;

    yed_plugin_set_unload_fn(self, unload);


    line.kind = EVENT_LINE_PRE_DRAW;
    line.fn   = syntax_sh_line_handler;

    yed_plugin_add_event_handler(self, line);

    highlight_info_make(&hinfo);

    highlight_numbers(&hinfo);

    highlight_within(&hinfo, "$(", ")", 0, -1, HL_CON);
    highlight_to_eol_from(&hinfo, "#", HL_COMMENT);
    highlight_within(&hinfo, "\"", "\"", '\\', -1, HL_STR);

    ys->redraw = 1;

    return 0;
}

void unload(yed_plugin *self) {
    highlight_info_free(&hinfo);
    ys->redraw = 1;
}

void syntax_sh_line_handler(yed_event *event) {
    yed_frame *frame;
    yed_line  *line;
    yed_attrs  attn;
    yed_attrs  fn;
    yed_attrs  fn1;
    yed_glyph  first;
    yed_glyph *git;
    yed_attrs *ait;
    int        col;
    int        i;
    int        colon_col;
    int        save_col;
    int        found;
    int        first_space;
    int        space_col;

    frame = event->frame;

    if (!frame
    ||  !frame->buffer
    ||  frame->buffer->kind != BUFF_KIND_FILE
    ||  frame->buffer->ft != yed_get_ft("Make")) {
        return;
    }

    highlight_line(&hinfo, event);

    line = yed_buff_get_line(event->frame->buffer, event->row);

    if (line->visual_width == 0) { return; }

    attn    = yed_active_style_get_attention();
    attn.bg = attn.fg;
    fn      = yed_active_style_get_code_fn_call();
    fn1      = yed_active_style_get_code_keyword();

    first = *yed_line_col_to_glyph(line, 1);

    if (first.c == '\t' || first.c == ' ') {
        col = 1;
        yed_line_glyph_traverse(*line, git) {
            if (git->c == ' ') {
                ait = array_item(event->line_attrs, col - 1);
                yed_combine_attrs(ait, &attn);
            } else if (git->c != '\t') {
                goto Var;
            }
            col += yed_get_glyph_width(*git);
        }
    } else {
        col       = 1;
        colon_col = 0;
        yed_line_glyph_traverse(*line, git) {
            if (git->c == ' ') { break; }
            if (git->c == ':') {
                colon_col = col;
                break;
            }
            col += yed_get_glyph_width(*git);
        }

        if (colon_col) {
            for (i = 1; i < colon_col; i += 1) {
                ait = array_item(event->line_attrs, i - 1);
                yed_combine_attrs(ait, &fn);
            }
        }else {
            col = 1;
            goto Var;
        }
    }
    goto Done;

Var:;
    save_col = col;
    found = 0;
    first_space = 0;
    space_col = 0;
    yed_line_glyph_traverse(*line, git) {
        if (git->c == '=') {
            found = 1;
            break;
        }else if(git->c == ' ' && first_space == 0) {
            space_col = col;
            first_space = 1;
        }
        col += yed_get_glyph_width(*git);
    }

    if (found) {
        for (i = save_col; i < space_col; i += 1) {
            ait = array_item(event->line_attrs, i - 1);
            yed_combine_attrs(ait, &fn1);
        }
    }

Done:;
}
