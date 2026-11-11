#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define DURATA 6.0

static gint64 t0;
static GtkWidget *win;

static double sm(double x)
{
    if (x < 0) x = 0;
    if (x > 1) x = 1;
    return x * x * (3 - 2 * x);
}

static double eo(double x)
{
    return 1 - pow(1 - x, 3);
}

static void line(cairo_t *cr, double x1, double y1, double x2, double y2)
{
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

static void logo(cairo_t *cr, double a)
{
    cairo_save(cr);
    cairo_translate(cr, -420, -420);
    cairo_push_group(cr);
    cairo_set_source_rgb(cr, 7 / 255.0, 20 / 255.0, 42 / 255.0);
    cairo_arc(cr, 420, 420, 388, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 43);
    cairo_arc(cr, 420, 420, 343.5, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_set_line_width(cr, 85);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgb(cr, 1, 1, 1);
    line(cr, 250, 553, 250, 290);
    line(cr, 580, 553, 580, 290);
    line(cr, 413, 497, 580, 290);
    cairo_set_source_rgb(cr, 185 / 255.0, 192 / 255.0, 204 / 255.0);
    line(cr, 250, 290, 413, 485);
    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, a);
    cairo_restore(cr);
}

static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer d)
{
    double t = (g_get_monotonic_time() - t0) / 1e6;
    int W = gtk_widget_get_allocated_width(w);
    int H = gtk_widget_get_allocated_height(w);
    double cx = W / 2.0, cy = H / 2.0;
    double k = H / 720.0;
    double a = sm(t / 1.2);
    double out = t > DURATA - 2 ? sm((t - (DURATA - 2)) / 2.0) : 0;
    double th = t < 2.8 ? M_PI * 6 * (1 - eo(t / 2.8)) : 0;
    double sx = fmax(fabs(cos(th)), 0.02);
    double sc = 0.55 + 0.45 * eo(fmin(1, t / 2.8));
    double ga, R, ang, sa, u;
    cairo_pattern_t *p;

    a *= 1 - out;
    if (t > 2.8) sc *= 1 + 0.025 * sin(2 * M_PI * (t - 2.8) / 2.2);
    sc *= 1 + 0.5 * out;

    cairo_set_source_rgb(cr, 11 / 255.0, 18 / 255.0, 32 / 255.0);
    cairo_paint(cr);

    ga = a * (0.35 + 0.25 * sin(2 * M_PI * t / 2.2));
    if (ga < 0) ga = 0;
    p = cairo_pattern_create_radial(cx, cy, 0, cx, cy, 330 * k);
    cairo_pattern_add_color_stop_rgba(p, 0, 40 / 255.0, 90 / 255.0, 170 / 255.0, ga);
    cairo_pattern_add_color_stop_rgba(p, 1, 40 / 255.0, 90 / 255.0, 170 / 255.0, 0);
    cairo_set_source(cr, p);
    cairo_paint(cr);
    cairo_pattern_destroy(p);

    if (t > 1.0) {
        sa = a * sm(t - 1.0);
        R = 170 * k * sc + 26 * k;
        ang = fmod(t * 260, 360) * M_PI / 180;
        cairo_set_source_rgba(cr, 120 / 255.0, 170 / 255.0, 235 / 255.0, sa);
        cairo_set_line_width(cr, 5 * k);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        cairo_new_path(cr);
        cairo_arc(cr, cx, cy, R, ang, ang + 110 * M_PI / 180);
        cairo_stroke(cr);
        cairo_arc(cr, cx, cy, R, ang + M_PI, ang + M_PI + 50 * M_PI / 180);
        cairo_stroke(cr);
    }

    u = 340.0 * k * sc / 776.0;
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, u * sx, u);
    logo(cr, a);
    cairo_restore(cr);
    return FALSE;
}

static gboolean tick(gpointer d)
{
    double t = (g_get_monotonic_time() - t0) / 1e6;
    if (t >= DURATA) {
        gtk_main_quit();
        return FALSE;
    }
    gdk_window_raise(gtk_widget_get_window(win));
    gtk_widget_queue_draw(win);
    return TRUE;
}

static gboolean on_click(GtkWidget *w, GdkEvent *e, gpointer d)
{
    gtk_main_quit();
    return TRUE;
}

static gboolean on_key(GtkWidget *w, GdkEvent *e, gpointer d)
{
    gtk_main_quit();
    return TRUE;
}

static int gia_visto(void)
{
    char p[96];
    int fd;
    snprintf(p, sizeof p, "/dev/shm/maginat-splash-%u", (unsigned)getuid());
    if (access(p, F_OK) == 0) return 1;
    fd = open(p, O_CREAT | O_WRONLY, 0600);
    if (fd >= 0) close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    int i, sempre = 0, nofork = 0;
    GdkDisplay *dpy;
    GdkMonitor *mon;
    GdkRectangle r;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--sempre")) sempre = 1;
        if (!strcmp(argv[i], "--no-fork")) nofork = 1;
    }
    if (!sempre && gia_visto()) return 0;

    if (!nofork) {
        pid_t pid = fork();
        if (pid > 0) return 0;
        if (pid < 0) return 1;
        setsid();
    }

    gtk_init(&argc, &argv);
    dpy = gdk_display_get_default();
    mon = gdk_display_get_primary_monitor(dpy);
    if (!mon) mon = gdk_display_get_monitor(dpy, 0);
    gdk_monitor_get_geometry(mon, &r);

    win = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_move(GTK_WINDOW(win), r.x, r.y);
    gtk_window_set_default_size(GTK_WINDOW(win), r.width, r.height);
    gtk_widget_set_app_paintable(win, TRUE);
    gtk_widget_add_events(win, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);
    g_signal_connect(win, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(win, "button-press-event", G_CALLBACK(on_click), NULL);
    g_signal_connect(win, "key-press-event", G_CALLBACK(on_key), NULL);
    gtk_widget_show_all(win);
    gdk_window_move_resize(gtk_widget_get_window(win), r.x, r.y, r.width, r.height);
    gdk_seat_grab(gdk_display_get_default_seat(dpy), gtk_widget_get_window(win), GDK_SEAT_CAPABILITY_ALL, TRUE, NULL, NULL, NULL, NULL);

    t0 = g_get_monotonic_time();
    g_timeout_add(16, tick, NULL);
    gtk_main();
    return 0;
}
