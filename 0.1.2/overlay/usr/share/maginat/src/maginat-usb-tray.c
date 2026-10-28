/* maginat-usb-tray: avvisa quando si collega o si toglie una chiavetta o un disco USB
 * Fa parte di maginat-tray: usb_main() viene chiamata da maginat-tray-main.c
 * Licenza: GPL-2.0-or-later
 */
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static int usb_is_it(void) { const char *l = getenv("LANG"); return l && l[0] == 'i' && l[1] == 't'; }
#define UTR(en, it) (usb_is_it() ? (it) : (en))

static void usb_avvisa(const char *titolo, const char *testo, const char *icona)
{
    gchar *argv[] = {"notify-send", "-a", "Maginat", "-i", (gchar *)icona, "-t", "6000", (gchar *)titolo, (gchar *)testo, NULL};
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                  NULL, NULL, NULL, NULL);
}

static gboolean usb_rimovibile(GDrive *d)
{
    if (!d) return FALSE;
    return g_drive_is_removable(d) || g_drive_is_media_removable(d) || g_drive_can_eject(d);
}

static void usb_montato(GVolumeMonitor *vm, GMount *m, gpointer dati)
{
    GDrive *d;
    gchar *nome;
    gchar *testo;
    (void)vm; (void)dati;
    if (!m || g_mount_is_shadowed(m)) return;
    d = g_mount_get_drive(m);
    if (!usb_rimovibile(d)) { if (d) g_object_unref(d); return; }
    g_object_unref(d);
    nome = g_mount_get_name(m);
    testo = g_strdup_printf("%s\n%s", nome ? nome : "USB",
        UTR("Open it from Files or from the desktop.", "Aprila da File o dal desktop."));
    usb_avvisa(UTR("USB drive connected", "Unità USB collegata"), testo, "drive-removable-media-usb");
    g_free(testo);
    g_free(nome);
}

static void usb_scollegata(GVolumeMonitor *vm, GDrive *d, gpointer dati)
{
    gchar *nome;
    (void)vm; (void)dati;
    if (!usb_rimovibile(d)) return;
    nome = g_drive_get_name(d);
    usb_avvisa(UTR("USB drive removed", "Unità USB rimossa"), nome ? nome : "USB", "drive-removable-media-usb");
    g_free(nome);
}

static void usb_mount_fine(GObject *src, GAsyncResult *res, gpointer dati)
{
    GError *err = NULL;
    (void)dati;
    g_volume_mount_finish(G_VOLUME(src), res, &err);
    if (err) g_error_free(err);
}

static gboolean usb_prova_montare(gpointer dati)
{
    GVolume *v = G_VOLUME(dati);
    GDrive *d = g_volume_get_drive(v);
    GMount *m = g_volume_get_mount(v);
    if (m) {
        g_object_unref(m);
    } else if (usb_rimovibile(d) && g_volume_can_mount(v)) {
        g_volume_mount(v, G_MOUNT_MOUNT_NONE, NULL, NULL, usb_mount_fine, NULL);
    }
    if (d) g_object_unref(d);
    g_object_unref(v);
    return G_SOURCE_REMOVE;
}

static void usb_volume_nuovo(GVolumeMonitor *vm, GVolume *v, gpointer dati)
{
    (void)vm; (void)dati;
    if (!v) return;
    g_timeout_add(1500, usb_prova_montare, g_object_ref(v));
}

int main(int argc, char **argv)
{
    GVolumeMonitor *vm;
    gtk_init(&argc, &argv);
    vm = g_volume_monitor_get();
    g_signal_connect(vm, "mount-added", G_CALLBACK(usb_montato), NULL);
    g_signal_connect(vm, "drive-disconnected", G_CALLBACK(usb_scollegata), NULL);
    g_signal_connect(vm, "volume-added", G_CALLBACK(usb_volume_nuovo), NULL);
    gtk_main();
    return 0;
}
