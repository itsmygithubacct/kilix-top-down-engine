#include "kilix_top_down_view.h"

#include <math.h>
#include <stddef.h>

static bool rect_is_valid(ki_td_rect rect)
{
    return rect.width > 0 && rect.height > 0;
}

bool ki_td_fit_spec_init(ki_td_fit_spec *spec, int logical_width,
                         int logical_height, int framebuffer_width,
                         int framebuffer_height)
{
    if (!spec || logical_width <= 0 || logical_height <= 0 ||
        framebuffer_width <= 0 || framebuffer_height <= 0)
        return false;

    spec->logical_width = logical_width;
    spec->logical_height = logical_height;
    spec->fit_bounds = (ki_td_rect){0, 0, framebuffer_width,
                                    framebuffer_height};
    spec->align_bounds = spec->fit_bounds;
    spec->scale_policy = KI_TD_SCALE_FRACTIONAL;
    spec->minimum_scale = 0.0f;
    spec->integer_scale_threshold = 2.0f;
    spec->clamp_origin_x = false;
    spec->clamp_origin_y = false;
    return true;
}

bool ki_td_view_fit(ki_td_view *view, const ki_td_fit_spec *spec)
{
    if (!view || !spec || spec->logical_width <= 0 ||
        spec->logical_height <= 0 || !rect_is_valid(spec->fit_bounds) ||
        !rect_is_valid(spec->align_bounds) ||
        !isfinite(spec->minimum_scale) || spec->minimum_scale < 0.0f ||
        !isfinite(spec->integer_scale_threshold) ||
        spec->integer_scale_threshold < 1.0f ||
        spec->scale_policy < KI_TD_SCALE_FRACTIONAL ||
        spec->scale_policy > KI_TD_SCALE_PIXEL_ART)
        return false;

    float scale_x = (float)spec->fit_bounds.width /
                    (float)spec->logical_width;
    float scale_y = (float)spec->fit_bounds.height /
                    (float)spec->logical_height;
    float candidate = fminf(scale_x, scale_y);
    if (!isfinite(candidate) || candidate <= 0.0f)
        return false;

    float scale = candidate;
    if (spec->scale_policy == KI_TD_SCALE_INTEGER) {
        scale = floorf(candidate);
    } else if (spec->scale_policy == KI_TD_SCALE_PIXEL_ART) {
        scale = floorf(candidate);
        if (scale < spec->integer_scale_threshold)
            scale = candidate;
    }
    if (scale < spec->minimum_scale)
        scale = spec->minimum_scale;
    if (!isfinite(scale) || scale <= 0.0f)
        return false;

    ki_td_view next = {0};
    next.logical_width = spec->logical_width;
    next.logical_height = spec->logical_height;
    next.scale = scale;
    next.origin_x = spec->align_bounds.x +
        (int)(((float)spec->align_bounds.width -
               (float)spec->logical_width * scale) *
              0.5f);
    next.origin_y = spec->align_bounds.y +
        (int)(((float)spec->align_bounds.height -
               (float)spec->logical_height * scale) *
              0.5f);
    if (spec->clamp_origin_x && next.origin_x < spec->align_bounds.x)
        next.origin_x = spec->align_bounds.x;
    if (spec->clamp_origin_y && next.origin_y < spec->align_bounds.y)
        next.origin_y = spec->align_bounds.y;
    *view = next;
    return true;
}

void ki_td_view_set_offset(ki_td_view *view, int x, int y)
{
    if (!view) return;
    view->offset_x = x;
    view->offset_y = y;
}

int ki_td_screen_x(const ki_td_view *view, float logical_x)
{
    if (!view || !isfinite(logical_x)) return 0;
    return view->origin_x + view->offset_x +
           (int)floorf(logical_x * view->scale + 0.5f);
}

int ki_td_screen_y(const ki_td_view *view, float logical_y)
{
    if (!view || !isfinite(logical_y)) return 0;
    return view->origin_y + view->offset_y +
           (int)floorf(logical_y * view->scale + 0.5f);
}

float ki_td_screen_scale(const ki_td_view *view, float logical_length)
{
    if (!view || !isfinite(logical_length)) return 0.0f;
    return logical_length * view->scale;
}

static uint32_t visual_noise(uint32_t value)
{
    value ^= value >> 16;
    value *= UINT32_C(0x7feb352d);
    value ^= value >> 15;
    value *= UINT32_C(0x846ca68b);
    value ^= value >> 16;
    return value;
}

int ki_td_shake_axis(uint32_t frame, float magnitude, uint32_t salt)
{
    if (!isfinite(magnitude) || magnitude <= 0.0f) return 0;
    uint32_t bits = visual_noise(frame ^ salt);
    float unit = (float)(bits >> 8) * (1.0f / 16777216.0f);
    return (int)((unit - 0.5f) * magnitude);
}
