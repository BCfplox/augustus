#include "top_menu.h"

#include "assets/assets.h"
#include "building/construction.h"
#include "city/constants.h"
#include "city/finance.h"
#include "city/population.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/lang.h"
#include "game/campaign.h"
#include "game/file.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/system.h"
#include "game/time.h"
#include "game/undo.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/menu.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/property.h"
#include "widget/city.h"
#include "window/advisors.h"
#include "window/city.h"
#include "window/config.h"
#include "window/file_dialog.h"
#include "window/hotkey_config.h"
#include "window/main_menu.h"
#include "window/message_dialog.h"
#include "window/mission_selection.h"
#include "window/plain_message_dialog.h"
#include "window/popup_dialog.h"

enum {
    INFO_NONE = 0,
    INFO_FUNDS = 1,
    INFO_POPULATION = 2,
    INFO_DATE = 3
};

#define BLACK_PANEL_BLOCK_WIDTH 20
#define BLACK_PANEL_MIDDLE_BLOCKS 4
#define BLACK_PANEL_TOTAL_BLOCKS 6

static void menu_file_replay_map(int param);
static void menu_file_load_game(int param);
static void menu_file_save_game(int param);
static void menu_file_delete_game(int param);
static void menu_file_exit_to_main_menu(int param);
static void menu_file_exit_game(int param);

static void menu_options_general(int param);
static void menu_options_user_interface(int param);
static void menu_options_gameplay(int param);
static void menu_options_city_management(int param);
static void menu_options_hotkeys(int param);
static void menu_options_monthly_autosave(int param);
static void menu_options_yearly_autosave(int param);

static void menu_help_help(int param);
static void menu_help_mouse_help(int param);
static void menu_help_warnings(int param);
static void menu_help_about(int param);

static void menu_advisors_go_to(int advisor);

static menu_item menu_file[] = {
    {1, 2, menu_file_replay_map, 0},
    {1, 3, menu_file_load_game, 0},
    {1, 4, menu_file_save_game, 0},
    {1, 6, menu_file_delete_game, 0},
    {CUSTOM_TRANSLATION, TR_BUTTON_BACK_TO_MAIN_MENU, menu_file_exit_to_main_menu, 0},
    {1, 5, menu_file_exit_game, 0},
};

static menu_item menu_options[] = {
    {CUSTOM_TRANSLATION, TR_CONFIG_HEADER_GENERAL, menu_options_general, 0},
    {CUSTOM_TRANSLATION, TR_CONFIG_HEADER_UI_CHANGES, menu_options_user_interface, 0},
    {CUSTOM_TRANSLATION, TR_CONFIG_HEADER_GAMEPLAY_CHANGES, menu_options_gameplay, 0},
    {CUSTOM_TRANSLATION, TR_CONFIG_HEADER_CITY_MANAGEMENT_CHANGES, menu_options_city_management, 0},
    {CUSTOM_TRANSLATION, TR_BUTTON_CONFIGURE_HOTKEYS, menu_options_hotkeys, 0},
    {19, 51, menu_options_monthly_autosave, 0},
    {CUSTOM_TRANSLATION, TR_BUTTON_YEARLY_AUTOSAVE_OFF, menu_options_yearly_autosave, 0},
};

static menu_item menu_help[] = {
    {3, 1, menu_help_help, 0},
    {3, 2, menu_help_mouse_help, 0},
    {3, 5, menu_help_warnings, 0},
    {3, 7, menu_help_about, 0},
};

static menu_item menu_advisors[] = {
    {4, 1, menu_advisors_go_to, ADVISOR_LABOR},
    {4, 2, menu_advisors_go_to, ADVISOR_MILITARY},
    {4, 3, menu_advisors_go_to, ADVISOR_IMPERIAL},
    {4, 4, menu_advisors_go_to, ADVISOR_RATINGS},
    {4, 5, menu_advisors_go_to, ADVISOR_TRADE},
    {4, 6, menu_advisors_go_to, ADVISOR_POPULATION},
    {CUSTOM_TRANSLATION, TR_HEADER_HOUSING, menu_advisors_go_to, ADVISOR_HOUSING},
    {4, 7, menu_advisors_go_to, ADVISOR_HEALTH},
    {4, 8, menu_advisors_go_to, ADVISOR_EDUCATION},
    {4, 9, menu_advisors_go_to, ADVISOR_ENTERTAINMENT},
    {4, 10, menu_advisors_go_to, ADVISOR_RELIGION},
    {4, 11, menu_advisors_go_to, ADVISOR_FINANCIAL},
    {4, 12, menu_advisors_go_to, ADVISOR_CHIEF},
};

