/* maginat-bluetooth-tray: icona Bluetooth per Maginat (GTK3 + BlueZ via D-Bus)
 * Licenza: GPL-2.0-or-later
 * Compilazione: gcc -O2 -o maginat-bluetooth-tray maginat-bluetooth-tray.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0)
 */
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
#define TR(en, it) (is_it() ? (it) : (en))
#define AGENT_PATH "/org/maginat/agent"

typedef struct {
    char path[160];
    char nome[96];
    gboolean paired, connected;
} Dev;

static GDBusConnection *bus;
static GtkStatusIcon *icona;
static char adapter[160];
static gboolean powered, cerca, agente_ok;
static int nconn;
static Dev devs[64];
static int ndev;
static GtkWidget *info_dlg;

/* ---------- lettura stato da BlueZ ---------- */

static void leggi(void)
{
    GError *e = NULL;
    GVariant *r, *objs, *ifs;
    GVariantIter it;
    const char *path;

    adapter[0] = 0; ndev = 0; nconn = 0; powered = FALSE;
    r = g_dbus_connection_call_sync(bus, "org.bluez", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects", NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"), G_DBUS_CALL_FLAGS_NONE, 2000, NULL, &e);
    if (!r) { g_clear_error(&e); return; }
    objs = g_variant_get_child_value(r, 0);
    g_variant_iter_init(&it, objs);
    while (g_variant_iter_next(&it, "{&o@a{sa{sv}}}", &path, &ifs)) {
        GVariant *ad = g_variant_lookup_value(ifs, "org.bluez.Adapter1", NULL);
        GVariant *dv = g_variant_lookup_value(ifs, "org.bluez.Device1", NULL);
        if (ad) {
            if (!adapter[0]) {
                gboolean p = FALSE;
                g_strlcpy(adapter, path, sizeof adapter);
                if (g_variant_lookup(ad, "Powered", "b", &p)) powered = p;
            }
            g_variant_unref(ad);
        }
        if (dv) {
            if (ndev < 64) {
                Dev *d = &devs[ndev];
                const char *s = NULL;
                gboolean b = FALSE;
                memset(d, 0, sizeof *d);
                g_strlcpy(d->path, path, sizeof d->path);
                if (g_variant_lookup(dv, "Alias", "&s", &s) ||
                    g_variant_lookup(dv, "Address", "&s", &s))
                    g_strlcpy(d->nome, s, sizeof d->nome);
                if (g_variant_lookup(dv, "Paired", "b", &b)) d->paired = b;
                b = FALSE;
                if (g_variant_lookup(dv, "Connected", "b", &b)) d->connected = b;
                if (d->connected) nconn++;
                ndev++;
            }
            g_variant_unref(dv);
        }
        g_variant_unref(ifs);
    }
    g_variant_unref(objs);
    g_variant_unref(r);
}

/* ---------- chiamate a BlueZ ---------- */

static void chiama(const char *path, const char *iface, const char *metodo, GVariant *par)
{
    g_dbus_connection_call(bus, "org.bluez", path, iface, metodo, par, NULL,
        G_DBUS_CALL_FLAGS_NONE, 30000, NULL, NULL, NULL);
}

static void imposta_bool(const char *path, const char *iface, const char *prop, gboolean v)
{
    g_dbus_connection_call(bus, "org.bluez", path, "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", iface, prop, g_variant_new_boolean(v)), NULL,
        G_DBUS_CALL_FLAGS_NONE, 5000, NULL, NULL, NULL);
}

static void pair_fine(GObject *src, GAsyncResult *res, gpointer dati)
{
    char *path = dati;
    GError *e = NULL;
    GVariant *r = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
    if (r) {
        g_variant_unref(r);
        imposta_bool(path, "org.bluez.Device1", "Trusted", TRUE);
        chiama(path, "org.bluez.Device1", "Connect", NULL);
    } else {
        g_clear_error(&e);
    }
    g_free(path);
}

static gboolean ferma_ricerca(gpointer d)
{
    (void)d;
    if (adapter[0]) chiama(adapter, "org.bluez.Adapter1", "StopDiscovery", NULL);
    cerca = FALSE;
    return FALSE;
}

/* ---------- callback del menu ---------- */

static void cb_acceso(GtkMenuItem *m, gpointer d)
{
    (void)m; (void)d;
    if (!powered) g_spawn_command_line_async("rfkill unblock bluetooth", NULL);
    if (adapter[0]) imposta_bool(adapter, "org.bluez.Adapter1", "Powered", !powered);
}

static void cb_cerca(GtkMenuItem *m, gpointer d)
{
    (void)m; (void)d;
    if (!adapter[0] || cerca) return;
    cerca = TRUE;
    chiama(adapter, "org.bluez.Adapter1", "StartDiscovery", NULL);
    g_timeout_add_seconds(10, ferma_ricerca, NULL);
}

