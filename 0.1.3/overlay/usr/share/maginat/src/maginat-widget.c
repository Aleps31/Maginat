/* maginat-widget: widget da desktop per Maginat (orologio e calendario, meteo, monitor, note)
 * Licenza: GPL-2.0-or-later
 * Fa parte di maginat-tray: wid_main() viene chiamata da maginat-tray-main.c
 * Il meteo resta spento finche l'utente non sceglie una citta (usa open-meteo.com)
 */
#include <math.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/statvfs.h>

static int is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
static int is_es(void) { const char *l = getenv("LANG"); return l && l[0] == 'e' && l[1] == 's'; }
#define TR(en, it, es) (is_it() ? (it) : (is_es() ? (es) : (en)))

#define LARG 260
#define H_CAL 250
#define H_MET 120
#define H_MON 124
#define H_NOTE 150
#define H_OROL 100
#define H_BAT 100
#define H_LINK 152

static GtkWidget *w_cal, *w_met, *w_mon, *w_note, *w_orol, *w_bat, *w_link;
static GtkWidget *a_cal, *a_met, *a_mon, *a_orol, *a_bat, *grid_link;
static GtkTextBuffer *buf_nota;
static guint t_salva;

/* ---------- disegno ---------- */

static int g_comp;
#define M_OMB (g_comp ? 12 : 0)
#define R_ANG 14
#define CX(i) (22 + (i) * (LARG - 44) / 6.0)
static const double CS[8][2] = {{1, 0}, {0.7071, 0.7071}, {0, 1}, {-0.7071, 0.7071}, {-1, 0}, {-0.7071, -0.7071}, {0, -1}, {0.7071, -0.7071}};

static void rrett(cairo_t *c, double x, double y, double w, double h, double r);

static void sfondo(cairo_t *c, int w, int h)
{
    int i;
    if (g_comp) {
        cairo_save(c);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(c, 0, 0, 0, 0);
        cairo_paint(c);
        cairo_restore(c);
        for (i = 9; i >= 1; i--) {
            rrett(c, 12 - i, 12 - i + 3, w + 2 * i, h + 2 * i, R_ANG + i);
            cairo_set_source_rgba(c, 0, 0, 0, 0.032);
            cairo_fill(c);
        }
        cairo_translate(c, 12, 12);
        cairo_save(c);
        cairo_set_operator(c, CAIRO_OPERATOR_CLEAR);
        rrett(c, 0.5, 0.5, w - 1, h - 1, R_ANG);
        cairo_fill(c);
        cairo_restore(c);
    }
    rrett(c, 0.5, 0.5, w - 1, h - 1, R_ANG);
    if (g_comp) cairo_set_source_rgba(c, 0.929, 0.949, 0.980, 0.88);
    else cairo_set_source_rgb(c, 0.929, 0.949, 0.980);
    cairo_fill_preserve(c);
    if (g_comp) cairo_set_source_rgba(c, 0.765, 0.804, 0.863, 0.95);
    else cairo_set_source_rgb(c, 0.765, 0.804, 0.863);
    cairo_set_line_width(c, 1);
    cairo_stroke(c);
}

static void testo(cairo_t *c, double x, double y, double sz, int bold, const char *s,
                  double r, double g, double b, int align)
{
    cairo_text_extents_t e;
    double px = x;
    cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL,
        bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(c, sz);
    cairo_text_extents(c, s, &e);
    if (align == 1) px = x - e.width / 2 - e.x_bearing;
    else if (align == 2) px = x - e.width - e.x_bearing;
    cairo_set_source_rgb(c, r, g, b);
    cairo_move_to(c, px, y);
    cairo_show_text(c, s);
}

static gboolean fondo_finestra(GtkWidget *w, cairo_t *c, gpointer d)
{
    int h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "h"));
    (void)d;
    cairo_save(c);
    sfondo(c, LARG, h);
    cairo_restore(c);
    return FALSE;
}

static void forma_finestra(GtkWidget *w, gpointer d)
{
    int h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "h"));
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_A1, LARG, h);
    cairo_t *c = cairo_create(s);
    cairo_region_t *r;
    (void)d;
    rrett(c, 0, 0, LARG, h, R_ANG);
    cairo_fill(c);
    cairo_destroy(c);
    r = gdk_cairo_region_create_from_surface(s);
    gtk_widget_shape_combine_region(w, r);
    cairo_region_destroy(r);
    cairo_surface_destroy(s);
}

static GtkWidget *nuova(int h, int *y, int focus)
{
    GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GdkScreen *sc = gdk_screen_get_default();
    gtk_window_set_decorated(GTK_WINDOW(w), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(w), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(w), TRUE);
    gtk_window_set_keep_below(GTK_WINDOW(w), TRUE);
    gtk_window_stick(GTK_WINDOW(w));
    gtk_window_set_accept_focus(GTK_WINDOW(w), focus);
    gtk_window_set_resizable(GTK_WINDOW(w), FALSE);
    gtk_widget_set_size_request(w, LARG + 2 * M_OMB, h + 2 * M_OMB);
    gtk_window_move(GTK_WINDOW(w), gdk_screen_get_width(sc) - LARG - 16 - M_OMB, *y - M_OMB);
    g_object_set_data(G_OBJECT(w), "h", GINT_TO_POINTER(h));
    gtk_widget_set_app_paintable(w, TRUE);
    if (g_comp) gtk_widget_set_visual(w, gdk_screen_get_rgba_visual(sc));
    else g_signal_connect(w, "realize", G_CALLBACK(forma_finestra), NULL);
    *y += h + 12;
    return w;
}

/* ---------- orologio e calendario ---------- */