static menu_bar_item menu[] = {
    {1, menu_file, 6},
    {2, menu_options, 7},
    {3, menu_help, 4},
    {4, menu_advisors, 13},
};

static const int INDEX_OPTIONS = 1;
static const int INDEX_HELP = 2;

static struct {
    int offset_funds;
    int offset_population;
    int offset_date;

    int open_sub_menu;
    int focus_menu_id;
    int focus_sub_menu_id;
} data;

static struct {
    int population;
    int treasury;
    int month;
} drawn;

static void clear_state(void)
{
    data.open_sub_menu = 0;
    data.focus_menu_id = 0;
    data.focus_sub_menu_id = 0;
}

static void set_text_for_monthly_autosave(void)
{
    menu_update_text(&menu[INDEX_OPTIONS], 5, setting_monthly_autosave() ? 51 : 52);
}

static void set_text_for_yearly_autosave(void)
{
    menu_update_text(&menu[INDEX_OPTIONS], 6, 
        config_get(CONFIG_GP_CH_YEARLY_AUTOSAVE) ? TR_BUTTON_YEARLY_AUTOSAVE_ON : TR_BUTTON_YEARLY_AUTOSAVE_OFF);
}


static void set_text_for_tooltips(void)
{
    int new_text;
    switch (setting_tooltips()) {
        case TOOLTIPS_NONE:
            new_text = 2;
            break;
        case TOOLTIPS_SOME:
            new_text = 3;
            break;
        case TOOLTIPS_FULL:
            new_text = 4;
            break;
        default:
            return;
    }
    menu_update_text(&menu[INDEX_HELP], 1, new_text);
}

static void set_text_for_warnings(void)
{
    menu_update_text(&menu[INDEX_HELP], 2, setting_warnings() ? 6 : 5);
}

static void init(void)
{
    set_text_for_monthly_autosave();
    set_text_for_yearly_autosave();
    set_text_for_tooltips();
    set_text_for_warnings();
}

static void draw_background(void)
{
    window_city_draw_panels();
    window_city_draw();
}

static void draw_foreground(void)
{
    if (data.open_sub_menu) {
        menu_draw(&menu[data.open_sub_menu - 1], data.focus_sub_menu_id);
    }
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    widget_top_menu_handle_input(m, h);
}

static void top_menu_window_show(void)
{
    window_type window = {
        WINDOW_TOP_MENU,
        draw_background,
        draw_foreground,
        handle_input
    };
    init();
    window_show(&window);
}

static void refresh_background(void)
{
    int block_width = 24;
    int image_base = image_group(GROUP_TOP_MENU);
    int s_width = screen_width();
    for (int i = 0; i * block_width < s_width; i++) {
        image_draw(image_base + i % 8, i * block_width, 0, COLOR_MASK_NONE, SCALE_NONE);
    }
}

static int draw_black_panel(int x, int y, int width)
{
    int blocks = (width / BLACK_PANEL_BLOCK_WIDTH) - 1;
    if ((width % BLACK_PANEL_BLOCK_WIDTH) > 0) {
        blocks++;
    }

    image_draw(image_group(GROUP_TOP_MENU) + 14, x, y, COLOR_MASK_NONE, SCALE_NONE);
    if (blocks <= BLACK_PANEL_MIDDLE_BLOCKS) {
        return BLACK_PANEL_TOTAL_BLOCKS * BLACK_PANEL_BLOCK_WIDTH;
    }
    static int black_panel_base_id;
    if (!black_panel_base_id) {
        black_panel_base_id = assets_get_image_id("UI", "Top_UI_Panel");
    }
    int x_offset = BLACK_PANEL_BLOCK_WIDTH * (BLACK_PANEL_MIDDLE_BLOCKS + 1);
    blocks -= BLACK_PANEL_MIDDLE_BLOCKS;

    for (int i = 0; i < blocks; i++) {
        image_draw(black_panel_base_id + (i % BLACK_PANEL_MIDDLE_BLOCKS) + 1, x + x_offset, y,
            COLOR_MASK_NONE, SCALE_NONE);
        x_offset += BLACK_PANEL_BLOCK_WIDTH;
    }

    image_draw(black_panel_base_id + 5, x + x_offset, y, COLOR_MASK_NONE, SCALE_NONE);

    return x_offset + BLACK_PANEL_BLOCK_WIDTH;
}

