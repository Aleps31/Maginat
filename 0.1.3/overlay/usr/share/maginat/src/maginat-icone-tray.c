/* maginat-icone-tray: controlla se ci sono nuove icone SVG di app Flatpak e lancia la conversione in PNG
 * Fa parte di maginat-tray: icone_main() viene chiamata da maginat-tray-main.c
 * Licenza: GPL-2.0-or-later
 */
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <string.h>

static time_t icone_ultimo[2];

static time_t icone_mtime(const char *p)
{
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    return st.st_mtime;
}

static void icone_converti(void)
{
    gchar *argv[] = {"maginat-icone-flatpak", NULL};
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                  NULL, NULL, NULL, NULL);
}

static gboolean icone_controlla(gpointer d)
{
    gchar *u = g_build_filename(g_get_home_dir(), ".local/share/flatpak/exports/share/icons/hicolor/scalable/apps", NULL);
    time_t a = icone_mtime(u), b = icone_mtime("/var/lib/flatpak/exports/share/icons/hicolor/scalable/apps");
    (void)d;
    g_free(u);
    if (a != icone_ultimo[0] || b != icone_ultimo[1]) {
        icone_ultimo[0] = a;
        icone_ultimo[1] = b;
        icone_converti();
    }
    return G_SOURCE_CONTINUE;
}

static gboolean icone_avvio(gpointer d)
{
    (void)d;
    icone_controlla(NULL);
    g_timeout_add_seconds(20, icone_controlla, NULL);
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    g_timeout_add_seconds(12, icone_avvio, NULL);
    gtk_main();
    return 0;
}
