/* maginat-battery-tray: battery icon for Maginat (GTK3, no icon theme needed)
 * License: GPL-2.0-or-later
 * Build: gcc -O2 -o maginat-battery-tray maginat-battery-tray.c $(pkg-config --cflags --libs gtk+-3.0)
 */
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static int is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
#define TR(en, it) (is_it() ? (it) : (en))

static GtkStatusIcon *icona;

static int leggi_file(const char *dir, const char *nome, char *out, size_t n)
{
    char p[400];
    FILE *f;
    snprintf(p, sizeof p, "/sys/class/power_supply/%s/%s", dir, nome);
    f = fopen(p, "r");
    if (!f) return 0;
    if (!fgets(out, (int)n, f)) { fclose(f); return 0; }
    fclose(f);
    out[strcspn(out, "\n")] = 0;
    return 1;
}

/* returns 1 if a system battery exists */
static int leggi(int *pct, char *stato, size_t n)
{
    DIR *d = opendir("/sys/class/power_supply");
    struct dirent *e;
    int trovata = 0;
    if (!d) return 0;
    while (!trovata && (e = readdir(d))) {
        char t[64], sc[64], cap[64];
        if (e->d_name[0] == '.') continue;
        if (!leggi_file(e->d_name, "type", t, sizeof t)) continue;
        if (strcmp(t, "Battery")) continue;
        if (leggi_file(e->d_name, "scope", sc, sizeof sc) && !strcmp(sc, "Device")) continue;
        if (!leggi_file(e->d_name, "capacity", cap, sizeof cap)) continue;
        *pct = atoi(cap);
        if (*pct < 0) *pct = 0;
        if (*pct > 100) *pct = 100;
        if (!leggi_file(e->d_name, "status", stato, n)) stato[0] = 0;
        trovata = 1;
    }
    closedir(d);
    return trovata;
}

static GdkPixbuf *disegna(int pct, int carica)
{
    const int S = 32;
    cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, S, S);
    cairo_t *c = cairo_create(sf);
    GdkPixbuf *pb;
    double r, g, b;

    if (pct <= 15) { r = 0.85; g = 0.15; b = 0.15; }
    else if (pct <= 30) { r = 0.95; g = 0.60; b = 0.10; }
    else { r = 0.15; g = 0.70; b = 0.25; }

    cairo_set_source_rgb(c, 0.25, 0.25, 0.25);
    cairo_set_line_width(c, 2.0);
    cairo_rectangle(c, 3, 9, 24, 16);
    cairo_stroke(c);
    cairo_rectangle(c, 27, 13, 3, 8);
    cairo_fill(c);

    cairo_set_source_rgb(c, r, g, b);
    cairo_rectangle(c, 5, 11, 20.0 * pct / 100.0, 12);
    cairo_fill(c);

    if (carica) {
        cairo_set_source_rgb(c, 1.0, 1.0, 0.85);
        cairo_move_to(c, 16, 7);
        cairo_line_to(c, 10, 18);
        cairo_line_to(c, 15, 18);
        cairo_line_to(c, 13, 27);
        cairo_line_to(c, 21, 15);
        cairo_line_to(c, 16, 15);
        cairo_close_path(c);
        cairo_fill_preserve(c);
        cairo_set_source_rgb(c, 0.25, 0.25, 0.25);
        cairo_set_line_width(c, 1.0);
        cairo_stroke(c);
    }
    pb = gdk_pixbuf_get_from_surface(sf, 0, 0, S, S);
    cairo_destroy(c);
    cairo_surface_destroy(sf);
    return pb;
}

static gboolean tick(gpointer d)
{
    int pct = 0, carica;
    char stato[64], tip[160];
    const char *s;
    GdkPixbuf *pb;
    (void)d;

    if (!leggi(&pct, stato, sizeof stato)) {
        gtk_status_icon_set_visible(icona, FALSE);
        return TRUE;
    }
    carica = !strcmp(stato, "Charging");
    if (carica) s = TR("charging", "in carica");
    else if (!strcmp(stato, "Discharging")) s = TR("on battery", "a batteria");
    else if (!strcmp(stato, "Full")) s = TR("full", "carica");
    else s = TR("not charging", "non in carica");
    snprintf(tip, sizeof tip, "%s: %d%% (%s)", TR("Battery", "Batteria"), pct, s);
    pb = disegna(pct, carica);
    gtk_status_icon_set_from_pixbuf(icona, pb);
    g_object_unref(pb);
    gtk_status_icon_set_tooltip_text(icona, tip);
    gtk_status_icon_set_visible(icona, TRUE);
    return TRUE;
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    icona = gtk_status_icon_new();
    gtk_status_icon_set_visible(icona, FALSE);
    tick(NULL);
    g_timeout_add_seconds(20, tick, NULL);
    gtk_main();
    return 0;
}
