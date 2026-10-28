/* maginat-widget: widget da desktop per Maginat (orologio e calendario, meteo, monitor, note)
 * Licenza: GPL-2.0-or-later
 * Fa parte di maginat-tray: wid_main() viene chiamata da maginat-tray-main.c
 * Il meteo resta spento finche l'utente non sceglie una citta (usa open-meteo.com)
 */
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/statvfs.h>

static int is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
#define TR(en, it) (is_it() ? (it) : (en))

#define LARG 230
#define H_CAL 250
#define H_MET 74
#define H_MON 124
#define H_NOTE 150
#define H_OROL 92
#define H_BAT 100
#define H_LINK 152

static GtkWidget *w_cal, *w_met, *w_mon, *w_note, *w_orol, *w_bat, *w_link;
static GtkWidget *a_cal, *a_met, *a_mon, *a_orol, *a_bat, *grid_link;
static GtkTextBuffer *buf_nota;
static guint t_salva;

/* ---------- disegno ---------- */

static void sfondo(cairo_t *c, int w, int h)
{
    cairo_set_source_rgb(c, 0.106, 0.141, 0.188);
    cairo_paint(c);
    cairo_set_source_rgb(c, 0.25, 0.32, 0.40);
    cairo_set_line_width(c, 1);
    cairo_rectangle(c, 0.5, 0.5, w - 1, h - 1);
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
    gtk_widget_set_size_request(w, LARG, h);
    gtk_window_move(GTK_WINDOW(w), gdk_screen_get_width(sc) - LARG - 16, *y);
    *y += h + 12;
    return w;
}

/* ---------- orologio e calendario ---------- */