static gboolean disegna_cal(GtkWidget *wd, cairo_t *c, gpointer d)
{
    static const char *gi_it[] = {"domenica","lunedì","martedì","mercoledì","giovedì","venerdì","sabato"};
    static const char *gi_en[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    static const char *gi_es[] = {"domingo","lunes","martes","miércoles","jueves","viernes","sábado"};
    static const char *me_it[] = {"gennaio","febbraio","marzo","aprile","maggio","giugno","luglio","agosto","settembre","ottobre","novembre","dicembre"};
    static const char *me_en[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
    static const char *me_es[] = {"enero","febrero","marzo","abril","mayo","junio","julio","agosto","septiembre","octubre","noviembre","diciembre"};
    static const char *in_it[] = {"L","M","M","G","V","S","D"};
    static const char *in_en[] = {"M","T","W","T","F","S","S"};
    static const char *in_es[] = {"L","M","X","J","V","S","D"};
    static const int gm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    time_t t = time(NULL);
    struct tm tm, p;
    char s[96];
    int i, off, nd, riga, col, anno;

    (void)wd; (void)d;
    localtime_r(&t, &tm);
    sfondo(c, LARG, H_CAL);
    snprintf(s, sizeof s, is_es() ? "%s %d de %s" : "%s %d %s", TR(gi_en, gi_it, gi_es)[tm.tm_wday], tm.tm_mday, TR(me_en, me_it, me_es)[tm.tm_mon]);
    {
        cairo_text_extents_t ex;
        double sz = 21;
        cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        for (;;) {
            cairo_set_font_size(c, sz);
            cairo_text_extents(c, s, &ex);
            if (ex.width <= LARG - 36 || sz <= 11) break;
            sz -= 0.5;
        }
        testo(c, LARG / 2.0, 66, sz, 1, s, 0.10, 0.42, 0.82, 1);
    }
    snprintf(s, sizeof s, "%d", tm.tm_year + 1900);
    testo(c, LARG / 2.0, 92, 14, 0, s, 0.36, 0.43, 0.52, 1);

    for (i = 0; i < 7; i++)
        testo(c, CX(i), 124, 11, 1, TR(in_en, in_it, in_es)[i], 0.36, 0.43, 0.52, 1);
    p = tm;
    p.tm_mday = 1;
    mktime(&p);
    off = (p.tm_wday + 6) % 7;
    anno = tm.tm_year + 1900;
    nd = gm[tm.tm_mon];
    if (tm.tm_mon == 1 && ((anno % 4 == 0 && anno % 100 != 0) || anno % 400 == 0)) nd = 29;
    for (i = 1; i <= nd; i++) {
        col = (off + i - 1) % 7;
        riga = (off + i - 1) / 7;
        if (i == tm.tm_mday) {
            cairo_set_source_rgb(c, 0.10, 0.42, 0.82);
            cairo_arc(c, CX(col), 142 + riga * 18 - 4, 9, 0, 6.2832);
            cairo_fill(c);
            snprintf(s, sizeof s, "%d", i);
            testo(c, CX(col), 142 + riga * 18, 11, 1, s, 1, 1, 1, 1);
        } else {
            snprintf(s, sizeof s, "%d", i);
            testo(c, CX(col), 142 + riga * 18, 11, 0, s,
                  col >= 5 ? 0.78 : 0.13, col >= 5 ? 0.35 : 0.17, col >= 5 ? 0.12 : 0.22, 1);
        }
    }
    return TRUE;
}

static gboolean tick_cal(gpointer d)
{
    (void)d;
    if (gtk_widget_get_visible(w_cal)) gtk_widget_queue_draw(a_cal);
    if (gtk_widget_get_visible(w_orol)) gtk_widget_queue_draw(a_orol);
    return TRUE;
}

/* ---------- monitor di sistema ---------- */

static double cpu_pct, ram_pct, disk_pct, rx_s, tx_s;
static unsigned long long cpu_t0, cpu_i0, rx0, tx0;
static gint64 net_t0;

static void leggi_cpu(void)
{
    FILE *f = fopen("/proc/stat", "r");
    unsigned long long u, n, s, id, io, ir, so, st, tot, idl;
    if (!f) return;
    if (fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &u, &n, &s, &id, &io, &ir, &so, &st) == 8) {
        tot = u + n + s + id + io + ir + so + st;
        idl = id + io;
        if (cpu_t0 && tot > cpu_t0)
            cpu_pct = 100.0 * (double)((tot - cpu_t0) - (idl - cpu_i0)) / (double)(tot - cpu_t0);
        cpu_t0 = tot;
        cpu_i0 = idl;
    }
    fclose(f);
}

static void leggi_ram(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char r[128];
    unsigned long long tot = 0, av = 0, v;
    if (!f) return;
    while (fgets(r, sizeof r, f)) {
        if (sscanf(r, "MemTotal: %llu", &v) == 1) tot = v;
        else if (sscanf(r, "MemAvailable: %llu", &v) == 1) av = v;
    }
    fclose(f);
    if (tot) ram_pct = 100.0 * (double)(tot - av) / (double)tot;
}

static void leggi_disco(void)
{
    struct statvfs sv;
    if (statvfs("/", &sv) == 0 && sv.f_blocks)
        disk_pct = 100.0 * (double)(sv.f_blocks - sv.f_bavail) / (double)sv.f_blocks;
}

static void leggi_rete(void)
{
    FILE *f = fopen("/proc/net/dev", "r");
    char r[512];
    unsigned long long rb, tb, x[7], rx = 0, tx = 0;
    gint64 ora = g_get_monotonic_time();
    if (!f) return;
    while (fgets(r, sizeof r, f)) {
        char *p = strchr(r, ':');
        if (!p) continue;
        if (strstr(r, "lo:")) continue;
        if (sscanf(p + 1, "%llu %llu %llu %llu %llu %llu %llu %llu %llu", &rb, &x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &x[6], &tb) == 9) {
            rx += rb;
            tx += tb;
        }
    }
    fclose(f);
    if (net_t0 && ora > net_t0 && rx >= rx0 && tx >= tx0) {
        double dt = (double)(ora - net_t0) / 1e6;
        rx_s = (double)(rx - rx0) / dt;
        tx_s = (double)(tx - tx0) / dt;
    }
    rx0 = rx;
    tx0 = tx;
    net_t0 = ora;
}

static void fmt_rate(double b, char *o, size_t n)
{
    if (b < 1024) snprintf(o, n, "%.0f B/s", b);
    else if (b < 1048576) snprintf(o, n, "%.0f KB/s", b / 1024);
    else snprintf(o, n, "%.1f MB/s", b / 1048576);
}

static void barra(cairo_t *c, double y, const char *nome, double pct, double r, double g, double b)
{
    char s[16];
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    testo(c, 12, y, 12, 0, nome, 0.22, 0.28, 0.36, 0);
    cairo_set_source_rgba(c, 0.13, 0.17, 0.22, 0.13);
    rrett(c, 62, y - 10, LARG - 122, 10, 5);
    cairo_fill(c);
    if (pct > 0) {
        double bw = (LARG - 122) * pct / 100.0;
        if (bw < 10) bw = 10;
        cairo_set_source_rgb(c, r, g, b);
        rrett(c, 62, y - 10, bw, 10, 5);
        cairo_fill(c);
    }
    snprintf(s, sizeof s, "%.0f%%", pct);
    testo(c, LARG - 12, y, 12, 1, s, 0.13, 0.17, 0.22, 2);
}

static gboolean disegna_mon(GtkWidget *wd, cairo_t *c, gpointer d)
{
    char a[40], b[40], s[96];
    (void)wd; (void)d;
    sfondo(c, LARG, H_MON);
    testo(c, 12, 20, 11, 1, TR("System", "Sistema", "Sistema"), 0.36, 0.43, 0.52, 0);
    barra(c, 42, "CPU", cpu_pct, 0.10, 0.42, 0.82);
    barra(c, 64, "RAM", ram_pct, 0.22, 0.65, 0.36);
    barra(c, 86, TR("Disk", "Disco", "Disco"), disk_pct, 0.80, 0.45, 0.10);
    fmt_rate(rx_s, a, sizeof a);
    fmt_rate(tx_s, b, sizeof b);
    snprintf(s, sizeof s, "↓ %s   ↑ %s", a, b);
    testo(c, 12, 111, 12, 0, s, 0.22, 0.28, 0.36, 0);
    return TRUE;
}

static gboolean tick_mon(gpointer d)
{
    (void)d;
    if (d == NULL && w_mon && !gtk_widget_get_visible(w_mon)) return TRUE;
    leggi_cpu();
    leggi_ram();
    leggi_disco();
    leggi_rete();
    gtk_widget_queue_draw(a_mon);
    return TRUE;
}

/* ---------- meteo (spento finche l'utente non sceglie una citta) ---------- */

static char m_citta[64], m_msg[96];
static char m_lat[24], m_lon[24];
static double m_temp;
static int m_conf, m_ok, m_code;

static char *file_meteo(void)
{
    return g_build_filename(g_get_user_config_dir(), "maginat", "meteo.conf", NULL);
}

static const char *descrizione(int c)
{
    if (c == 0) return TR("Clear", "Sereno", "Despejado");
    if (c == 1) return TR("Mostly clear", "Prevalentemente sereno", "Mayormente despejado");
    if (c == 2) return TR("Partly cloudy", "Parzialmente nuvoloso", "Parcialmente nublado");
    if (c == 3) return TR("Overcast", "Coperto", "Cubierto");
    if (c == 45 || c == 48) return TR("Fog", "Nebbia", "Niebla");
    if (c >= 51 && c <= 57) return TR("Drizzle", "Pioggerella", "Llovizna");
    if (c >= 61 && c <= 67) return TR("Rain", "Pioggia", "Lluvia");
    if (c >= 71 && c <= 77) return TR("Snow", "Neve", "Nieve");
    if (c >= 80 && c <= 82) return TR("Showers", "Rovesci", "Chubascos");
    if (c == 85 || c == 86) return TR("Snow showers", "Rovesci di neve", "Chubascos de nieve");
    if (c >= 95) return TR("Thunderstorm", "Temporale", "Tormenta");
    return TR("Unknown", "Variabile", "Variable");
}

static void nuvola(cairo_t *c, double x, double y, double a)
{
    cairo_set_source_rgba(c, 0.60, 0.69, 0.82, a);
    cairo_arc(c, x, y, 11, 0, 6.2832);
    cairo_fill(c);
    cairo_arc(c, x + 14, y - 6, 15, 0, 6.2832);
    cairo_fill(c);
    cairo_arc(c, x + 29, y + 1, 10, 0, 6.2832);
    cairo_fill(c);
    cairo_rectangle(c, x, y, 29, 11);
    cairo_fill(c);
}

static void icona_meteo(cairo_t *c, double cx, double cy, int code, double a)
{
    int k;
    int sole = code <= 1, parz = code == 2;
    int pioggia = (code >= 51 && code <= 67) || (code >= 80 && code <= 82);
    int neve = (code >= 71 && code <= 77) || code == 85 || code == 86;
    int nebbia = code == 45 || code == 48, tuono = code >= 95;
    cairo_save(c);
    cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
    if (sole || parz) {
        double sx = parz ? cx - 8 : cx, sy = parz ? cy - 8 : cy, r = parz ? 14 : 17;
        cairo_set_source_rgba(c, 1, 0.72, 0.12, a);
        cairo_arc(c, sx, sy, r, 0, 6.2832);
        cairo_fill(c);
        cairo_set_line_width(c, 3);
        for (k = 0; k < 8; k++) {
            cairo_move_to(c, sx + CS[k][0] * (r + 6), sy + CS[k][1] * (r + 6));
            cairo_line_to(c, sx + CS[k][0] * (r + 12), sy + CS[k][1] * (r + 12));
            cairo_stroke(c);
        }
    }
    if (parz) nuvola(c, cx - 2, cy + 14, a);
    if (!sole && !parz) {
        double x = cx - 14, y = cy - 4;
        nuvola(c, x, y, a);
        cairo_set_line_width(c, 2.6);
        if (pioggia) {
            cairo_set_source_rgba(c, 0.18, 0.48, 0.88, a);
            for (k = 0; k < 3; k++) {
                cairo_move_to(c, x + 4 + k * 10, y + 16);
                cairo_line_to(c, x + 1 + k * 10, y + 24);
                cairo_stroke(c);
            }
        } else if (neve) {
            cairo_set_source_rgba(c, 0.45, 0.58, 0.80, a);
            for (k = 0; k < 3; k++) {
                cairo_arc(c, x + 3 + k * 11, y + 20, 2.2, 0, 6.2832);
                cairo_fill(c);
            }
        } else if (nebbia) {
            cairo_set_source_rgba(c, 0.58, 0.66, 0.78, a);
            for (k = 0; k < 2; k++) {
                cairo_move_to(c, x - 6, y + 17 + k * 7);
                cairo_line_to(c, x + 34, y + 17 + k * 7);
                cairo_stroke(c);
            }
        } else if (tuono) {
            cairo_set_source_rgba(c, 1, 0.72, 0.12, a);
            cairo_move_to(c, x + 17, y + 12);
            cairo_line_to(c, x + 9, y + 22);
            cairo_line_to(c, x + 15, y + 22);
            cairo_line_to(c, x + 11, y + 31);
            cairo_line_to(c, x + 23, y + 18);
            cairo_line_to(c, x + 16, y + 18);
            cairo_close_path(c);
            cairo_fill(c);
        }
    }
    cairo_restore(c);
}

static gboolean disegna_met(GtkWidget *wd, cairo_t *c, gpointer d)
{
    char s[64];
    (void)wd; (void)d;
    sfondo(c, LARG, H_MET);
    if (!m_conf) {
        icona_meteo(c, 50, 62, 3, 0.45);
        testo(c, 100, 56, 16, 1, TR("Weather", "Meteo", "El tiempo"), 0.13, 0.17, 0.22, 0);
        testo(c, 100, 80, 12, 0, m_msg[0] ? m_msg : TR("Click to turn on", "Clic per attivare", "Clic para activar"), 0.36, 0.43, 0.52, 0);
    } else if (!m_ok) {
        icona_meteo(c, 50, 62, 3, 0.45);
        testo(c, 100, 56, 15, 1, m_citta, 0.13, 0.17, 0.22, 0);
        testo(c, 100, 80, 12, 0, m_msg[0] ? m_msg : TR("Loading...", "Caricamento...", "Cargando..."), 0.36, 0.43, 0.52, 0);
    } else {
        testo(c, 16, 26, 12, 1, m_citta, 0.30, 0.36, 0.45, 0);
        icona_meteo(c, 52, 64, m_code, 1.0);
        snprintf(s, sizeof s, "%.0f°", m_temp);
        testo(c, 106, 78, 40, 1, s, 0.13, 0.17, 0.22, 0);
        testo(c, 16, H_MET - 14, 12, 0, descrizione(m_code), 0.10, 0.42, 0.82, 0);
    }
    return TRUE;
}

static void meteo_scarica(void);

static char *cerca(const char *buf, const char *chiave)
{
    const char *p = strstr(buf, chiave);
    return p ? (char *)p + strlen(chiave) : NULL;
}

static void fine_meteo(GObject *src, GAsyncResult *res, gpointer d)
{
    char *out = NULL;
    GError *e = NULL;
    char *p, *q;
    (void)d;
    g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &e);
    g_clear_error(&e);
    g_object_unref(src);
    m_ok = 0;
    if (out && (p = cerca(out, "\"current\":{")) != NULL) {
        char *t = cerca(p, "\"temperature_2m\":");
        char *cd = cerca(p, "\"weather_code\":");
        if (t && cd) {
            m_temp = strtod(t, NULL);
            m_code = atoi(cd);
            m_ok = 1;
            m_msg[0] = 0;
        }
    }
    if (!m_ok) g_strlcpy(m_msg, TR("Not available", "Non disponibile", "No disponible"), sizeof m_msg);
    (void)q;
    g_free(out);
    gtk_widget_queue_draw(a_met);
}

