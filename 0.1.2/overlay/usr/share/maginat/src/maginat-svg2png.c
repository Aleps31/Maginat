/* maginat-svg2png: converte un'icona SVG in PNG (usa nanosvg, licenza zlib)
 * Uso: maginat-svg2png entrata.svg uscita.png dimensione
 * Licenza del programma: GPL-2.0-or-later
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cairo.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

int main(int argc, char **argv)
{
    NSVGimage *img;
    NSVGrasterizer *rast;
    int size, w, h, x, y, ox, oy;
    unsigned char *buf;
    cairo_surface_t *s;
    unsigned char *dst;
    int stride;
    float sc;

    if (argc < 4) { fprintf(stderr, "uso: %s entrata.svg uscita.png dimensione\n", argv[0]); return 2; }
    size = atoi(argv[3]);
    if (size < 8 || size > 1024) return 2;
    img = nsvgParseFromFile(argv[1], "px", 96);
    if (!img || img->width <= 0 || img->height <= 0) return 1;
    rast = nsvgCreateRasterizer();
    if (!rast) return 1;
    sc = (float)size / (img->width > img->height ? img->width : img->height);
    w = (int)ceilf(img->width * sc);
    h = (int)ceilf(img->height * sc);
    if (w > size) w = size;
    if (h > size) h = size;
    ox = (size - w) / 2;
    oy = (size - h) / 2;
    buf = calloc((size_t)w * h * 4, 1);
    if (!buf) return 1;
    nsvgRasterize(rast, img, 0, 0, sc, buf, w, h, w * 4);
    s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_surface_flush(s);
    dst = cairo_image_surface_get_data(s);
    stride = cairo_image_surface_get_stride(s);
    memset(dst, 0, (size_t)stride * size);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            unsigned char *p = buf + ((size_t)y * w + x) * 4;
            unsigned int a = p[3];
            unsigned int *q = (unsigned int *)(dst + (size_t)(y + oy) * stride) + (x + ox);
            *q = (a << 24) | (((p[0] * a + 127) / 255) << 16) | (((p[1] * a + 127) / 255) << 8) | ((p[2] * a + 127) / 255);
        }
    cairo_surface_mark_dirty(s);
    if (cairo_surface_write_to_png(s, argv[2]) != CAIRO_STATUS_SUCCESS) return 1;
    cairo_surface_destroy(s);
    free(buf);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(img);
    return 0;
}
