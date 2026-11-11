/* maginat-tray: avvia in un solo processo le icone di volume, luminosita, Bluetooth e batteria
 * Licenza: GPL-2.0-or-later
 */
#include <gtk/gtk.h>
int vol_main(int argc, char **argv);
int lum_main(int argc, char **argv);
int bt_main(int argc, char **argv);
int batt_main(int argc, char **argv);
int wid_main(int argc, char **argv);
int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    vol_main(argc, argv);
    lum_main(argc, argv);
    bt_main(argc, argv);
    batt_main(argc, argv);
    wid_main(argc, argv);
    gtk_main();
    return 0;
}