static void lancia(const char *url, GAsyncReadyCallback cb)
{
    GError *e = NULL;
    GSubprocess *p = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE, &e,
        "curl", "-s", "--max-time", "10", url, NULL);
    if (!p) {
        g_clear_error(&e);
        g_strlcpy(m_msg, TR("Not available", "Non disponibile", "No disponible"), sizeof m_msg);
        gtk_widget_queue_draw(a_met);
        return;
    }
    g_subprocess_communicate_utf8_async(p, NULL, NULL, cb, NULL);
}

static void meteo_scarica(void)
{
    char url[300];
    if (!m_conf) return;
    snprintf(url, sizeof url,
        "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,weather_code&timezone=auto",
        m_lat, m_lon);
    lancia(url, fine_meteo);
}

static gboolean tick_meteo(gpointer d)
{
    (void)d;
    if (!gtk_widget_get_visible(w_met)) return TRUE;
    meteo_scarica();
    return TRUE;
}

static void fine_geo(GObject *src, GAsyncResult *res, gpointer d)
{
    char *out = NULL;
    GError *e = NULL;
    char *la, *lo, *nm;
    (void)d;
    g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &e);
    g_clear_error(&e);
    g_object_unref(src);
    la = out ? cerca(out, "\"latitude\":") : NULL;
    lo = out ? cerca(out, "\"longitude\":") : NULL;
    nm = out ? cerca(out, "\"name\":\"") : NULL;
    if (la && lo && nm) {
        char *fine = strchr(nm, '"');
        char *cf;
        FILE *f;
        snprintf(m_lat, sizeof m_lat, "%.4f", strtod(la, NULL));
        snprintf(m_lon, sizeof m_lon, "%.4f", strtod(lo, NULL));
        if (fine) {
            size_t n = (size_t)(fine - nm);
            if (n >= sizeof m_citta) n = sizeof m_citta - 1;
            memcpy(m_citta, nm, n);
            m_citta[n] = 0;
        }
        m_conf = 1;
        m_msg[0] = 0;
        cf = file_meteo();
        g_mkdir_with_parents(g_path_get_dirname(cf), 0700);
        f = fopen(cf, "w");
        if (f) { fprintf(f, "%s;%s;%s\n", m_lat, m_lon, m_citta); fclose(f); }
        g_free(cf);
        meteo_scarica();
    } else {
        g_strlcpy(m_msg, TR("City not found", "Città non trovata", "Ciudad no encontrada"), sizeof m_msg);
    }
    g_free(out);
    gtk_widget_queue_draw(a_met);
}

