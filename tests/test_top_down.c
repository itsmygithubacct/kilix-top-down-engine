#include "kilix_top_down.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define EXPECT(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static uint64_t hash_bytes(const uint8_t *bytes, size_t count)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < count; i++) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static void test_fit_and_transform(void)
{
    ki_td_fit_spec spec;
    ki_td_view view = {0};
    EXPECT(!ki_td_fit_spec_init(NULL, 1, 1, 1, 1));
    EXPECT(!ki_td_fit_spec_init(&spec, 0, 1, 1, 1));
    EXPECT(ki_td_fit_spec_init(&spec, 256, 176, 960, 540));
    spec.fit_bounds = (ki_td_rect){0, 64, 960, 466};
    spec.align_bounds = (ki_td_rect){0, 64, 960, 476};
    spec.scale_policy = KI_TD_SCALE_PIXEL_ART;
    spec.minimum_scale = 1.0f;
    spec.integer_scale_threshold = 2.0f;
    spec.clamp_origin_y = true;
    EXPECT(ki_td_view_fit(&view, &spec));
    EXPECT(view.scale == 2.0f);
    EXPECT(view.origin_x == 224);
    EXPECT(view.origin_y == 126);
    EXPECT(ki_td_screen_x(&view, 0.0f) == 224);
    EXPECT(ki_td_screen_y(&view, 0.0f) == 126);
    EXPECT(ki_td_screen_x(&view, 12.25f) == 249);
    EXPECT(ki_td_screen_scale(&view, 3.25f) == 6.5f);

    ki_td_view_set_offset(&view, -3, 4);
    EXPECT(ki_td_screen_x(&view, 12.25f) == 246);
    EXPECT(ki_td_screen_y(&view, 5.0f) == 140);

    EXPECT(ki_td_fit_spec_init(&spec, 100, 100, 150, 170));
    spec.scale_policy = KI_TD_SCALE_PIXEL_ART;
    spec.integer_scale_threshold = 2.0f;
    EXPECT(ki_td_view_fit(&view, &spec));
    EXPECT(fabsf(view.scale - 1.5f) < 0.0001f);
    EXPECT(view.origin_x == 0 && view.origin_y == 10);

    spec.fit_bounds = (ki_td_rect){0, 0, 290, 290};
    spec.align_bounds = spec.fit_bounds;
    EXPECT(ki_td_view_fit(&view, &spec));
    EXPECT(view.scale == 2.0f);
    EXPECT(view.origin_x == 45 && view.origin_y == 45);

    spec.fit_bounds = (ki_td_rect){0, 0, 50, 50};
    spec.align_bounds = spec.fit_bounds;
    spec.minimum_scale = 1.0f;
    EXPECT(ki_td_view_fit(&view, &spec));
    EXPECT(view.scale == 1.0f);
    EXPECT(view.origin_x == -25 && view.origin_y == -25);

    view = (ki_td_view){.scale = 2.0f, .origin_x = 10, .origin_y = 20};
    EXPECT(ki_td_screen_x(&view, -0.25f) == 10);
    EXPECT(ki_td_screen_x(&view, -0.75f) == 9);
}

static void test_shake(void)
{
    int x = ki_td_shake_axis(UINT32_C(12345), 7.0f,
                             UINT32_C(0x6d2b79f5));
    int y = ki_td_shake_axis(UINT32_C(12345), 7.0f,
                             UINT32_C(0x1b873593));
    EXPECT(x == ki_td_shake_axis(UINT32_C(12345), 7.0f,
                                 UINT32_C(0x6d2b79f5)));
    EXPECT(y == ki_td_shake_axis(UINT32_C(12345), 7.0f,
                                 UINT32_C(0x1b873593)));
    EXPECT(x >= -3 && x <= 3);
    EXPECT(y >= -3 && y <= 3);
    EXPECT(ki_td_shake_axis(1u, 0.0f, 2u) == 0);
    EXPECT(ki_td_shake_axis(1u, -1.0f, 2u) == 0);
}

static void test_renderer_lifetime(void)
{
    ki_td_soft_renderer renderer = {0};
    EXPECT(!ki_td_soft_renderer_resize(&renderer, 0, 12));
    EXPECT(ki_td_soft_renderer_init(&renderer, 16, 12));
    EXPECT(ki_td_soft_width(&renderer) == 16);
    EXPECT(ki_td_soft_height(&renderer) == 12);
    EXPECT(ki_td_soft_canvas(&renderer) != NULL);

    uint8_t *original_rgba = renderer.rgba;
    uint32_t *original_pixels = renderer.canvas.px;
    EXPECT(ki_td_soft_renderer_resize(&renderer, 16, 12));
    EXPECT(renderer.rgba == original_rgba);
    EXPECT(renderer.canvas.px == original_pixels);
    EXPECT(!ki_td_soft_renderer_resize(&renderer, -1, 12));
    EXPECT(renderer.rgba == original_rgba);
    EXPECT(renderer.canvas.px == original_pixels);

    ki_td_soft_clear(&renderer, UINT32_C(0x123456));
    uint8_t *packed = ki_td_soft_pack_rgba(&renderer);
    EXPECT(packed != NULL);
    EXPECT(packed[0] == 0x12u && packed[1] == 0x34u && packed[2] == 0x56u &&
           packed[3] == 0xffu);
    EXPECT(ki_td_soft_renderer_resize(&renderer, 9, 7));
    EXPECT(ki_td_soft_width(&renderer) == 9);
    EXPECT(ki_td_soft_height(&renderer) == 7);
    ki_td_soft_renderer_destroy(&renderer);
    EXPECT(renderer.rgba == NULL && renderer.canvas.px == NULL);
    EXPECT(ki_td_soft_width(&renderer) == 0);
    ki_td_soft_renderer_destroy(&renderer);
}