static void cb_connetti(GtkMenuItem *m, gpointer d)
{
    const char *path = g_object_get_data(G_OBJECT(m), "p");
    gboolean conn = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(m), "c"));
    (void)d;
    chiama(path, "org.bluez.Device1", conn ? "Disconnect" : "Connect", NULL);
}

static void cb_rimuovi(GtkMenuItem *m, gpointer d)
{
    const char *path = g_object_get_data(G_OBJECT(m), "p");
    (void)d;
    if (adapter[0])
        chiama(adapter, "org.bluez.Adapter1", "RemoveDevice", g_variant_new("(o)", path));
}

static void cb_accoppia(GtkMenuItem *m, gpointer d)
{
    const char *path = g_object_get_data(G_OBJECT(m), "p");
    (void)d;
    g_dbus_connection_call(bus, "org.bluez", path, "org.bluez.Device1", "Pair", NULL, NULL,
        G_DBUS_CALL_FLAGS_NONE, 60000, NULL, pair_fine, g_strdup(path));
}

static GtkWidget *voce(GtkWidget *menu, const char *testo, GCallback cb, const char *path, int c)
{
    GtkWidget *it = gtk_menu_item_new_with_label(testo);
    if (path) g_object_set_data_full(G_OBJECT(it), "p", g_strdup(path), g_free);
    g_object_set_data(G_OBJECT(it), "c", GINT_TO_POINTER(c));
    if (cb) g_signal_connect(it, "activate", cb, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it);
    return it;
}

static void mostra_menu(GtkStatusIcon *i, guint button, guint tempo)
{
    GtkWidget *menu, *it;
    int k, trovati = 0, paired = 0;
    char buf[200];

    leggi();
    menu = gtk_menu_new();
    if (!adapter[0]) {
        it = voce(menu, TR("No Bluetooth adapter", "Nessun adattatore Bluetooth"), NULL, NULL, 0);
        gtk_widget_set_sensitive(it, FALSE);
    } else {
        it = gtk_check_menu_item_new_with_label(TR("Bluetooth on", "Bluetooth acceso"));
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(it), powered);
        g_signal_connect(it, "activate", G_CALLBACK(cb_acceso), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), it);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        for (k = 0; k < ndev; k++) {
            GtkWidget *sub, *top;
            if (!devs[k].paired) continue;
            paired++;
            snprintf(buf, sizeof buf, "%s%s", devs[k].nome, devs[k].connected ? TR("  (connected)", "  (connesso)") : "");
            top = voce(menu, buf, NULL, NULL, 0);
            sub = gtk_menu_new();
            voce(sub, devs[k].connected ? TR("Disconnect", "Disconnetti") : TR("Connect", "Connetti"), G_CALLBACK(cb_connetti),
                 devs[k].path, devs[k].connected);
            voce(sub, TR("Remove device", "Rimuovi dispositivo"), G_CALLBACK(cb_rimuovi), devs[k].path, 0);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(top), sub);
        }
        if (!paired) {
            it = voce(menu, TR("No paired devices", "Nessun dispositivo accoppiato"), NULL, NULL, 0);
            gtk_widget_set_sensitive(it, FALSE);
        }
        for (k = 0; k < ndev; k++) {
            if (devs[k].paired) continue;
            if (!trovati) {
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
                it = voce(menu, TR("Found (click to pair):", "Trovati (clic per accoppiare):"), NULL, NULL, 0);
                gtk_widget_set_sensitive(it, FALSE);
            }
            trovati++;
            voce(menu, devs[k].nome, G_CALLBACK(cb_accoppia), devs[k].path, 0);
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        it = voce(menu, cerca ? TR("Searching...", "Ricerca in corso...") : TR("Search for new devices (10 s)", "Cerca nuovi dispositivi (10 s)"),
                  G_CALLBACK(cb_cerca), NULL, 0);
        gtk_widget_set_sensitive(it, powered && !cerca);
    }
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, i, button, tempo);
}

static void cb_popup(GtkStatusIcon *i, guint button, guint tempo, gpointer d)
{
    (void)d;
    mostra_menu(i, button, tempo);
}

static void cb_activate(GtkStatusIcon *i, gpointer d)
{
    (void)d;
    mostra_menu(i, 0, gtk_get_current_event_time());
}

/* ---------- icona disegnata con cairo (stato: 0 spento, 1 acceso, 2 connesso) ---------- */