static void meteo_configura(void)
{
    GtkWidget *dl, *en, *lb, *ar;
    char url[400];
    char *esc;
    dl = gtk_dialog_new_with_buttons(TR("Weather", "Meteo", "El tiempo"), NULL, 0,
        TR("Cancel", "Annulla", "Cancelar"), GTK_RESPONSE_CANCEL, "OK", GTK_RESPONSE_OK, NULL);
    gtk_window_set_keep_above(GTK_WINDOW(dl), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(dl), 8);
    ar = gtk_dialog_get_content_area(GTK_DIALOG(dl));
    lb = gtk_label_new(TR("City name. The weather uses open-meteo.com: the city name and your IP address will be sent to that service.",
                          "Nome della città. Il meteo usa open-meteo.com: il nome della città e il tuo indirizzo IP saranno inviati a quel servizio.", "Nombre de la ciudad. El tiempo usa open-meteo.com: el nombre de la ciudad y tu dirección IP se enviarán a ese servicio."));
    gtk_label_set_line_wrap(GTK_LABEL(lb), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(lb), 40);
    en = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(en), TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(dl), GTK_RESPONSE_OK);
    gtk_box_pack_start(GTK_BOX(ar), lb, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(ar), en, FALSE, FALSE, 4);
    gtk_widget_show_all(dl);
    if (gtk_dialog_run(GTK_DIALOG(dl)) == GTK_RESPONSE_OK && gtk_entry_get_text(GTK_ENTRY(en))[0]) {
        esc = g_uri_escape_string(gtk_entry_get_text(GTK_ENTRY(en)), NULL, FALSE);
        snprintf(url, sizeof url,
            "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=%s&format=json",
            esc, is_it() ? "it" : (is_es() ? "es" : "en"));
        g_free(esc);
        g_strlcpy(m_msg, TR("Searching...", "Ricerca...", "Buscando..."), sizeof m_msg);
        gtk_widget_queue_draw(a_met);
        lancia(url, fine_geo);
    }
    gtk_widget_destroy(dl);
}

static gboolean clic_meteo(GtkWidget *wd, GdkEventButton *e, gpointer d)
{
    (void)wd; (void)e; (void)d;
    if (!m_conf) meteo_configura();
    else {
        g_strlcpy(m_msg, TR("Loading...", "Caricamento...", "Cargando..."), sizeof m_msg);
        meteo_scarica();
    }
    return TRUE;
}

static void carica_meteo(void)
{
    char *cf = file_meteo();
    char *dati = NULL;
    if (g_file_get_contents(cf, &dati, NULL, NULL)) {
        char **v = g_strsplit(g_strstrip(dati), ";", 3);
        if (v[0] && v[1] && v[2]) {
            g_strlcpy(m_lat, v[0], sizeof m_lat);
            g_strlcpy(m_lon, v[1], sizeof m_lon);
            g_strlcpy(m_citta, v[2], sizeof m_citta);
            m_conf = 1;
        }
        g_strfreev(v);
        g_free(dati);
    }
    g_free(cf);
}

/* ---------- note veloci ---------- */

static char *file_nota(void)
{
    return g_build_filename(g_get_user_data_dir(), "maginat", "nota.txt", NULL);
}

static gboolean salva_nota(gpointer d)
{
    GtkTextIter a, b;
    char *t, *cf, *dir;
    (void)d;
    t_salva = 0;
    gtk_text_buffer_get_bounds(buf_nota, &a, &b);
    t = gtk_text_buffer_get_text(buf_nota, &a, &b, FALSE);
    cf = file_nota();
    dir = g_path_get_dirname(cf);
    g_mkdir_with_parents(dir, 0700);
    g_file_set_contents(cf, t, -1, NULL);
    g_free(dir);
    g_free(cf);
    g_free(t);
    return FALSE;
}

static void nota_cambiata(GtkTextBuffer *b, gpointer d)
{
    (void)b; (void)d;
    if (t_salva) g_source_remove(t_salva);
    t_salva = g_timeout_add_seconds(2, salva_nota, NULL);
}

/* ---------- orologio piccolo ---------- */

