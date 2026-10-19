#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
#define TR(en, it) (is_it() ? (it) : (en))
static GtkStatusIcon *icon;
static GtkWidget *win, *scale;
static int updating;

static int leggi(int *muto)
{
    FILE *f = popen("maginat-vol get", "r");
    char r[256];
    int v = -1;
    *muto = 0;
    if (!f) return 0;
    while (fgets(r, sizeof r, f)) {
        char *p = strchr(r, '[');
        if (p && v < 0) v = atoi(p + 1);
        if (strstr(r, "[off]")) *muto = 1;
    }
    pclose(f);
    return v < 0 ? 0 : v;
}
static void aggiorna(void)
{
    int m, v = leggi(&m);
    const char *n;
    char t[64];
    if (m || v == 0) n = "audio-volume-muted";
    else if (v < 34) n = "audio-volume-low";
    else if (v < 67) n = "audio-volume-medium";
    else n = "audio-volume-high";
    snprintf(t, sizeof t, "Volume: %d%%%s", v, m ? TR(" (muted)", " (muto)") : "");
    gtk_status_icon_set_from_icon_name(icon, n);
    gtk_status_icon_set_tooltip_text(icon, t);
    updating = 1;
    gtk_range_set_value(GTK_RANGE(scale), v);
    updating = 0;
}
static void imposta(int v)
{
    char c[80];
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    snprintf(c, sizeof c, "maginat-vol set %d", v);
    system(c);
}

static void cambiato(GtkRange *r, gpointer d)
{
    if (updating) return;
    imposta((int)gtk_range_get_value(r));
    aggiorna();
}
static void mostra(GtkStatusIcon *s, gpointer d)
{
    GdkRectangle a;
    GdkScreen *sc;
    GtkOrientation o;
    int w, h, x;
    if (gtk_widget_get_visible(win)) {
        gtk_widget_hide(win);
        return;
    }
    aggiorna();
    gtk_widget_show_all(win);
    if (gtk_status_icon_get_geometry(s, &sc, &a, &o)) {
        gtk_window_get_size(GTK_WINDOW(win), &w, &h);
        x = a.x + a.width / 2 - w / 2;
        if (x + w > gdk_screen_get_width(sc)) x = gdk_screen_get_width(sc) - w;
        if (x < 0) x = 0;
        gtk_window_move(GTK_WINDOW(win), x, a.y + a.height + 2);
    }
    gtk_window_present(GTK_WINDOW(win));
}
#include "maginat-extra.h"
int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    icon = gtk_status_icon_new();
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 240, 50);
    gtk_container_set_border_width(GTK_CONTAINER(win), 8);
    scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_container_add(GTK_CONTAINER(win), scale);
    g_signal_connect(scale, "value-changed", G_CALLBACK(cambiato), NULL);
    g_signal_connect(scale, "format-value", G_CALLBACK(formato), NULL);
    g_signal_connect(win, "focus-out-event", G_CALLBACK(perso), NULL);
    g_signal_connect(icon, "activate", G_CALLBACK(mostra), NULL);
    g_signal_connect(icon, "scroll-event", G_CALLBACK(scorri), NULL);
    g_signal_connect(icon, "popup-menu", G_CALLBACK(destro), NULL);
    aggiorna();
    g_timeout_add_seconds(5, timer, NULL);
    gtk_main();
    return 0;
}
