#include "kilix_top_down_soft.h"
#include "kilix_top_down_view.h"

#include <math.h>
#include <stdlib.h>

static bool renderer_is_ready(const ki_td_soft_renderer *renderer)
{
    return renderer && renderer->canvas.px && renderer->rgba &&
           renderer->width > 0 && renderer->height > 0;
}

ki_td_rgba8 ki_td_rgba8_make(const void *pixels, int width, int height)
{
    ki_td_rgba8 image = {0};
    if (!pixels || width <= 0 || height <= 0 ||
        (size_t)width > SIZE_MAX / 4u)
        return image;
    image.pixels = pixels;
    image.width = width;
    image.height = height;
    image.stride = (size_t)width * 4u;
    return image;
}

bool ki_td_rgba8_is_valid(const ki_td_rgba8 *image)
{
    if (!image || !image->pixels || image->width <= 0 || image->height <= 0 ||
        (size_t)image->width > SIZE_MAX / 4u)
        return false;
    return image->stride >= (size_t)image->width * 4u &&
           (size_t)image->height <= SIZE_MAX / image->stride;
}

bool ki_td_soft_renderer_resize(ki_td_soft_renderer *renderer, int width,
                                int height)
{
    if (!renderer || width <= 0 || height <= 0 ||
        (size_t)width > SIZE_MAX / (size_t)height / 4u)
        return false;
    if (renderer_is_ready(renderer) && renderer->width == width &&
        renderer->height == height)
        return true;

    sr_canvas next_canvas = {0};
    if (!sr_canvas_init(&next_canvas, width, height))
        return false;
    size_t next_size = (size_t)width * (size_t)height * 4u;
    uint8_t *next_rgba = realloc(renderer->rgba, next_size);
    if (!next_rgba) {
        sr_canvas_free(&next_canvas);
        return false;
    }

    sr_canvas_free(&renderer->canvas);
    renderer->canvas = next_canvas;
    renderer->rgba = next_rgba;
    renderer->rgba_size = next_size;
    renderer->width = width;
    renderer->height = height;
    return true;
}

bool ki_td_soft_renderer_init(ki_td_soft_renderer *renderer, int width,
                              int height)
{
    if (!renderer) return false;
    ki_td_soft_renderer_destroy(renderer);
    return ki_td_soft_renderer_resize(renderer, width, height);
}

void ki_td_soft_renderer_destroy(ki_td_soft_renderer *renderer)
{
    if (!renderer) return;
    free(renderer->rgba);
    renderer->rgba = NULL;
    renderer->rgba_size = 0;
    sr_canvas_free(&renderer->canvas);
    renderer->width = 0;
    renderer->height = 0;
}

sr_canvas *ki_td_soft_canvas(ki_td_soft_renderer *renderer)
{
    return renderer ? &renderer->canvas : NULL;
}

const sr_canvas *ki_td_soft_canvas_const(const ki_td_soft_renderer *renderer)
{
    return renderer ? &renderer->canvas : NULL;
}

int ki_td_soft_width(const ki_td_soft_renderer *renderer)
{
    return renderer ? renderer->width : 0;
}

int ki_td_soft_height(const ki_td_soft_renderer *renderer)
{
    return renderer ? renderer->height : 0;
}

uint8_t *ki_td_soft_pack_rgba(ki_td_soft_renderer *renderer)
{
    if (!renderer_is_ready(renderer) ||
        !sr_pack_rgba(&renderer->canvas, renderer->rgba, renderer->rgba_size))
        return NULL;
    return renderer->rgba;
}

void ki_td_soft_clear(ki_td_soft_renderer *renderer, uint32_t rgb)
{
    if (renderer) sr_clear(&renderer->canvas, rgb);
}

void ki_td_soft_blend_pixel(ki_td_soft_renderer *renderer, int x, int y,
                            uint32_t rgb, float alpha)
{
    if (renderer) sr_blend(&renderer->canvas, x, y, rgb, alpha);
}

void ki_td_soft_fill_rect_px(ki_td_soft_renderer *renderer, float x, float y,
                             float width, float height, uint32_t rgb,
                             float alpha)
{
    if (renderer)
        sr_fill_rect(&renderer->canvas, x, y, width, height, rgb, alpha);
}