static gboolean disegna_orol(GtkWidget *wd, cairo_t *c, gpointer d)
{
    time_t t = time(NULL);
    struct tm tm;
    double cx = LARG / 2.0, cy = H_OROL / 2.0, R = 41, a, ca, sa, r1;
    int i;
    (void)wd; (void)d;
    localtime_r(&t, &tm);
    sfondo(c, LARG, H_OROL);
    /* quadrante */
    cairo_arc(c, cx, cy, R + 4, 0, 6.2832);
    cairo_set_source_rgb(c, 1, 1, 1);
    cairo_fill_preserve(c);
    cairo_set_source_rgb(c, 0.765, 0.804, 0.863);
    cairo_set_line_width(c, 1.5);
    cairo_stroke(c);
    /* trattini: 60, piu lunghi e spessi alle ore */
    cairo_set_line_cap(c, CAIRO_LINE_CAP_BUTT);
    for (i = 0; i < 60; i++) {
        a = i * 6.2832 / 60 - 1.5708;
        ca = cos(a); sa = sin(a);
        r1 = (i % 5 == 0) ? R - 9 : R - 4;
        cairo_set_line_width(c, (i % 5 == 0) ? 2.4 : 1);
        if (i % 5 == 0) cairo_set_source_rgb(c, 0.13, 0.17, 0.22);
        else cairo_set_source_rgb(c, 0.55, 0.60, 0.68);
        cairo_move_to(c, cx + ca * r1, cy + sa * r1);
        cairo_line_to(c, cx + ca * R, cy + sa * R);
        cairo_stroke(c);
    }
    cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
    /* ore */
    a = ((tm.tm_hour % 12) + tm.tm_min / 60.0) * 6.2832 / 12 - 1.5708;
    cairo_set_source_rgb(c, 0.13, 0.17, 0.22);
    cairo_set_line_width(c, 3.6);
    cairo_move_to(c, cx - cos(a) * 5, cy - sin(a) * 5);
    cairo_line_to(c, cx + cos(a) * (R - 18), cy + sin(a) * (R - 18));
    cairo_stroke(c);
    /* minuti */
    a = (tm.tm_min + tm.tm_sec / 60.0) * 6.2832 / 60 - 1.5708;
    cairo_set_line_width(c, 2.4);
    cairo_move_to(c, cx - cos(a) * 6, cy - sin(a) * 6);
    cairo_line_to(c, cx + cos(a) * (R - 8), cy + sin(a) * (R - 8));
    cairo_stroke(c);
    /* perno */
    cairo_arc(c, cx, cy, 3.4, 0, 6.2832);
    cairo_set_source_rgb(c, 0.10, 0.42, 0.82);
    cairo_fill(c);
    return TRUE;
}

/* ---------- batteria e rete ---------- */

static int b_pres, b_pct, b_rete;
static char b_stato[40], b_ssid[64];
static int b_cont;

static void leggi_batteria(void)
{
    GDir *d = g_dir_open("/sys/class/power_supply", 0, NULL);
    const char *n;
    b_pres = 0;
    b_stato[0] = 0;
    if (!d) return;
    while ((n = g_dir_read_name(d))) {
        if (strncmp(n, "BAT", 3) == 0) {
            char *pc = g_build_filename("/sys/class/power_supply", n, "capacity", NULL);
            char *ps = g_build_filename("/sys/class/power_supply", n, "status", NULL);
            char *v = NULL;
            if (g_file_get_contents(pc, &v, NULL, NULL)) {
                b_pct = atoi(v);
                b_pres = 1;
                g_free(v);
                v = NULL;
            }
            if (g_file_get_contents(ps, &v, NULL, NULL)) {
                g_strstrip(v);
                if (strcmp(v, "Charging") == 0) g_strlcpy(b_stato, TR("Charging", "In carica", "Cargando"), sizeof b_stato);
                else if (strcmp(v, "Discharging") == 0) g_strlcpy(b_stato, TR("On battery", "A batteria", "Con batería"), sizeof b_stato);
                else if (strcmp(v, "Full") == 0) g_strlcpy(b_stato, TR("Full", "Carica", "Completa"), sizeof b_stato);
                else if (strcmp(v, "Not charging") == 0) g_strlcpy(b_stato, TR("Plugged in", "Collegata", "Conectada"), sizeof b_stato);
                g_free(v);
            }
            g_free(pc);
            g_free(ps);
            if (b_pres) break;
        }
    }
    g_dir_close(d);
}

static void leggi_tipo_rete(void)
{
    GDir *d = g_dir_open("/sys/class/net", 0, NULL);
    const char *n;
    b_rete = 0;
    if (!d) return;
    while ((n = g_dir_read_name(d))) {
        char *po, *pw, *v = NULL;
        int su = 0, wl;
        if (strcmp(n, "lo") == 0 || strncmp(n, "docker", 6) == 0 || strncmp(n, "veth", 4) == 0 ||
            strncmp(n, "virbr", 5) == 0 || strncmp(n, "br-", 3) == 0) continue;
        po = g_build_filename("/sys/class/net", n, "operstate", NULL);
        pw = g_build_filename("/sys/class/net", n, "wireless", NULL);
        if (g_file_get_contents(po, &v, NULL, NULL)) {
            su = strncmp(v, "up", 2) == 0;
            g_free(v);
        }
        wl = g_file_test(pw, G_FILE_TEST_IS_DIR);
        if (su) {
            if (wl) b_rete = 2;
            else if (b_rete == 0) b_rete = 1;
        }
        g_free(po);
        g_free(pw);
    }
    g_dir_close(d);
}

static void fine_ssid(GObject *src, GAsyncResult *res, gpointer d)
{
    char *out = NULL;
    (void)d;
    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, NULL) && out) {
        g_strstrip(out);
        g_strlcpy(b_ssid, out, sizeof b_ssid);
    } else {
        b_ssid[0] = 0;
    }
    g_free(out);
    g_object_unref(src);
    gtk_widget_queue_draw(a_bat);
}

static void cerca_ssid(void)
{
    GSubprocess *p = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE, NULL,
        "sh", "-c", "iwgetid -r 2>/dev/null || nmcli -t -f active,ssid dev wifi 2>/dev/null | sed -n 's/^yes://p' | head -1", NULL);
    if (p) g_subprocess_communicate_utf8_async(p, NULL, NULL, fine_ssid, NULL);
}

static gboolean tick_bat(gpointer d)
{
    if (d == NULL && w_bat && !gtk_widget_get_visible(w_bat)) return TRUE;
    leggi_batteria();
    leggi_tipo_rete();
    if (b_rete == 2) {
        if (d != NULL || (b_cont++ % 3) == 0) cerca_ssid();
    } else {
        b_ssid[0] = 0;
    }
    gtk_widget_queue_draw(a_bat);
    return TRUE;
}

static gboolean disegna_bat(GtkWidget *wd, cairo_t *c, gpointer d)
{
    char s[120];
    (void)wd; (void)d;
    sfondo(c, LARG, H_BAT);
    testo(c, 12, 20, 11, 1, TR("Battery and network", "Batteria e rete", "Batería y red"), 0.36, 0.43, 0.52, 0);
    if (b_pres) {
        if (b_pct <= 20) barra(c, 44, TR("Battery", "Batteria", "Batería"), b_pct, 0.85, 0.25, 0.25);
        else barra(c, 44, TR("Battery", "Batteria", "Batería"), b_pct, 0.22, 0.65, 0.36);
        if (b_stato[0]) testo(c, 12, 62, 11, 0, b_stato, 0.36, 0.43, 0.52, 0);
    } else {
        testo(c, 12, 48, 12, 0, TR("Mains power", "Alimentazione di rete", "Alimentación de red"), 0.22, 0.28, 0.36, 0);
    }
    if (b_rete == 2) {
        if (b_ssid[0]) snprintf(s, sizeof s, "Wi-Fi: %s", b_ssid);
        else snprintf(s, sizeof s, "%s", TR("Wi-Fi: connected", "Wi-Fi: connesso", "Wi-Fi: conectado"));
        testo(c, 12, 86, 12, 0, s, 0.22, 0.28, 0.36, 0);
    } else if (b_rete == 1) {
        testo(c, 12, 86, 12, 0, TR("Wired: connected", "Cavo: connesso", "Cable: conectado"), 0.22, 0.28, 0.36, 0);
    } else {
        testo(c, 12, 86, 12, 0, TR("Network: not connected", "Rete: non connessa", "Red: sin conexión"), 0.80, 0.45, 0.10, 0);
    }
    return TRUE;
}

