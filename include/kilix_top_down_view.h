#ifndef KILIX_TOP_DOWN_VIEW_H
#define KILIX_TOP_DOWN_VIEW_H

#include "kilix_top_down_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes a fit specification that uses the entire framebuffer and a
 * fractional scale. Games can then narrow fit_bounds, choose independent
 * alignment bounds, or select an integer-oriented scale policy. */
bool ki_td_fit_spec_init(ki_td_fit_spec *spec, int logical_width,
                         int logical_height, int framebuffer_width,
                         int framebuffer_height);

/* Computes a deterministic orthographic view. KI_TD_SCALE_PIXEL_ART uses the
 * largest integer scale while it is at least integer_scale_threshold, then
 * falls back to the fractional fit. minimum_scale may deliberately make a
 * logical scene crop on a small framebuffer. */
bool ki_td_view_fit(ki_td_view *view, const ki_td_fit_spec *spec);

void ki_td_view_set_offset(ki_td_view *view, int x, int y);
int ki_td_screen_x(const ki_td_view *view, float logical_x);
int ki_td_screen_y(const ki_td_view *view, float logical_y);
float ki_td_screen_scale(const ki_td_view *view, float logical_length);

/* Frame-derived visual shake. It does not consume or mutate game RNG state. */
int ki_td_shake_axis(uint32_t frame, float magnitude, uint32_t salt);

#ifdef __cplusplus
}
#endif

#endif /* KILIX_TOP_DOWN_VIEW_H */
