#include "health.h"

#include "building/count.h"
#include "city/culture.h"
#include "city/health.h"
#include "city/houses.h"
#include "city/population.h"
#include "core/calc.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"

#define ADVISOR_HEIGHT 26

static int get_health_advice(void)
{
    house_demands *demands = city_houses_demands();
    switch (demands->health) {
        case 1:
            return demands->requiring.bathhouse ? 1 : 0;
        case 2:
            return demands->requiring.barber ? 3 : 2;
        case 3:
            return demands->requiring.clinic ? 5 : 4;
        case 4:
            return 6;
        default:
            return 7;
    }
}

static void print_health_building_info(int y_offset, building_type type, int population_served, int coverage)
{
    static const int BUILDING_ID_TO_STRING_ID[] = { 28, 30, 24, 26 };

    lang_text_draw_amount(8, BUILDING_ID_TO_STRING_ID[type - BUILDING_DOCTOR],
        building_count_total(type), 40, y_offset, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_total(type), 180, y_offset, 100, FONT_NORMAL_WHITE);

    int width = text_draw_number(population_served, '@', " ", 305, y_offset, FONT_NORMAL_WHITE, 0);

    if (type == BUILDING_DOCTOR || type == BUILDING_HOSPITAL) {
        lang_text_draw(56, 6, 305 + width, y_offset, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw(58, 5, 305 + width, y_offset, FONT_NORMAL_WHITE);
    }

    if (coverage == 0) {
        lang_text_draw_centered(57, 10, 440, y_offset, 160, FONT_NORMAL_WHITE);
    } else if (coverage < 100) {
        lang_text_draw_centered(57, coverage / 10 + 11, 440, y_offset, 160, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, 440, y_offset, 160, FONT_NORMAL_WHITE);
    }
}

static int draw_background(void)
{
    outer_panel_draw(0, 0, 40, ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 6, 10, 10, COLOR_MASK_NONE, SCALE_NONE);

    int sickness_level = city_health_get_global_sickness_level();

    lang_text_draw(56, 0, 60, 12, FONT_LARGE_BLACK); // City health

    int x_offset = lang_text_draw(CUSTOM_TRANSLATION, TR_ADVISOR_HEALTH_RATING, 60, 44, FONT_NORMAL_BLACK);
    text_draw_number(city_health(), 0, "", 60 + x_offset, 44, FONT_NORMAL_BLACK, 0);

    if (city_population() >= 200) {
        lang_text_draw_multiline(56, city_health() / 10 + 16, 60, 65, 560, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_multiline(56, 15, 60, 65, 560, FONT_NORMAL_BLACK);
    }
    lang_text_draw_centered(56, 3, 165, 110, 130, FONT_SMALL_PLAIN);    // Working
    lang_text_draw(56, 4, 312, 110, FONT_SMALL_PLAIN);                  // Care for
    lang_text_draw_centered(56, 5, 441, 110, 160, FONT_SMALL_PLAIN);    // City coverage

    inner_panel_draw(32, 124, 36, 5);

    int population = city_population();

    int people_covered = city_health_get_population_with_baths_access();
    print_health_building_info(128, BUILDING_BATHHOUSE, people_covered, calc_percentage(people_covered, population));

    people_covered = city_health_get_population_with_barber_access();
    print_health_building_info(148, BUILDING_BARBER, people_covered, calc_percentage(people_covered, population));

    people_covered = city_health_get_population_with_clinic_access();
    print_health_building_info(168, BUILDING_DOCTOR, people_covered, calc_percentage(people_covered, population));

    people_covered = 1000 * building_count_active(BUILDING_HOSPITAL);
    print_health_building_info(188, BUILDING_HOSPITAL, people_covered, city_culture_coverage_hospital());

    int text_height = lang_text_draw_multiline(56, 7 + get_health_advice(), 45, 226, 560, FONT_NORMAL_BLACK);

    lang_text_draw(CUSTOM_TRANSLATION, TR_ADVISOR_HEALTH_SURVEILLANCE, 45, 246 + text_height, FONT_NORMAL_BLACK);
    text_height += 16;
    text_draw_multiline(translation_for(TR_ADVISOR_SICKNESS_LEVEL_LOW + sickness_level),
        45, 246 + text_height, 560, 0, FONT_NORMAL_BLACK, 0);

    return ADVISOR_HEIGHT;
}

const advisor_window_type *window_advisor_health(void)
{
    static const advisor_window_type window = {
        draw_background,
        0,
        0,
        0
    };
    return &window;
}
