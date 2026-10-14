#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sml_hooks.h"
#include "gbc_video.h"

static int s_tests_passed = 0;
static int s_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        s_tests_passed++; \
    } else { \
        s_tests_failed++; \
        printf("FAILED: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
    } \
} while(0)

int main(void) {
    printf("========================================\n");
    printf("   SUPER MARIO LAND WIDESCREEN TESTS    \n");
    printf("========================================\n");

    gbc_video_init();

    // 1. Standard Mode Tests (Vanilla Parity)
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);
    TEST_ASSERT(sml_hook_get_camera_width_tiles() == 20, "Standard camera must be 20 tiles wide");
    TEST_ASSERT(sml_hook_initial_column_end(0x54) == 0x54, "Standard initial column end must be 0x54 (20 cols)");
    TEST_ASSERT(sml_hook_scroll_threshold(0x50) == 0x50, "Standard scroll threshold must be 0x50 (80 px)");
    TEST_ASSERT(sml_hook_is_enemy_in_active_window(185, 0) == false, "Enemy at X=185 must be outside standard 160+16 px window");
    TEST_ASSERT(sml_hook_is_enemy_in_active_window(100, 0) == true, "Enemy at X=100 must be inside standard window");

    // 2. Widescreen Mode Tests (WonderSwan Color Native 224 px)
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_WIDESCREEN);
    TEST_ASSERT(sml_hook_get_camera_width_tiles() == 28, "Widescreen camera must be 28 tiles wide");
    TEST_ASSERT(sml_hook_initial_column_end(0x54) == 0x5C, "Widescreen initial column end must be 0x5C (28 cols)");
    TEST_ASSERT(sml_hook_scroll_threshold(0x50) == 0x70, "Widescreen scroll threshold must be 0x70 (112 px)");
    TEST_ASSERT(sml_hook_is_enemy_in_active_window(185, 0) == true, "Enemy at X=185 must be inside widescreen 224+16 px window");
    TEST_ASSERT(sml_hook_is_enemy_in_active_window(235, 0) == true, "Enemy at X=235 must be within 224+16 px margin");
    TEST_ASSERT(sml_hook_is_enemy_in_active_window(250, 0) == false, "Enemy at X=250 must be outside widescreen window");

    // 3. Top HUD Centering
    uint8_t hud[32];
    memset(hud, 0x01, sizeof(hud));
    sml_hook_update_hud_layout(hud);
    TEST_ASSERT(hud[0] == 0x00 && hud[1] == 0x00 && hud[2] == 0x00 && hud[3] == 0x00, "HUD margin must be 4 tiles blank");
    TEST_ASSERT(hud[4] == 0x01, "HUD content starts centered at tile 4");

    printf("\nResults:\n");
    printf("  Passed: %d\n", s_tests_passed);
    printf("  Failed: %d\n", s_tests_failed);

    if (s_tests_failed == 0) {
        printf("\nALL SUPER MARIO LAND WIDESCREEN TESTS PASSED.\n");
        return 0;
    } else {
        printf("\nTESTS FAILED.\n");
        return 1;
    }
}