void ki_td_soft_fill_circle_px(ki_td_soft_renderer *renderer, float x, float y,
                               float radius, uint32_t rgb, float alpha)
{
    if (renderer)
        sr_fill_circle(&renderer->canvas, x, y, radius, rgb, alpha);
}

void ki_td_soft_fill_ellipse_px(ki_td_soft_renderer *renderer, float x,
                                float y, float radius_x, float radius_y,
                                uint32_t rgb, float alpha)
{
    if (renderer)
        sr_fill_ellipse(&renderer->canvas, x, y, radius_x, radius_y, rgb,
                        alpha);
}

void ki_td_soft_line_px(ki_td_soft_renderer *renderer, float x0, float y0,
                        float x1, float y1, float width, uint32_t rgb,
                        float alpha)
{
    if (renderer)
        sr_line(&renderer->canvas, x0, y0, x1, y1, width, rgb, alpha, 0, 0);
}

void ki_td_soft_fill_rect(ki_td_soft_renderer *renderer,
                          const ki_td_view *view, float x, float y,
                          float width, float height, uint32_t rgb,
                          float alpha)
{
    if (!view) return;
    ki_td_soft_fill_rect_px(renderer, (float)ki_td_screen_x(view, x),
                            (float)ki_td_screen_y(view, y),
                            ki_td_screen_scale(view, width),
                            ki_td_screen_scale(view, height), rgb, alpha);
}

void ki_td_soft_fill_circle(ki_td_soft_renderer *renderer,
                            const ki_td_view *view, float x, float y,
                            float radius, uint32_t rgb, float alpha)
{
    if (!view) return;
    ki_td_soft_fill_circle_px(renderer, (float)ki_td_screen_x(view, x),
                              (float)ki_td_screen_y(view, y),
                              ki_td_screen_scale(view, radius), rgb, alpha);
}

void ki_td_soft_fill_ellipse(ki_td_soft_renderer *renderer,
                             const ki_td_view *view, float x, float y,
                             float radius_x, float radius_y, uint32_t rgb,
                             float alpha)
{
    if (!view) return;
    ki_td_soft_fill_ellipse_px(renderer, (float)ki_td_screen_x(view, x),
                               (float)ki_td_screen_y(view, y),
                               ki_td_screen_scale(view, radius_x),
                               ki_td_screen_scale(view, radius_y), rgb,
                               alpha);
}

void ki_td_soft_line(ki_td_soft_renderer *renderer, const ki_td_view *view,
                     float x0, float y0, float x1, float y1, float width,
                     uint32_t rgb, float alpha)
{
    if (!view) return;
    ki_td_soft_line_px(renderer, (float)ki_td_screen_x(view, x0),
                       (float)ki_td_screen_y(view, y0),
                       (float)ki_td_screen_x(view, x1),
                       (float)ki_td_screen_y(view, y1),
                       ki_td_screen_scale(view, width), rgb, alpha);
}

static const uint8_t *image_pixel(const ki_td_rgba8 *image, int x, int y)
{
    return image->pixels + (size_t)y * image->stride + (size_t)x * 4u;
}

static void blend_rgba_pixel(ki_td_soft_renderer *renderer, int x, int y,
                             const uint8_t *pixel, float alpha)
{
    if (pixel[3] < 8u) return;
    uint32_t rgb = ((uint32_t)pixel[0] << 16) |
                   ((uint32_t)pixel[1] << 8) | (uint32_t)pixel[2];
    ki_td_soft_blend_pixel(renderer, x, y, rgb,
                           alpha * (pixel[3] / 255.0f));
}

static void fill_rgba_world_pixel(ki_td_soft_renderer *renderer,
                                  const ki_td_view *view, float x, float y,
                                  const uint8_t *pixel, float alpha)
{
    if (pixel[3] < 8u) return;
    uint32_t rgb = ((uint32_t)pixel[0] << 16) |
                   ((uint32_t)pixel[1] << 8) | (uint32_t)pixel[2];
    ki_td_soft_fill_rect(renderer, view, x, y, 1.0f, 1.0f, rgb,
                         alpha * (pixel[3] / 255.0f));
}

void ki_td_soft_rgba_px(ki_td_soft_renderer *renderer, int x, int y,
                        const ki_td_rgba8 *image, float alpha)
{
    if (!renderer || !ki_td_rgba8_is_valid(image)) return;
    for (int yy = 0; yy < image->height; yy++)
        for (int xx = 0; xx < image->width; xx++)
            blend_rgba_pixel(renderer, x + xx, y + yy,
                             image_pixel(image, xx, yy), alpha);
}

