#include "live_mode.h"

#include <string.h>

// this
#include "flash.h"
#include "fudge.h"
#include "gitversion.h"
#include "keyboard_helper.h"
#include "line_editor.h"

// libavr32
#include "font.h"
#include "region.h"
#include "util.h"

// asf
#include "conf_usb_host.h"  // needed in order to include "usb_protocol_hid.h"
#include "usb_protocol_hid.h"

#define HISTORY_SIZE 6
tele_command_t history[HISTORY_SIZE];
uint8_t history_line;
line_editor_t le;
process_result_t output;
error_t status;
char error_msg[TELE_ERROR_MSG_LENGTH];
uint8_t activity_prev;

void init_live_mode() {
    status = E_WELCOME;
    strcpy(error_msg, git_version);
    activity_prev = 0xff;
}

void set_live_mode() {
    line_editor_set(&le, "");
    history_line = HISTORY_SIZE;
    activity |= A_REFRESH;
}

void process_live_keys(uint8_t k, uint8_t m, bool is_held_key) {
    if (match_no_mod(m, k, HID_DOWN)) {  // down
        if (history_line < (HISTORY_SIZE - 1)) {
            history_line++;
            line_editor_set_command(&le, &history[history_line]);
            r_edit_dirty |= R_INPUT;
        }
        else {
            history_line = HISTORY_SIZE;
            line_editor_set(&le, "");
            r_edit_dirty |= R_INPUT;
        }
    }
    else if (match_no_mod(m, k, HID_UP)) {  // up
        if (history_line) {
            history_line--;
            line_editor_set_command(&le, &history[history_line]);
            r_edit_dirty |= R_INPUT;
        }
    }
    else if (match_no_mod(m, k, HID_ENTER)) {  // enter
        r_edit_dirty |= R_MESSAGE;  // something will definitely happen
        r_edit_dirty |= R_INPUT;

        tele_command_t command;

        status = parse(line_editor_get(&le), &command, error_msg);
        if (status != E_OK)
            return;  // quit, screen_refresh_live will display the error message

        status = validate(&command, error_msg);
        if (status != E_OK)
            return;  // quit, screen_refresh_live will display the error message

        history_line = HISTORY_SIZE;
        if (command.length) {
            // shuffle the history up
            // should really use some sort of ring buffer
            for (size_t i = 0; i < HISTORY_SIZE - 1; i++) {
                memcpy(&history[i], &history[i + 1], sizeof(command));
            }
            memcpy(&history[HISTORY_SIZE - 1], &command, sizeof(command));

            output = run_command(&scene_state, &command);
        }
        line_editor_set(&le, "");
    }
    else {  // pass the key though to the line editor
        bool processed = line_editor_process_keys(&le, k, m, is_held_key);
        if (processed) r_edit_dirty |= R_INPUT;
    }
}


