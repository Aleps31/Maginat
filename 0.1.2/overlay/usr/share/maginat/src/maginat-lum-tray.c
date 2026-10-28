#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static gint64 t_show;
static int is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
#define TR(en, it) (is_it() ? (it) : (en))
static GtkStatusIcon *icon;
static GtkWidget *win, *scale;
static int updating;

static int leggi(void)
{
    FILE *f = popen("maginat-lum get", "r");
    char r[64];
    int v = -1;
    if (!f) return -1;
    if (fgets(r, sizeof r, f)) v = atoi(r);
    pclose(f);
    return v;
}
static void aggiorna(void)
{
    int v = leggi();
    char t[64];
    if (v < 0) {
        gtk_status_icon_set_visible(icon, FALSE);
        return;
    }
    gtk_status_icon_set_visible(icon, TRUE);
    snprintf(t, sizeof t, "%s: %d%%", TR("Brightness", "Luminosità"), v);
    gtk_status_icon_set_from_icon_name(icon, "maginat-luminosita");
    gtk_status_icon_set_tooltip_text(icon, t);
    updating = 1;
    gtk_range_set_value(GTK_RANGE(scale), v);
    updating = 0;
}
static void imposta(int v)
{
    char c[80];
    if (v < 5) v = 5;
    if (v > 100) v = 100;
    snprintf(c, sizeof c, "maginat-lum set %d", v);
    system(c);
}
static void cambiato(GtkRange *r, gpointer d)
{
    if (updating) return;
    imposta((int)gtk_range_get_value(r));
    aggiorna();
}
static gchar *formato(GtkScale *s, gdouble v, gpointer d)
{
    return g_strdup_printf("%d%%", (int)v);
}
static gboolean perso(GtkWidget *w, GdkEvent *e, gpointer d)
{
    if (g_get_monotonic_time() - t_show < 400000) return FALSE;
    gtk_widget_hide(w);
    return FALSE;
}
static gboolean scorri(GtkStatusIcon *s, GdkEventScroll *e, gpointer d)
{
    int v = leggi();
    if (v < 0) return TRUE;
    imposta(e->direction == GDK_SCROLL_UP ? v + 5 : v - 5);
    aggiorna();
    return TRUE;
}
static gboolean timer(gpointer d)
{
    aggiorna();
    return TRUE;
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
    t_show = g_get_monotonic_time();
}
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
    scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5, 100, 1);
    gtk_container_add(GTK_CONTAINER(win), scale);
    g_signal_connect(scale, "value-changed", G_CALLBACK(cambiato), NULL);
    g_signal_connect(scale, "format-value", G_CALLBACK(formato), NULL);
    g_signal_connect(win, "focus-out-event", G_CALLBACK(perso), NULL);
    g_signal_connect(icon, "activate", G_CALLBACK(mostra), NULL);
    g_signal_connect(icon, "scroll-event", G_CALLBACK(scorri), NULL);
    aggiorna();
    g_timeout_add_seconds(5, timer, NULL);
    gtk_main();
    return 0;
}