/* ---------- collegamenti rapidi (~/.config/maginat/collegamenti.conf, una riga: Nome;comando) ---------- */

static char *file_link(void)
{
    return g_build_filename(g_get_user_config_dir(), "maginat", "collegamenti.conf", NULL);
}

static gboolean cmd_ok(const char *cmd)
{
    gint ac;
    gchar **av = NULL;
    char *pth;
    gboolean ok;
    if (!g_shell_parse_argv(cmd, &ac, &av, NULL)) return FALSE;
    pth = g_find_program_in_path(av[0]);
    ok = pth != NULL;
    g_free(pth);
    g_strfreev(av);
    return ok;
}

static void clic_link(GtkButton *b, gpointer d)
{
    const char *cmd = g_object_get_data(G_OBJECT(b), "cmd");
    (void)d;
    if (cmd) g_spawn_command_line_async(cmd, NULL);
}

static gboolean disegna_icona_link(GtkWidget *wd, cairo_t *c, gpointer d)
{
    int k = GPOINTER_TO_INT(d), i;
    (void)wd;
    cairo_set_source_rgb(c, 0.17, 0.23, 0.31);
    cairo_set_line_width(c, 1.6);
    cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(c, CAIRO_LINE_JOIN_ROUND);
    if (k == 0) {
        rrett(c, 1, 2, 14, 12, 2.5);
        cairo_stroke(c);
        cairo_move_to(c, 4, 6); cairo_line_to(c, 7, 8.5); cairo_line_to(c, 4, 11);
        cairo_stroke(c);
        cairo_move_to(c, 8.5, 11.2); cairo_line_to(c, 12, 11.2);
        cairo_stroke(c);
    } else if (k == 1) {
        cairo_move_to(c, 1, 4); cairo_line_to(c, 6, 4); cairo_line_to(c, 8, 6); cairo_line_to(c, 15, 6);
        cairo_line_to(c, 15, 13); cairo_line_to(c, 1, 13); cairo_close_path(c);
        cairo_stroke(c);
    } else if (k == 2) {
        cairo_move_to(c, 3, 6); cairo_line_to(c, 13, 6); cairo_line_to(c, 12, 14); cairo_line_to(c, 4, 14); cairo_close_path(c);
        cairo_stroke(c);
        cairo_arc(c, 8, 6, 3.2, G_PI, 2 * G_PI);
        cairo_stroke(c);
    } else if (k == 3) {
        cairo_arc(c, 8, 8, 5.2, 0.3, 2 * G_PI - 0.9);
        cairo_stroke(c);
        cairo_move_to(c, 10.5, 1.5); cairo_line_to(c, 13.5, 3); cairo_line_to(c, 11, 5.7);
        cairo_stroke(c);
    } else if (k == 4) {
        cairo_arc(c, 8, 8, 2.4, 0, 2 * G_PI);
        cairo_stroke(c);
        for (i = 0; i < 8; i++) {
            cairo_move_to(c, 8 + CS[i][0] * 4.8, 8 + CS[i][1] * 4.8);
            cairo_line_to(c, 8 + CS[i][0] * 6.8, 8 + CS[i][1] * 6.8);
            cairo_stroke(c);
        }
    } else if (k == 5) {
        cairo_move_to(c, 8, 1.5);
        cairo_curve_to(c, 15, 8, 12.5, 12, 9, 11);
        cairo_line_to(c, 10.2, 14.5); cairo_line_to(c, 5.8, 14.5); cairo_line_to(c, 7, 11);
        cairo_curve_to(c, 3.5, 12, 1, 8, 8, 1.5);
        cairo_stroke(c);
    } else {
        rrett(c, 2, 2, 12, 12, 3);
        cairo_stroke(c);
    }
    return TRUE;
}

static GtkWidget *bottone_icona(const char *nome, const char *cmd)
{
    GtkWidget *b = gtk_button_new(), *bx = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 7);
    GtkWidget *ic = gtk_drawing_area_new(), *lb = gtk_label_new(nome);
    int k = 6;
    if (strstr(cmd, "terminal")) k = 0;
    else if (strstr(cmd, "thunar")) k = 1;
    else if (strstr(cmd, "negozio")) k = 2;
    else if (strstr(cmd, "aggiorna")) k = 3;
    else if (strstr(cmd, "settings")) k = 4;
    else if (strstr(cmd, "ventuno") || strstr(cmd, "poker")) k = 5;
    gtk_widget_set_size_request(ic, 16, 16);
    gtk_widget_set_valign(ic, GTK_ALIGN_CENTER);
    g_signal_connect(ic, "draw", G_CALLBACK(disegna_icona_link), GINT_TO_POINTER(k));
    gtk_label_set_xalign(GTK_LABEL(lb), 0);
    gtk_label_set_ellipsize(GTK_LABEL(lb), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(bx), ic, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bx), lb, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(b), bx);
    return b;
}

static void costruisci_link(void)
{
    static const char *pre[][4] = {{"Terminal", "Terminale", "xfce4-terminal", "Terminal"}, {"Files", "File", "thunar", "Archivos"},
        {"Store", "Negozio", "maginat-negozio", "Tienda"}, {"Update", "Aggiorna", "maginat-aggiorna", "Actualizar"},
        {"Settings", "Impostazioni", "xfce4-settings-manager", "Ajustes"}, {"Blackjack", "Ventuno", "maginat-ventuno", "Veintiuna"},
        {"Poker", "Poker", "maginat-poker", "Póquer"}};
    char *cf = file_link(), *dati = NULL, **righe;
    int i, n = 0;
    gtk_container_foreach(GTK_CONTAINER(grid_link), (GtkCallback)gtk_widget_destroy, NULL);
    if (!g_file_get_contents(cf, &dati, NULL, NULL)) {
        GString *g = g_string_new(TR("# One link per line: Name;command (maximum 6)\n", "# Un collegamento per riga: Nome;comando (massimo 6)\n", "# Un enlace por línea: Nombre;comando (máximo 6)\n"));
        char *dir = g_path_get_dirname(cf);
        for (i = 0; i < (int)(sizeof pre / sizeof pre[0]); i++)
            if (cmd_ok(pre[i][2])) g_string_append_printf(g, "%s;%s\n", TR(pre[i][0], pre[i][1], pre[i][3]), pre[i][2]);
        g_mkdir_with_parents(dir, 0700);
        g_file_set_contents(cf, g->str, -1, NULL);
        dati = g_string_free(g, FALSE);
        g_free(dir);
    }
    righe = g_strsplit(dati, "\n", -1);
    for (i = 0; righe[i] && n < 6; i++) {
        char *sep, *cmd;
        GtkWidget *b;
        g_strstrip(righe[i]);
        if (!righe[i][0] || righe[i][0] == '#') continue;
        sep = strchr(righe[i], ';');
        if (!sep) continue;
        *sep = 0;
        cmd = g_strstrip(sep + 1);
        if (!cmd[0]) continue;
        b = bottone_icona(g_strstrip(righe[i]), cmd);
        gtk_widget_set_hexpand(b, TRUE);
        g_object_set_data_full(G_OBJECT(b), "cmd", g_strdup(cmd), g_free);
        g_signal_connect(b, "clicked", G_CALLBACK(clic_link), NULL);
        gtk_grid_attach(GTK_GRID(grid_link), b, n % 2, n / 2, 1, 1);
        n++;
    }
    g_strfreev(righe);
    g_free(dati);
    g_free(cf);
    gtk_widget_show_all(grid_link);
}