static GdkPixbuf *disegna(int stato)
{
    const int S = 32;
    cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, S, S);
    cairo_t *c = cairo_create(sf);
    GdkPixbuf *pb;

    if (stato == 0) cairo_set_source_rgb(c, 0.45, 0.45, 0.45);
    else cairo_set_source_rgb(c, 0.05, 0.35, 0.85);
    cairo_arc(c, S / 2.0, S / 2.0, S / 2.0 - 1, 0, 6.2832);
    cairo_fill(c);

    cairo_set_source_rgb(c, 1, 1, 1);
    cairo_set_line_width(c, S * 0.08);
    cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(c, CAIRO_LINE_JOIN_ROUND);
    cairo_move_to(c, S * 0.30, S * 0.35);
    cairo_line_to(c, S * 0.70, S * 0.65);
    cairo_line_to(c, S * 0.50, S * 0.82);
    cairo_line_to(c, S * 0.50, S * 0.18);
    cairo_line_to(c, S * 0.70, S * 0.35);
    cairo_line_to(c, S * 0.30, S * 0.65);
    cairo_stroke(c);

    if (stato == 0) {
        cairo_set_source_rgb(c, 0.85, 0.1, 0.1);
        cairo_set_line_width(c, S * 0.09);
        cairo_move_to(c, S * 0.18, S * 0.82);
        cairo_line_to(c, S * 0.82, S * 0.18);
        cairo_stroke(c);
    } else if (stato == 2) {
        cairo_set_source_rgb(c, 0.1, 0.75, 0.2);
        cairo_arc(c, S * 0.80, S * 0.80, S * 0.14, 0, 6.2832);
        cairo_fill(c);
    }
    pb = gdk_pixbuf_get_from_surface(sf, 0, 0, S, S);
    cairo_destroy(c);
    cairo_surface_destroy(sf);
    return pb;
}

/* ---------- agente di accoppiamento (org.bluez.Agent1) ---------- */

static const char *agent_xml =
    "<node><interface name='org.bluez.Agent1'>"
    "<method name='Release'/>"
    "<method name='RequestPinCode'><arg type='o' direction='in'/><arg type='s' direction='out'/></method>"
    "<method name='DisplayPinCode'><arg type='o' direction='in'/><arg type='s' direction='in'/></method>"
    "<method name='RequestPasskey'><arg type='o' direction='in'/><arg type='u' direction='out'/></method>"
    "<method name='DisplayPasskey'><arg type='o' direction='in'/><arg type='u' direction='in'/><arg type='q' direction='in'/></method>"
    "<method name='RequestConfirmation'><arg type='o' direction='in'/><arg type='u' direction='in'/></method>"
    "<method name='RequestAuthorization'><arg type='o' direction='in'/></method>"
    "<method name='AuthorizeService'><arg type='o' direction='in'/><arg type='s' direction='in'/></method>"
    "<method name='Cancel'/>"
    "</interface></node>";

static char *chiedi(const char *msg)
{
    GtkWidget *d = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_OK_CANCEL, "%s", msg);
    GtkWidget *en = gtk_entry_new();
    char *out = NULL;
    gtk_window_set_keep_above(GTK_WINDOW(d), TRUE);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(d))), en);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_OK)
        out = g_strdup(gtk_entry_get_text(GTK_ENTRY(en)));
    gtk_widget_destroy(d);
    return out;
}