void widget_top_menu_draw(int force)
{
    if (!force && drawn.treasury == city_finance_treasury() &&
        drawn.population == city_population() &&
        drawn.month == game_time_month()) {
        return;
    }

    int s_width = screen_width();

    refresh_background();
    menu_bar_draw(menu, 4, s_width < 1024 ? 338 : 493);

    color_t treasure_color = COLOR_WHITE;
    int treasury = city_finance_treasury();
    if (treasury < 0) {
        treasure_color = COLOR_FONT_RED;
    }
    int draw_panel_pop_date;
    font_t pop_date_font;
    color_t pop_color;
    color_t date_color;
    if (s_width < 800) {
        data.offset_funds = 330;
        data.offset_population = 450;
        data.offset_date = 540;
        draw_panel_pop_date = 0;
        pop_date_font = FONT_NORMAL_GREEN;
        pop_color = COLOR_MASK_NONE;
        date_color = COLOR_MASK_NONE;
    } else if (s_width < 1024) {
        data.offset_funds = 330;
        data.offset_population = 450;
        data.offset_date = 650;
        draw_panel_pop_date = 1;
        pop_date_font = FONT_NORMAL_PLAIN;
        pop_color = COLOR_WHITE;
        date_color = COLOR_FONT_YELLOW;
    } else {
        data.offset_funds = 490;
        data.offset_population = 635;
        data.offset_date = 850;
        draw_panel_pop_date = 1;
        pop_date_font = FONT_NORMAL_PLAIN;
        pop_color = COLOR_WHITE;
        date_color = COLOR_FONT_YELLOW;
    }

    int text_width = text_get_width(lang_get_string(6, 0), FONT_NORMAL_PLAIN);
    text_width += calc_digits_in_number(treasury) * 11;
    int panel_width = draw_black_panel(data.offset_funds, 0, text_width);
    int x_offset = (panel_width - text_width) / 2;

    int width = lang_text_draw_colored(6, 0, data.offset_funds + x_offset, 5, FONT_NORMAL_PLAIN, treasure_color);
    text_draw_number(treasury, '@', " ", data.offset_funds - 6 + x_offset + width, 5, FONT_NORMAL_PLAIN, treasure_color);

    if (draw_panel_pop_date) {
        text_width = text_get_width(lang_get_string(6, 1), pop_date_font);
        text_width += calc_digits_in_number(city_population()) * 11;
        panel_width = draw_black_panel(data.offset_population, 0, text_width);
        x_offset = (panel_width - text_width) / 2;
    } else {
        x_offset = 0;
    }

    width = lang_text_draw_colored(6, 1, data.offset_population + x_offset, 5, pop_date_font, pop_color);
    text_draw_number(city_population(), '@', " ", data.offset_population - 6 + x_offset + width, 5,
        pop_date_font, pop_color);

    if (draw_panel_pop_date) {
        panel_width = draw_black_panel(data.offset_date, 0, 100);
        x_offset = (panel_width - 100) / 2;
    } else {
        x_offset = 0;
    }

    lang_text_draw_month_year_max_width(game_time_month(), game_time_year(), data.offset_date + x_offset, 5, 100,
        pop_date_font, date_color);

    drawn.treasury = treasury;
    drawn.population = city_population();
    drawn.month = game_time_month();
}

static int handle_input_submenu(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        clear_state();
        window_go_back();
        return 1;
    }
    int menu_id = menu_bar_handle_mouse(m, menu, 4, &data.focus_menu_id);
    if (menu_id && menu_id != data.open_sub_menu) {
        window_request_refresh();
        data.open_sub_menu = menu_id;
    }
    if (!menu_handle_mouse(m, &menu[data.open_sub_menu - 1], &data.focus_sub_menu_id)) {
        if (m->left.went_up) {
            clear_state();
            window_go_back();
            return 1;
        }
    }
    return 0;
}

static int get_info_id(int mouse_x, int mouse_y)
{
    if (mouse_y < 4 || mouse_y >= 18) {
        return INFO_NONE;
    }
    if (mouse_x > data.offset_funds && mouse_x < data.offset_funds + 128) {
        return INFO_FUNDS;
    }
    if (mouse_x > data.offset_population && mouse_x < data.offset_population + 128) {
        return INFO_POPULATION;
    }
    if (mouse_x > data.offset_date && mouse_x < data.offset_date + 128) {
        return INFO_DATE;
    }
    return INFO_NONE;
}

static int handle_right_click(int type)
{
    if (type == INFO_NONE) {
        return 0;
    }
    if (type == INFO_FUNDS) {
        window_message_dialog_show(MESSAGE_DIALOG_TOP_FUNDS, window_city_draw_all);
    } else if (type == INFO_POPULATION) {
        window_message_dialog_show(MESSAGE_DIALOG_TOP_POPULATION, window_city_draw_all);
    } else if (type == INFO_DATE) {
        window_message_dialog_show(MESSAGE_DIALOG_TOP_DATE, window_city_draw_all);
    }
    return 1;
}