/* ---------- scelta dei widget (icona nella barra e segnale SIGUSR1) ---------- */

enum { IW_OROL, IW_CAL, IW_MET, IW_MON, IW_BAT, IW_LINK, IW_NOTE, NW };
static GtkWidget *finestre[NW];
static const int altezze[NW] = {H_OROL, H_CAL, H_MET, H_MON, H_BAT, H_LINK, H_NOTE};
static const char *chiavi[NW] = {"orologio", "calendario", "meteo", "monitor", "batteria", "collegamenti", "note"};
static int attivo[NW];
static GtkWidget *voci[NW], *menu;
static const char *nomi_w[NW];
static GtkStatusIcon *ico;

static char *file_attivi(void)
{
    return g_build_filename(g_get_user_config_dir(), "maginat", "widget-attivi", NULL);
}

static void salva_attivi(void)
{
    char *cf = file_attivi();
    char *dir = g_path_get_dirname(cf);
    GString *g = g_string_new("");
    int i;
    for (i = 0; i < NW; i++)
        if (attivo[i]) g_string_append_printf(g, "%s\n", chiavi[i]);
    g_mkdir_with_parents(dir, 0700);
    g_file_set_contents(cf, g->str, -1, NULL);
    g_string_free(g, TRUE);
    g_free(dir);
    g_free(cf);
}

static void carica_attivi(void)
{
    char *cf = file_attivi();
    char *dati = NULL;
    int i, j;
    for (i = 0; i < NW; i++) attivo[i] = 0;
    if (g_file_get_contents(cf, &dati, NULL, NULL)) {
        char **v = g_strsplit(dati, "\n", -1);
        for (j = 0; v[j]; j++) {
            g_strstrip(v[j]);
            for (i = 0; i < NW; i++)
                if (strcmp(v[j], chiavi[i]) == 0) attivo[i] = 1;
        }
        g_strfreev(v);
        g_free(dati);
    }
    g_free(cf);
}

static void disponi(void)
{
    GdkScreen *sc = gdk_screen_get_default();
    int i, y = 44, col = 0;
    for (i = 0; i < NW; i++) {
        GtkWidget *w = finestre[i];
        if (attivo[i]) {
            int era = gtk_widget_get_visible(w);
            if (y + altezze[i] > gdk_screen_get_height(sc) - 40 && y > 44) {
                col++;
                y = 44;
            }
            gtk_window_move(GTK_WINDOW(w), gdk_screen_get_width(sc) - (LARG + 16) * (col + 1) - M_OMB, y - M_OMB);
            if (!era) {
                gtk_widget_show_all(w);
                if (i == IW_MET && m_conf) meteo_scarica();
                if (i == IW_MON) tick_mon(NULL);
                if (i == IW_BAT) tick_bat(GINT_TO_POINTER(1));
                if (i == IW_LINK) costruisci_link();
            }
            y += altezze[i] + 12;
        } else {
            gtk_widget_hide(w);
        }
    }
}

static void aggiorna_voci(void)
{
    int i;
    for (i = 0; i < NW; i++) {
        char *t = g_strdup_printf("%s  %s", attivo[i] ? "[x]" : "[  ]", nomi_w[i]);
        gtk_menu_item_set_label(GTK_MENU_ITEM(voci[i]), t);
        g_free(t);
    }
}

static void voce_cambiata(GtkMenuItem *it, gpointer d)
{
    int i = GPOINTER_TO_INT(d);
    (void)it;
    attivo[i] = !attivo[i];
    aggiorna_voci();
    salva_attivi();
    disponi();
}

static void tutti(GtkMenuItem *it, gpointer d)
{
    int i;
    (void)it;
    for (i = 0; i < NW; i++) attivo[i] = GPOINTER_TO_INT(d);
    aggiorna_voci();
    salva_attivi();
    disponi();
}

static gboolean rileggi(gpointer d)
{
    (void)d;
    carica_attivi();
    aggiorna_voci();
    disponi();
    return G_SOURCE_CONTINUE;
}

static void mostra_menu(GtkStatusIcon *s, guint b, guint t, gpointer d)
{
    (void)d;
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, s, b, t);
}

static void clic_icona(GtkStatusIcon *s, gpointer d)
{
    mostra_menu(s, 1, gtk_get_current_event_time(), d);
}

static void rrett(cairo_t *c, double x, double y, double w, double h, double r)
{
    cairo_new_sub_path(c);
    cairo_arc(c, x + w - r, y + r, r, -G_PI / 2, 0);
    cairo_arc(c, x + w - r, y + h - r, r, 0, G_PI / 2);
    cairo_arc(c, x + r, y + h - r, r, G_PI / 2, G_PI);
    cairo_arc(c, x + r, y + r, r, G_PI, 1.5 * G_PI);
    cairo_close_path(c);
}

static GdkPixbuf *icona_widget(void)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 24, 24);
    cairo_t *c = cairo_create(s);
    GdkPixbuf *pb;
    cairo_set_source_rgb(c, 0.2, 0.78, 0.5);
    rrett(c, 1, 3, 22, 18, 4);
    cairo_fill(c);
    cairo_set_source_rgb(c, 1, 1, 1);
    rrett(c, 4, 6, 9, 6, 1.2);
    cairo_fill(c);
    rrett(c, 15, 6, 5, 6, 1.2);
    cairo_fill(c);
    rrett(c, 4, 14, 16, 4, 1.2);
    cairo_fill(c);
    cairo_destroy(c);
    pb = gdk_pixbuf_get_from_surface(s, 0, 0, 24, 24);
    cairo_surface_destroy(s);
    return pb;
}