void screen_refresh_live() {
    if (r_edit_dirty & R_INPUT) {
        line_editor_draw(&le, '>', &line[7]);
        screen_dirty = true;
        r_edit_dirty &= ~R_INPUT;
    }

    if (r_edit_dirty & R_MESSAGE) {
        char s[32];
        if (status != E_OK) {
            strcpy(s, tele_error(status));
            if (error_msg[0]) {
                strcat(s, ": ");
                strcat(s, error_msg);
                error_msg[0] = 0;
            }
            status = E_OK;
        }

        else if (output.has_value) {
            itoa(output.value, s, 10);
            output.has_value = false;
        }
        else {
            s[0] = 0;
        }

        region_fill(&line[6], 0);
        font_string_region_clip(&line[6], s, 0, 0, 0x4, 0);

        screen_dirty = true;
        r_edit_dirty &= ~R_MESSAGE;
    }

    if (r_edit_dirty & R_LIST) {
        for (int i = 0; i < 6; i++) region_fill(&line[i], 0);

        screen_dirty = true;
        r_edit_dirty &= ~R_LIST;
    }

    if ((activity != activity_prev)) {
        region_fill(&line[0], 0);

        // slew icon
        uint8_t slew_fg = activity & A_SLEW ? 15 : 1;
        line[0].data[98 + 0 + 512] = slew_fg;
        line[0].data[98 + 1 + 384] = slew_fg;
        line[0].data[98 + 2 + 256] = slew_fg;
        line[0].data[98 + 3 + 128] = slew_fg;
        line[0].data[98 + 4 + 0] = slew_fg;

        // delay icon
        uint8_t delay_fg = activity & A_DELAY ? 15 : 1;
        line[0].data[106 + 0 + 0] = delay_fg;
        line[0].data[106 + 1 + 0] = delay_fg;
        line[0].data[106 + 2 + 0] = delay_fg;
        line[0].data[106 + 3 + 0] = delay_fg;
        line[0].data[106 + 4 + 0] = delay_fg;
        line[0].data[106 + 0 + 128] = delay_fg;
        line[0].data[106 + 0 + 256] = delay_fg;
        line[0].data[106 + 0 + 384] = delay_fg;
        line[0].data[106 + 0 + 512] = delay_fg;
        line[0].data[106 + 4 + 128] = delay_fg;
        line[0].data[106 + 4 + 256] = delay_fg;
        line[0].data[106 + 4 + 384] = delay_fg;
        line[0].data[106 + 4 + 512] = delay_fg;

        // queue icon
        uint8_t queue_fg = activity & A_Q ? 15 : 1;
        line[0].data[114 + 0 + 0] = queue_fg;
        line[0].data[114 + 1 + 0] = queue_fg;
        line[0].data[114 + 2 + 0] = queue_fg;
        line[0].data[114 + 3 + 0] = queue_fg;
        line[0].data[114 + 4 + 0] = queue_fg;
        line[0].data[114 + 0 + 256] = queue_fg;
        line[0].data[114 + 1 + 256] = queue_fg;
        line[0].data[114 + 2 + 256] = queue_fg;
        line[0].data[114 + 3 + 256] = queue_fg;
        line[0].data[114 + 4 + 256] = queue_fg;
        line[0].data[114 + 0 + 512] = queue_fg;
        line[0].data[114 + 1 + 512] = queue_fg;
        line[0].data[114 + 2 + 512] = queue_fg;
        line[0].data[114 + 3 + 512] = queue_fg;
        line[0].data[114 + 4 + 512] = queue_fg;

        // metro icon
        uint8_t metro_fg = activity & A_METRO ? 15 : 1;
        line[0].data[122 + 0 + 0] = metro_fg;
        line[0].data[122 + 0 + 128] = metro_fg;
        line[0].data[122 + 0 + 256] = metro_fg;
        line[0].data[122 + 0 + 384] = metro_fg;
        line[0].data[122 + 0 + 512] = metro_fg;
        line[0].data[122 + 1 + 128] = metro_fg;
        line[0].data[122 + 2 + 256] = metro_fg;
        line[0].data[122 + 3 + 128] = metro_fg;
        line[0].data[122 + 4 + 0] = metro_fg;
        line[0].data[122 + 4 + 128] = metro_fg;
        line[0].data[122 + 4 + 256] = metro_fg;
        line[0].data[122 + 4 + 384] = metro_fg;
        line[0].data[122 + 4 + 512] = metro_fg;

        // mutes
        line[0].data[87 + 0 + 128] = 15 - mutes[0] * 12;
        line[0].data[87 + 1 + 384] = 15 - mutes[1] * 12;
        line[0].data[87 + 2 + 128] = 15 - mutes[2] * 12;
        line[0].data[87 + 3 + 384] = 15 - mutes[3] * 12;
        line[0].data[87 + 4 + 128] = 15 - mutes[4] * 12;
        line[0].data[87 + 5 + 384] = 15 - mutes[5] * 12;
        line[0].data[87 + 6 + 128] = 15 - mutes[6] * 12;
        line[0].data[87 + 7 + 384] = 15 - mutes[7] * 12;

        activity_prev = activity;
        screen_dirty = true;
        activity &= ~A_MUTES;
        activity &= ~A_REFRESH;
    }
}