static int handle_mouse_menu(const mouse *m)
{
    int menu_id = menu_bar_handle_mouse(m, menu, 4, &data.focus_menu_id);
    if (menu_id && m->left.went_up) {
        data.open_sub_menu = menu_id;
        top_menu_window_show();
        return 1;
    }
    if (m->right.went_up) {
        return handle_right_click(get_info_id(m->x, m->y));
    }
    return 0;
}

int widget_top_menu_handle_input(const mouse *m, const hotkeys *h)
{
    if (widget_city_has_input()) {
        return 0;
    }
    if (data.open_sub_menu) {
        return handle_input_submenu(m, h);
    } else {
        return handle_mouse_menu(m);
    }
}

int widget_top_menu_get_tooltip_text(tooltip_context *c)
{
    if (data.focus_menu_id) {
        return 49 + data.focus_menu_id;
    }
    int button_id = get_info_id(c->mouse_x, c->mouse_y);
    if (button_id) {
        return 59 + button_id;
    }
    return 0;
}

static void replay_map_confirmed(int confirmed, int checked)
{
    if (!confirmed) {
        window_city_show();
        return;
    }
    if (!game_campaign_is_active()) {
        window_city_show();
        if (!game_file_start_scenario_by_name(scenario_name())) {
            window_plain_message_dialog_show_with_extra(TR_REPLAY_MAP_NOT_FOUND_TITLE,
                TR_REPLAY_MAP_NOT_FOUND_MESSAGE, 0, scenario_name());
        }
    } else {
        int mission_id = game_campaign_is_original() ? scenario_campaign_mission() : 0;
        setting_set_personal_savings_for_mission(mission_id, scenario_starting_personal_savings());
        scenario_save_campaign_player_name();
        window_mission_selection_show_again();
    }
}

static void menu_file_replay_map(int param)
{
    clear_state();
    building_construction_clear_type();
    window_popup_dialog_show_confirmation(lang_get_string(1, 2), 0, 0, replay_map_confirmed);
}

static void menu_file_load_game(int param)
{
    clear_state();
    building_construction_clear_type();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
}

static void menu_file_save_game(int param)
{
    clear_state();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_SAVE);
}

static void menu_file_delete_game(int param)
{
    clear_state();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_DELETE);
}

static void menu_file_confirm_exit(int accepted, int checked)
{
    if (accepted) {
        system_exit();
    } else {
        window_city_return();
    }
}

static void main_menu_confirmed(int confirmed, int checked)
{
    if (!confirmed) {
        window_city_show();
        return;
    }
    building_construction_clear_type();
    game_undo_disable();
    game_state_reset_overlay();
    window_main_menu_show(1);
}

static void menu_file_exit_to_main_menu(int param)
{
    clear_state();
    window_popup_dialog_show_confirmation(translation_for(TR_BUTTON_BACK_TO_MAIN_MENU), 0, 0,
        main_menu_confirmed);
}

static void menu_file_exit_game(int param)
{
    clear_state();
    window_popup_dialog_show(POPUP_DIALOG_QUIT, menu_file_confirm_exit, 1);
}

static void menu_options_general(int param)
{
    clear_state();
    window_go_back();
    window_config_show(CONFIG_PAGE_GENERAL, 0);
}

static void menu_options_user_interface(int param)
{
    clear_state();
    window_go_back();
    window_config_show(CONFIG_PAGE_UI_CHANGES, 0);
}

static void menu_options_gameplay(int param)
{
    clear_state();
    window_go_back();
    window_config_show(CONFIG_PAGE_GAMEPLAY_CHANGES, 0);
}

static void menu_options_city_management(int param)
{
    clear_state();
    window_go_back();
    window_config_show(CONFIG_PAGE_CITY_MANAGEMENT_CHANGES, 0);
}

static void menu_options_hotkeys(int param)
{
    clear_state();
    window_go_back();
    window_hotkey_config_show();
}

static void menu_options_monthly_autosave(int param)
{
    setting_toggle_monthly_autosave();
    set_text_for_monthly_autosave();
}

static void menu_options_yearly_autosave(int param)
{
    config_set(CONFIG_GP_CH_YEARLY_AUTOSAVE, !config_get(CONFIG_GP_CH_YEARLY_AUTOSAVE));
    config_save();
    set_text_for_yearly_autosave();
}

static void menu_help_help(int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_HELP, window_city_draw_all);
}

static void menu_help_mouse_help(int param)
{
    setting_cycle_tooltips();
    set_text_for_tooltips();
}

static void menu_help_warnings(int param)
{
    setting_toggle_warnings();
    set_text_for_warnings();
}

static void menu_help_about(int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_ABOUT, window_city_draw_all);
}

static void menu_advisors_go_to(int advisor)
{
    clear_state();
    window_go_back();
    window_advisors_show_advisor(advisor);
}