void ki_td_soft_rgba(ki_td_soft_renderer *renderer, const ki_td_view *view,
                     float x, float y, const ki_td_rgba8 *image, float alpha)
{
    if (!renderer || !view || !ki_td_rgba8_is_valid(image)) return;
    for (int yy = 0; yy < image->height; yy++)
        for (int xx = 0; xx < image->width; xx++)
            fill_rgba_world_pixel(renderer, view, x + (float)xx,
                                  y + (float)yy,
                                  image_pixel(image, xx, yy), alpha);
}

void ki_td_soft_rgba_resized(ki_td_soft_renderer *renderer,
                             const ki_td_view *view, float x, float y,
                             const ki_td_rgba8 *image, int width, int height,
                             float alpha)
{
    if (!renderer || !view || !ki_td_rgba8_is_valid(image) || width <= 0 ||
        height <= 0)
        return;
    for (int yy = 0; yy < height; yy++) {
        int source_y = yy * image->height / height;
        for (int xx = 0; xx < width; xx++) {
            int source_x = xx * image->width / width;
            fill_rgba_world_pixel(renderer, view, x + (float)xx,
                                  y + (float)yy,
                                  image_pixel(image, source_x, source_y),
                                  alpha);
        }
    }
}

void ki_td_soft_rgba_rotated(ki_td_soft_renderer *renderer,
                             const ki_td_view *view, float x, float y,
                             const ki_td_rgba8 *image, int quarter_turns,
                             float alpha)
{
    if (!renderer || !view || !ki_td_rgba8_is_valid(image)) return;
    int turns = quarter_turns % 4;
    if (turns < 0) turns += 4;
    if (turns == 0) {
        ki_td_soft_rgba(renderer, view, x, y, image, alpha);
        return;
    }

    for (int source_y = 0; source_y < image->height; source_y++) {
        for (int source_x = 0; source_x < image->width; source_x++) {
            int dest_x = source_x;
            int dest_y = source_y;
            if (turns == 1) {
                dest_x = image->height - 1 - source_y;
                dest_y = source_x;
            } else if (turns == 2) {
                dest_x = image->width - 1 - source_x;
                dest_y = image->height - 1 - source_y;
            } else {
                dest_x = source_y;
                dest_y = image->width - 1 - source_x;
            }
            fill_rgba_world_pixel(renderer, view, x + (float)dest_x,
                                  y + (float)dest_y,
                                  image_pixel(image, source_x, source_y),
                                  alpha);
        }
    }
}

void ki_td_soft_rgba_pixel_art(ki_td_soft_renderer *renderer,
                               const ki_td_view *view, float x, float y,
                               const ki_td_rgba8 *image, float alpha)
{
    if (!renderer || !view || !ki_td_rgba8_is_valid(image)) return;
    int scale = (int)(view->scale + 0.5f);
    if (scale < 1 || fabsf(view->scale - (float)scale) > 0.01f) {
        ki_td_soft_rgba(renderer, view, x, y, image, alpha);
        return;
    }

    int origin_x = ki_td_screen_x(view, x);
    int origin_y = ki_td_screen_y(view, y);
    for (int yy = 0; yy < image->height; yy++) {
        for (int xx = 0; xx < image->width; xx++) {
            const uint8_t *pixel = image_pixel(image, xx, yy);
            if (pixel[3] < 8u) continue;
            uint32_t rgb = ((uint32_t)pixel[0] << 16) |
                           ((uint32_t)pixel[1] << 8) |
                           (uint32_t)pixel[2];
            int pixel_x = origin_x + xx * scale;
            int pixel_y = origin_y + yy * scale;
            if (pixel[3] == 255u) {
                ki_td_soft_fill_rect_px(renderer, (float)pixel_x,
                                        (float)pixel_y, (float)scale,
                                        (float)scale, rgb, alpha);
                continue;
            }
            for (int dy = 0; dy < scale; dy++)
                for (int dx = 0; dx < scale; dx++)
                    ki_td_soft_blend_pixel(
                        renderer, pixel_x + dx, pixel_y + dy, rgb,
                        alpha * (pixel[3] / 255.0f));
        }
    }
}