static void draw_adapter_golden(ki_td_soft_renderer *renderer)
{
    static const uint8_t sprite_pixels[] = {
        255,   0,   0, 255,    0, 255,   0,   7,    0,   0, 255, 128,
        255, 255, 255, 255,  255, 192,   0, 192,   32, 224, 255, 255
    };
    ki_td_rgba8 sprite = ki_td_rgba8_make(sprite_pixels, 3, 2);
    ki_td_fit_spec spec;
    ki_td_view view;
    EXPECT(ki_td_fit_spec_init(&spec, 24, 18, renderer->width,
                               renderer->height));
    spec.scale_policy = KI_TD_SCALE_PIXEL_ART;
    spec.minimum_scale = 1.0f;
    EXPECT(ki_td_view_fit(&view, &spec));

    ki_td_soft_clear(renderer, UINT32_C(0x07111d));
    ki_td_soft_fill_rect(renderer, &view, 1.0f, 1.0f, 22.0f, 16.0f,
                         UINT32_C(0x183153), 1.0f);
    ki_td_soft_fill_circle(renderer, &view, 6.0f, 7.0f, 3.5f,
                           UINT32_C(0xe64980), 0.85f);
    ki_td_soft_fill_ellipse(renderer, &view, 17.0f, 6.0f, 4.0f, 2.0f,
                            UINT32_C(0x74c0fc), 0.75f);
    ki_td_soft_line(renderer, &view, 2.0f, 15.0f, 22.0f, 10.0f, 1.25f,
                    UINT32_C(0xffd166), 0.9f);
    ki_td_soft_rgba_pixel_art(renderer, &view, 3.0f, 2.0f, &sprite, 1.0f);
    ki_td_soft_rgba_resized(renderer, &view, 10.0f, 2.0f, &sprite, 6, 4,
                            0.8f);
    ki_td_soft_rgba_rotated(renderer, &view, 3.0f, 11.0f, &sprite, 1, 1.0f);
    ki_td_soft_rgba_rotated(renderer, &view, 8.0f, 11.0f, &sprite, 2, 1.0f);
    ki_td_soft_rgba_rotated(renderer, &view, 14.0f, 11.0f, &sprite, 3, 1.0f);
    ki_td_soft_rgba_px(renderer, 1, 1, &sprite, 0.65f);
}

static void test_adapter(void)
{
    static const uint8_t cutoff_pixels[] = {
        255, 0, 0, 255, 0, 255, 0, 7
    };
    ki_td_rgba8 cutoff = ki_td_rgba8_make(cutoff_pixels, 2, 1);
    EXPECT(ki_td_rgba8_is_valid(&cutoff));
    ki_td_rgba8 invalid = cutoff;
    invalid.stride = 3;
    EXPECT(!ki_td_rgba8_is_valid(&invalid));

    ki_td_soft_renderer renderer = {0};
    EXPECT(ki_td_soft_renderer_init(&renderer, 96, 72));
    ki_td_soft_clear(&renderer, 0);
    ki_td_soft_rgba_px(&renderer, 2, 2, &cutoff, 1.0f);
    EXPECT(renderer.canvas.px[2 + 2 * renderer.canvas.w] ==
           UINT32_C(0xffff0000));
    EXPECT(renderer.canvas.px[3 + 2 * renderer.canvas.w] ==
           UINT32_C(0xff000000));

    draw_adapter_golden(&renderer);
    uint8_t *rgba = ki_td_soft_pack_rgba(&renderer);
    EXPECT(rgba != NULL);
    uint64_t hash = hash_bytes(rgba, renderer.rgba_size);
    const uint64_t expected = UINT64_C(0xf541ab62fc7fee0c);
    if (hash != expected) {
        fprintf(stderr, "adapter golden hash: %016" PRIx64 "\n", hash);
        failures++;
    }
    ki_td_soft_renderer_destroy(&renderer);
}

int main(void)
{
    test_fit_and_transform();
    test_shake();
    test_renderer_lifetime();
    test_adapter();
    if (failures != 0) {
        fprintf(stderr, "FAIL: %d top-down checks\n", failures);
        return 1;
    }
    printf("PASS: top-down view, shake, lifecycle, primitives, and RGBA adapter\n");
    return 0;
}