static gboolean disegna_cal(GtkWidget *wd, cairo_t *c, gpointer d)
{
    static const char *gi_it[] = {"domenica","lunedì","martedì","mercoledì","giovedì","venerdì","sabato"};
    static const char *gi_en[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    static const char *me_it[] = {"gennaio","febbraio","marzo","aprile","maggio","giugno","luglio","agosto","settembre","ottobre","novembre","dicembre"};
    static const char *me_en[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
    static const char *in_it[] = {"L","M","M","G","V","S","D"};
    static const char *in_en[] = {"M","T","W","T","F","S","S"};
    static const int gm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    time_t t = time(NULL);
    struct tm tm, p;
    char s[96];
    int i, off, nd, riga, col, anno;

    (void)wd; (void)d;
    localtime_r(&t, &tm);
    sfondo(c, LARG, H_CAL);
    snprintf(s, sizeof s, "%02d:%02d", tm.tm_hour, tm.tm_min);
    testo(c, LARG / 2.0, 58, 46, 1, s, 0.91, 0.94, 0.96, 1);
    snprintf(s, sizeof s, "%s %d %s", TR(gi_en, gi_it)[tm.tm_wday], tm.tm_mday, TR(me_en, me_it)[tm.tm_mon]);
    testo(c, LARG / 2.0, 82, 15, 0, s, 0.29, 0.64, 1.0, 1);
    snprintf(s, sizeof s, "%d", tm.tm_year + 1900);
    testo(c, LARG / 2.0, 100, 12, 0, s, 0.56, 0.64, 0.72, 1);

    for (i = 0; i < 7; i++)
        testo(c, 22 + i * 31, 124, 11, 1, TR(in_en, in_it)[i], 0.56, 0.64, 0.72, 1);
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
            cairo_set_source_rgb(c, 0.29, 0.64, 1.0);
            cairo_arc(c, 22 + col * 31, 142 + riga * 18 - 4, 9, 0, 6.2832);
            cairo_fill(c);
            snprintf(s, sizeof s, "%d", i);
            testo(c, 22 + col * 31, 142 + riga * 18, 11, 1, s, 0.06, 0.09, 0.12, 1);
        } else {
            snprintf(s, sizeof s, "%d", i);
            testo(c, 22 + col * 31, 142 + riga * 18, 11, 0, s,
                  col >= 5 ? 0.95 : 0.88, col >= 5 ? 0.62 : 0.91, col >= 5 ? 0.45 : 0.94, 1);
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
    testo(c, 12, y, 12, 0, nome, 0.78, 0.84, 0.90, 0);
    cairo_set_source_rgb(c, 0.20, 0.25, 0.31);
    cairo_rectangle(c, 62, y - 9, 108, 9);
    cairo_fill(c);
    cairo_set_source_rgb(c, r, g, b);
    cairo_rectangle(c, 62, y - 9, 108.0 * pct / 100.0, 9);
    cairo_fill(c);
    snprintf(s, sizeof s, "%.0f%%", pct);
    testo(c, LARG - 12, y, 12, 1, s, 0.91, 0.94, 0.96, 2);
}

static gboolean disegna_mon(GtkWidget *wd, cairo_t *c, gpointer d)
{
    char a[40], b[40], s[96];
    (void)wd; (void)d;
    sfondo(c, LARG, H_MON);
    testo(c, 12, 20, 11, 1, TR("System", "Sistema"), 0.56, 0.64, 0.72, 0);
    barra(c, 42, "CPU", cpu_pct, 0.29, 0.64, 1.0);
    barra(c, 64, "RAM", ram_pct, 0.48, 0.85, 0.56);
    barra(c, 86, TR("Disk", "Disco"), disk_pct, 0.95, 0.69, 0.37);
    fmt_rate(rx_s, a, sizeof a);
    fmt_rate(tx_s, b, sizeof b);
    snprintf(s, sizeof s, "↓ %s   ↑ %s", a, b);
    testo(c, 12, 111, 12, 0, s, 0.78, 0.84, 0.90, 0);
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
    if (c == 0) return TR("Clear", "Sereno");
    if (c == 1) return TR("Mostly clear", "Prevalentemente sereno");
    if (c == 2) return TR("Partly cloudy", "Parzialmente nuvoloso");
    if (c == 3) return TR("Overcast", "Coperto");
    if (c == 45 || c == 48) return TR("Fog", "Nebbia");
    if (c >= 51 && c <= 57) return TR("Drizzle", "Pioggerella");
    if (c >= 61 && c <= 67) return TR("Rain", "Pioggia");
    if (c >= 71 && c <= 77) return TR("Snow", "Neve");
    if (c >= 80 && c <= 82) return TR("Showers", "Rovesci");
    if (c == 85 || c == 86) return TR("Snow showers", "Rovesci di neve");
    if (c >= 95) return TR("Thunderstorm", "Temporale");
    return TR("Unknown", "Variabile");
}

static gboolean disegna_met(GtkWidget *wd, cairo_t *c, gpointer d)
{
    char s[64];
    (void)wd; (void)d;
    sfondo(c, LARG, H_MET);
    if (!m_conf) {
        testo(c, 12, 28, 14, 1, TR("Weather", "Meteo"), 0.91, 0.94, 0.96, 0);
        testo(c, 12, 52, 12, 0, m_msg[0] ? m_msg : TR("Click to turn on", "Clic per attivare"), 0.56, 0.64, 0.72, 0);
    } else if (!m_ok) {
        testo(c, 12, 28, 14, 1, m_citta, 0.91, 0.94, 0.96, 0);
        testo(c, 12, 52, 12, 0, m_msg[0] ? m_msg : TR("Loading...", "Caricamento..."), 0.56, 0.64, 0.72, 0);
    } else {
        testo(c, 12, 28, 14, 1, m_citta, 0.91, 0.94, 0.96, 0);
        testo(c, 12, 54, 13, 0, descrizione(m_code), 0.29, 0.64, 1.0, 0);
        snprintf(s, sizeof s, "%.0f°", m_temp);
        testo(c, LARG - 12, 56, 32, 1, s, 0.91, 0.94, 0.96, 2);
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
    if (!m_ok) g_strlcpy(m_msg, TR("Not available", "Non disponibile"), sizeof m_msg);
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
        g_strlcpy(m_msg, TR("Not available", "Non disponibile"), sizeof m_msg);
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
        g_strlcpy(m_msg, TR("City not found", "Città non trovata"), sizeof m_msg);
    }
    g_free(out);
    gtk_widget_queue_draw(a_met);
}

static void meteo_configura(void)
{
    GtkWidget *dl, *en, *lb, *ar;
    char url[400];
    char *esc;
    dl = gtk_dialog_new_with_buttons(TR("Weather", "Meteo"), NULL, 0,
        TR("Cancel", "Annulla"), GTK_RESPONSE_CANCEL, "OK", GTK_RESPONSE_OK, NULL);
    gtk_window_set_keep_above(GTK_WINDOW(dl), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(dl), 8);
    ar = gtk_dialog_get_content_area(GTK_DIALOG(dl));
    lb = gtk_label_new(TR("City name. The weather uses open-meteo.com: the city name and your IP address will be sent to that service.",
                          "Nome della città. Il meteo usa open-meteo.com: il nome della città e il tuo indirizzo IP saranno inviati a quel servizio."));
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
            esc, is_it() ? "it" : "en");
        g_free(esc);
        g_strlcpy(m_msg, TR("Searching...", "Ricerca..."), sizeof m_msg);
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
        g_strlcpy(m_msg, TR("Loading...", "Caricamento..."), sizeof m_msg);
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
    static const char *gi_it[] = {"domenica","lunedì","martedì","mercoledì","giovedì","venerdì","sabato"};
    static const char *gi_en[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    static const char *me_it[] = {"gennaio","febbraio","marzo","aprile","maggio","giugno","luglio","agosto","settembre","ottobre","novembre","dicembre"};
    static const char *me_en[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
    time_t t = time(NULL);
    struct tm tm;
    char s[96];
    (void)wd; (void)d;
    localtime_r(&t, &tm);
    sfondo(c, LARG, H_OROL);
    snprintf(s, sizeof s, "%02d:%02d", tm.tm_hour, tm.tm_min);
    testo(c, LARG / 2.0, 56, 46, 1, s, 0.91, 0.94, 0.96, 1);
    snprintf(s, sizeof s, "%s %d %s", TR(gi_en, gi_it)[tm.tm_wday], tm.tm_mday, TR(me_en, me_it)[tm.tm_mon]);
    testo(c, LARG / 2.0, 80, 13, 0, s, 0.29, 0.64, 1.0, 1);
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
                if (strcmp(v, "Charging") == 0) g_strlcpy(b_stato, TR("Charging", "In carica"), sizeof b_stato);
                else if (strcmp(v, "Discharging") == 0) g_strlcpy(b_stato, TR("On battery", "A batteria"), sizeof b_stato);
                else if (strcmp(v, "Full") == 0) g_strlcpy(b_stato, TR("Full", "Carica"), sizeof b_stato);
                else if (strcmp(v, "Not charging") == 0) g_strlcpy(b_stato, TR("Plugged in", "Collegata"), sizeof b_stato);
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
    testo(c, 12, 20, 11, 1, TR("Battery and network", "Batteria e rete"), 0.56, 0.64, 0.72, 0);
    if (b_pres) {
        if (b_pct <= 20) barra(c, 44, TR("Battery", "Batteria"), b_pct, 0.95, 0.45, 0.45);
        else barra(c, 44, TR("Battery", "Batteria"), b_pct, 0.48, 0.85, 0.56);
        if (b_stato[0]) testo(c, 12, 62, 11, 0, b_stato, 0.56, 0.64, 0.72, 0);
    } else {
        testo(c, 12, 48, 12, 0, TR("Mains power", "Alimentazione di rete"), 0.78, 0.84, 0.90, 0);
    }
    if (b_rete == 2) {
        if (b_ssid[0]) snprintf(s, sizeof s, "Wi-Fi: %s", b_ssid);
        else snprintf(s, sizeof s, "%s", TR("Wi-Fi: connected", "Wi-Fi: connesso"));
        testo(c, 12, 86, 12, 0, s, 0.78, 0.84, 0.90, 0);
    } else if (b_rete == 1) {
        testo(c, 12, 86, 12, 0, TR("Wired: connected", "Cavo: connesso"), 0.78, 0.84, 0.90, 0);
    } else {
        testo(c, 12, 86, 12, 0, TR("Network: not connected", "Rete: non connessa"), 0.95, 0.69, 0.37, 0);
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

static void costruisci_link(void)
{
    static const char *pre[][3] = {{"Terminal", "Terminale", "xfce4-terminal"}, {"Files", "File", "thunar"},
        {"Store", "Negozio", "maginat-negozio"}, {"Update", "Aggiorna", "maginat-aggiorna"},
        {"Settings", "Impostazioni", "xfce4-settings-manager"}, {"Blackjack", "Ventuno", "maginat-ventuno"},
        {"Poker", "Poker", "maginat-poker"}};
    char *cf = file_link(), *dati = NULL, **righe;
    int i, n = 0;
    gtk_container_foreach(GTK_CONTAINER(grid_link), (GtkCallback)gtk_widget_destroy, NULL);
    if (!g_file_get_contents(cf, &dati, NULL, NULL)) {
        GString *g = g_string_new(TR("# One link per line: Name;command (maximum 6)\n", "# Un collegamento per riga: Nome;comando (massimo 6)\n"));
        char *dir = g_path_get_dirname(cf);
        for (i = 0; i < (int)(sizeof pre / sizeof pre[0]); i++)
            if (cmd_ok(pre[i][2])) g_string_append_printf(g, "%s;%s\n", TR(pre[i][0], pre[i][1]), pre[i][2]);
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
        b = gtk_button_new_with_label(g_strstrip(righe[i]));
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
            gtk_window_move(GTK_WINDOW(w), gdk_screen_get_width(sc) - (LARG + 16) * (col + 1), y);
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
    nomi_w[IW_OROL] = TR("Clock", "Orologio");
    nomi_w[IW_CAL] = TR("Calendar and clock", "Calendario e orologio");
    nomi_w[IW_MET] = TR("Weather", "Meteo");
    nomi_w[IW_MON] = TR("System monitor", "Monitor di sistema");
    nomi_w[IW_BAT] = TR("Battery and network", "Batteria e rete");
    nomi_w[IW_LINK] = TR("Quick links", "Collegamenti rapidi");
    nomi_w[IW_NOTE] = TR("Notes", "Note");
    menu = gtk_menu_new();
    for (i = 0; i < NW; i++) {
        voci[i] = gtk_menu_item_new_with_label("");
        g_signal_connect(voci[i], "activate", G_CALLBACK(voce_cambiata), GINT_TO_POINTER(i));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), voci[i]);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    it = gtk_menu_item_new_with_label(TR("Show all", "Mostra tutti"));
    g_signal_connect(it, "activate", G_CALLBACK(tutti), GINT_TO_POINTER(1));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it);
    it = gtk_menu_item_new_with_label(TR("Hide all", "Nascondi tutti"));
    g_signal_connect(it, "activate", G_CALLBACK(tutti), GINT_TO_POINTER(0));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it);
    gtk_widget_show_all(menu);
    ico = gtk_status_icon_new();
    pb = icona_widget();
    gtk_status_icon_set_from_pixbuf(ico, pb);
    g_object_unref(pb);
    gtk_status_icon_set_tooltip_text(ico, TR("Desktop widgets", "Widget del desktop"));
    g_signal_connect(ico, "activate", G_CALLBACK(clic_icona), NULL);
    g_signal_connect(ico, "popup-menu", G_CALLBACK(mostra_menu), NULL);
}

/* ---------- avvio ---------- */

static GtkWidget *area(GtkWidget *win, int h, GCallback draw)
{
    GtkWidget *a = gtk_drawing_area_new();
    gtk_widget_set_size_request(a, LARG, h);
    g_signal_connect(a, "draw", draw, NULL);
    gtk_container_add(GTK_CONTAINER(win), a);
    return a;
}

int wid_main(int argc, char **argv)
{
    int y = 44;
    GtkWidget *box, *lb, *sw, *tv;
    GtkCssProvider *css;
    char *cf, *dati = NULL;
    (void)argc; (void)argv;

    css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        ".maginat-wid { background-color: #1b2430; }"
        ".maginat-nota, .maginat-nota text { background-color: #1b2430; color: #e8eef5; font-size: 12px; }"
        ".maginat-wid button { background-image: none; background-color: #2a3748; color: #e8eef5; border: none; box-shadow: none; padding: 5px 6px; }"
        ".maginat-wid button:hover { background-color: #3a4d66; }"
        ".maginat-titolo { color: #8fa3b8; font-size: 11px; font-weight: bold; }", -1, NULL);
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
    gtk_widget_set_tooltip_text(a_met, TR("Click to refresh (or to choose the city)", "Clic per aggiornare (o per scegliere la città)"));

    w_mon = nuova(H_MON, &y, FALSE);
    a_mon = area(w_mon, H_MON, G_CALLBACK(disegna_mon));

    w_bat = nuova(H_BAT, &y, FALSE);
    a_bat = area(w_bat, H_BAT, G_CALLBACK(disegna_bat));

    w_link = nuova(H_LINK, &y, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(w_link), "maginat-wid");
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    lb = gtk_label_new(TR("Quick links", "Collegamenti rapidi"));
    gtk_label_set_xalign(GTK_LABEL(lb), 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lb), "maginat-titolo");
    gtk_widget_set_tooltip_text(lb, TR("Edit ~/.config/maginat/collegamenti.conf (one line: Name;command)",
        "Modifica ~/.config/maginat/collegamenti.conf (una riga: Nome;comando)"));
    grid_link = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_link), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid_link), 6);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid_link), TRUE);
    gtk_box_pack_start(GTK_BOX(box), lb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), grid_link, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(w_link), box);

    w_note = nuova(H_NOTE, &y, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(w_note), "maginat-wid");
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    lb = gtk_label_new(TR("Notes", "Note"));
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