static void crea_menu(void)
{
    GtkWidget *it;
    GdkPixbuf *pb;
    int i;
    nomi_w[IW_OROL] = TR("Clock", "Orologio", "Reloj");
    nomi_w[IW_CAL] = TR("Calendar and clock", "Calendario e orologio", "Calendario y reloj");
    nomi_w[IW_MET] = TR("Weather", "Meteo", "El tiempo");
    nomi_w[IW_MON] = TR("System monitor", "Monitor di sistema", "Monitor del sistema");
    nomi_w[IW_BAT] = TR("Battery and network", "Batteria e rete", "Batería y red");
    nomi_w[IW_LINK] = TR("Quick links", "Collegamenti rapidi", "Enlaces rápidos");
    nomi_w[IW_NOTE] = TR("Notes", "Note", "Notas");
    menu = gtk_menu_new();
    for (i = 0; i < NW; i++) {
        voci[i] = gtk_menu_item_new_with_label("");
        g_signal_connect(voci[i], "activate", G_CALLBACK(voce_cambiata), GINT_TO_POINTER(i));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), voci[i]);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    it = gtk_menu_item_new_with_label(TR("Show all", "Mostra tutti", "Mostrar todos"));
    g_signal_connect(it, "activate", G_CALLBACK(tutti), GINT_TO_POINTER(1));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it);
    it = gtk_menu_item_new_with_label(TR("Hide all", "Nascondi tutti", "Ocultar todos"));
    g_signal_connect(it, "activate", G_CALLBACK(tutti), GINT_TO_POINTER(0));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it);
    gtk_widget_show_all(menu);
    ico = gtk_status_icon_new();
    pb = icona_widget();
    gtk_status_icon_set_from_pixbuf(ico, pb);
    g_object_unref(pb);
    gtk_status_icon_set_tooltip_text(ico, TR("Desktop widgets", "Widget del desktop", "Widgets del escritorio"));
    g_signal_connect(ico, "activate", G_CALLBACK(clic_icona), NULL);
    g_signal_connect(ico, "popup-menu", G_CALLBACK(mostra_menu), NULL);
}

/* ---------- avvio ---------- */

static GtkWidget *area(GtkWidget *win, int h, GCallback draw)
{
    GtkWidget *a = gtk_drawing_area_new();
    gtk_widget_set_size_request(a, LARG + 2 * M_OMB, h + 2 * M_OMB);
    g_signal_connect(a, "draw", draw, NULL);
    gtk_container_add(GTK_CONTAINER(win), a);
    return a;
}

static char **g_argv0;

static int vuole_ombre(GdkScreen *sc)
{
    const char *e = getenv("MAGINAT_OMBRE");
    if (e && e[0] == '0') return 0;
    return gdk_screen_is_composited(sc) && gdk_screen_get_rgba_visual(sc) != NULL;
}

static gboolean riavvia(gpointer d)
{
    (void)d;
    if (g_argv0) execv("/proc/self/exe", g_argv0);
    return FALSE;
}

static void comp_cambiato(GdkScreen *sc, gpointer d)
{
    (void)d;
    if (vuole_ombre(sc) != g_comp) g_timeout_add(800, riavvia, NULL);
}

int wid_main(int argc, char **argv)
{
    int y = 44;
    GtkWidget *box, *lb, *sw, *tv;
    GtkCssProvider *css;
    char *cf, *dati = NULL;
    (void)argc;
    g_argv0 = argv;
    g_comp = vuole_ombre(gdk_screen_get_default());
    g_signal_connect(gdk_screen_get_default(), "composited-changed", G_CALLBACK(comp_cambiato), NULL);

    css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        ".maginat-wid { background-color: transparent; background-image: none; }"
        ".maginat-nota, .maginat-nota text { background-color: rgba(33,43,56,0.06); color: #212b38; font-size: 12px; }"
        ".maginat-wid button { background-image: none; background-color: rgba(33,43,56,0.07); color: #212b38; border: none; border-radius: 9px; box-shadow: none; padding: 6px 8px; font-size: 11px; }"
        ".maginat-wid button:hover { background-color: rgba(33,43,56,0.15); }"
        ".maginat-titolo { color: #5b6b80; font-size: 11px; font-weight: bold; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    w_orol = nuova(H_OROL, &y, FALSE);
    a_orol = area(w_orol, H_OROL, G_CALLBACK(disegna_orol));

    w_cal = nuova(H_CAL, &y, FALSE);
    a_cal = area(w_cal, H_CAL, G_CALLBACK(disegna_cal));

    w_met = nuova(H_MET, &y, FALSE);
    a_met = area(w_met, H_MET, G_CALLBACK(disegna_met));
    gtk_widget_add_events(a_met, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(a_met, "button-press-event", G_CALLBACK(clic_meteo), NULL);
    gtk_widget_set_tooltip_text(a_met, TR("Click to refresh (or to choose the city)", "Clic per aggiornare (o per scegliere la città)", "Clic para actualizar (o para elegir la ciudad)"));

    w_mon = nuova(H_MON, &y, FALSE);
    a_mon = area(w_mon, H_MON, G_CALLBACK(disegna_mon));

    w_bat = nuova(H_BAT, &y, FALSE);
    a_bat = area(w_bat, H_BAT, G_CALLBACK(disegna_bat));

    w_link = nuova(H_LINK, &y, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(w_link), M_OMB);
    g_signal_connect(w_link, "draw", G_CALLBACK(fondo_finestra), NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(w_link), "maginat-wid");
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    lb = gtk_label_new(TR("Quick links", "Collegamenti rapidi", "Enlaces rápidos"));
    gtk_label_set_xalign(GTK_LABEL(lb), 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lb), "maginat-titolo");
    gtk_widget_set_tooltip_text(lb, TR("Edit ~/.config/maginat/collegamenti.conf (one line: Name;command)",
        "Modifica ~/.config/maginat/collegamenti.conf (una riga: Nome;comando)", "Edita ~/.config/maginat/collegamenti.conf (una línea: Nombre;comando)"));
    grid_link = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_link), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid_link), 6);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid_link), TRUE);
    gtk_box_pack_start(GTK_BOX(box), lb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), grid_link, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(w_link), box);

    w_note = nuova(H_NOTE, &y, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(w_note), M_OMB);
    g_signal_connect(w_note, "draw", G_CALLBACK(fondo_finestra), NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(w_note), "maginat-wid");
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    lb = gtk_label_new(TR("Notes", "Note", "Notas"));
    gtk_label_set_xalign(GTK_LABEL(lb), 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lb), "maginat-titolo");
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    tv = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
    gtk_style_context_add_class(gtk_widget_get_style_context(tv), "maginat-nota");
    buf_nota = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    cf = file_nota();
    if (g_file_get_contents(cf, &dati, NULL, NULL)) {
        gtk_text_buffer_set_text(buf_nota, dati, -1);
        g_free(dati);
    }
    g_free(cf);
    g_signal_connect(buf_nota, "changed", G_CALLBACK(nota_cambiata), NULL);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_box_pack_start(GTK_BOX(box), lb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sw, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(w_note), box);

    finestre[IW_OROL] = w_orol; finestre[IW_CAL] = w_cal; finestre[IW_MET] = w_met; finestre[IW_MON] = w_mon;
    finestre[IW_BAT] = w_bat; finestre[IW_LINK] = w_link; finestre[IW_NOTE] = w_note;
    carica_meteo();
    carica_attivi();
    crea_menu();
    aggiorna_voci();
    tick_mon(GINT_TO_POINTER(1));
    tick_bat(GINT_TO_POINTER(1));
    disponi();
    g_timeout_add_seconds(20, tick_cal, NULL);
    g_timeout_add_seconds(3, tick_mon, NULL);
    g_timeout_add_seconds(5, tick_bat, NULL);
    g_timeout_add_seconds(1800, tick_meteo, NULL);
    g_unix_signal_add(SIGUSR1, rileggi, NULL);
    return 0;
}
