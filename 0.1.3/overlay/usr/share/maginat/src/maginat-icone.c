#include <cairo.h>
#include <math.h>
#include <stdio.h>
static void bell(cairo_t *cr, double s, int dot)
{
    cairo_set_source_rgb(cr, 0.122, 0.165, 0.216);
    cairo_arc(cr, .5*s, .40*s, .26*s, M_PI, 0);
    cairo_line_to(cr, .80*s, .70*s);
    cairo_line_to(cr, .20*s, .70*s);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_arc(cr, .5*s, .80*s, .09*s, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_arc(cr, .5*s, .12*s, .06*s, 0, 2*M_PI);
    cairo_fill(cr);
    if (dot) {
        cairo_set_source_rgb(cr, 0.85, 0.15, 0.15);
        cairo_arc(cr, .78*s, .24*s, .16*s, 0, 2*M_PI);
        cairo_fill(cr);
    }
}
static void wifi(cairo_t *cr, double s, int on)
{
    double r;
    cairo_set_source_rgba(cr, 0.122, 0.165, 0.216, on ? 1.0 : 0.4);
    cairo_set_line_width(cr, .10*s);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for (r = .28; r < .75; r += .20) {
        cairo_new_path(cr);
        cairo_arc(cr, .5*s, .86*s, r*s, -135*M_PI/180, -45*M_PI/180);
        cairo_stroke(cr);
    }
    cairo_arc(cr, .5*s, .86*s, .07*s, 0, 2*M_PI);
    cairo_fill(cr);
    if (!on) {
        cairo_set_source_rgb(cr, 0.85, 0.15, 0.15);
        cairo_set_line_width(cr, .09*s);
        cairo_move_to(cr, .12*s, .10*s);
        cairo_line_to(cr, .88*s, .90*s);
        cairo_stroke(cr);
    }
}
static void fai(const char *dir, int size, const char *nome, int tipo)
{
    char path[512];
    cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(sf);
    if (tipo == 0) bell(cr, size, 0);
    else if (tipo == 1) bell(cr, size, 1);
    else if (tipo == 2) wifi(cr, size, 1);
    else wifi(cr, size, 0);
    snprintf(path, sizeof path, "%s/%dx%d/status/%s.png", dir, size, size, nome);
    cairo_surface_write_to_png(sf, path);
    cairo_destroy(cr);
    cairo_surface_destroy(sf);
}
int main(int argc, char **argv)
{
    int sizes[] = {16, 22, 24, 32, 48};
    int i;
    if (argc < 2) return 1;
    for (i = 0; i < 5; i++) {
        fai(argv[1], sizes[i], "notification-symbolic", 0);
        fai(argv[1], sizes[i], "notifications-symbolic", 0);
        fai(argv[1], sizes[i], "notification-new-symbolic", 1);
        fai(argv[1], sizes[i], "notifications-new-symbolic", 1);
        fai(argv[1], sizes[i], "maginat-wifi-on", 2);
        fai(argv[1], sizes[i], "maginat-wifi-off", 3);
    }
    return 0;
}