static void mostra_info(const char *msg)
{
    if (info_dlg) gtk_widget_destroy(info_dlg);
    info_dlg = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", msg);
    gtk_window_set_keep_above(GTK_WINDOW(info_dlg), TRUE);
    g_signal_connect(info_dlg, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    g_signal_connect(info_dlg, "destroy", G_CALLBACK(gtk_widget_destroyed), &info_dlg);
    gtk_widget_show(info_dlg);
}

static void rifiuta(GDBusMethodInvocation *inv)
{
    g_dbus_method_invocation_return_dbus_error(inv, "org.bluez.Error.Rejected", TR("Rejected", "Rifiutato"));
}

static void agente_chiamata(GDBusConnection *conn, const char *sender, const char *path,
    const char *iface, const char *m, GVariant *par, GDBusMethodInvocation *inv, gpointer d)
{
    char buf[200];
    (void)conn; (void)sender; (void)path; (void)iface; (void)d;

    if (!strcmp(m, "RequestConfirmation")) {
        guint32 pk = 0;
        GtkWidget *dl;
        gint r;
        g_variant_get(par, "(ou)", NULL, &pk);
        snprintf(buf, sizeof buf, TR("Does the code %06u match the one shown on the device?", "Il codice %06u corrisponde a quello mostrato sul dispositivo?"), pk);
        dl = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", buf);
        gtk_window_set_keep_above(GTK_WINDOW(dl), TRUE);
        r = gtk_dialog_run(GTK_DIALOG(dl));
        gtk_widget_destroy(dl);
        if (r == GTK_RESPONSE_YES) g_dbus_method_invocation_return_value(inv, NULL);
        else rifiuta(inv);
    } else if (!strcmp(m, "RequestPinCode")) {
        char *s = chiedi(TR("Enter the PIN of the device:", "Inserisci il PIN del dispositivo:"));
        if (s) { g_dbus_method_invocation_return_value(inv, g_variant_new("(s)", s)); g_free(s); }
        else rifiuta(inv);
    } else if (!strcmp(m, "RequestPasskey")) {
        char *s = chiedi(TR("Enter the code of the device (6 digits):", "Inserisci il codice del dispositivo (6 cifre):"));
        if (s) { g_dbus_method_invocation_return_value(inv, g_variant_new("(u)", (guint32)atoi(s))); g_free(s); }
        else rifiuta(inv);
    } else if (!strcmp(m, "DisplayPasskey")) {
        guint32 pk = 0;
        g_variant_get(par, "(ouq)", NULL, &pk, NULL);
        snprintf(buf, sizeof buf, TR("Type %06u on the device and press Enter.", "Digita %06u sul dispositivo e premi Invio."), pk);
        mostra_info(buf);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(m, "DisplayPinCode")) {
        const char *pin = "";
        g_variant_get(par, "(o&s)", NULL, &pin);
        snprintf(buf, sizeof buf, TR("Type the PIN %s on the device and press Enter.", "Digita il PIN %s sul dispositivo e premi Invio."), pin);
        mostra_info(buf);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else if (!strcmp(m, "Cancel")) {
        if (info_dlg) gtk_widget_destroy(info_dlg);
        g_dbus_method_invocation_return_value(inv, NULL);
    } else {
        g_dbus_method_invocation_return_value(inv, NULL);
    }
}

static const GDBusInterfaceVTable agente_vtable = { agente_chiamata, NULL, NULL, { 0 } };

static void registra_agente(void)
{
    GError *e = NULL;
    GVariant *r = g_dbus_connection_call_sync(bus, "org.bluez", "/org/bluez",
        "org.bluez.AgentManager1", "RegisterAgent",
        g_variant_new("(os)", AGENT_PATH, "KeyboardDisplay"), NULL,
        G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &e);
    if (!r) { g_clear_error(&e); return; }
    g_variant_unref(r);
    r = g_dbus_connection_call_sync(bus, "org.bluez", "/org/bluez",
        "org.bluez.AgentManager1", "RequestDefaultAgent",
        g_variant_new("(o)", AGENT_PATH), NULL, G_DBUS_CALL_FLAGS_NONE, 3000, NULL, NULL);
    if (r) g_variant_unref(r);
    agente_ok = TRUE;
}

/* ---------- aggiornamento periodico ---------- */

static gboolean tick(gpointer d)
{
    int stato;
    char tip[80];
    GdkPixbuf *pb;
    (void)d;

    leggi();
    if (adapter[0] && !agente_ok) registra_agente();
    if (!adapter[0]) agente_ok = FALSE;
    gtk_status_icon_set_visible(icona, adapter[0] != 0);
    stato = !powered ? 0 : (nconn > 0 ? 2 : 1);
    pb = disegna(stato);
    gtk_status_icon_set_from_pixbuf(icona, pb);
    g_object_unref(pb);
    if (!powered) snprintf(tip, sizeof tip, TR("Bluetooth off", "Bluetooth spento"));
    else if (nconn > 0) snprintf(tip, sizeof tip, TR("Bluetooth: %d connected", "Bluetooth: %d connessi"), nconn);
    else snprintf(tip, sizeof tip, TR("Bluetooth on", "Bluetooth acceso"));
    gtk_status_icon_set_tooltip_text(icona, tip);
    return TRUE;
}

int main(int argc, char **argv)
{
    GError *e = NULL;
    GDBusNodeInfo *ni;

    gtk_init(&argc, &argv);
    bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &e);
    if (!bus) { fprintf(stderr, TR("System D-Bus not available\n", "D-Bus di sistema non disponibile\n")); return 1; }
    ni = g_dbus_node_info_new_for_xml(agent_xml, NULL);
    g_dbus_connection_register_object(bus, AGENT_PATH, ni->interfaces[0], &agente_vtable,
        NULL, NULL, &e);
    if (e) g_clear_error(&e);

    icona = gtk_status_icon_new();
    gtk_status_icon_set_visible(icona, FALSE);
    g_signal_connect(icona, "activate", G_CALLBACK(cb_activate), NULL);
    g_signal_connect(icona, "popup-menu", G_CALLBACK(cb_popup), NULL);
    tick(NULL);
    g_timeout_add_seconds(5, tick, NULL);
    gtk_main();
    return 0;
}
