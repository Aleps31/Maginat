# Changelog

Italiano · [English](#english) · [Español](#español)

## Italiano

### 0.1.3
- **Secure Boot**: shim e GRUB firmati (Ubuntu 24.04), kernel firmato con la chiave di Maginat; la chiave si registra una volta per computer. Lockdown del kernel.
- **Kernel Linux 6.18.62 LTS**, con USB Attached SCSI e i driver Wi-Fi Broadcom più vecchi (b43, brcmsmac).
- **Installatore flessibile**: intero disco, partizione esistente, spazio libero accanto a un altro sistema (dual boot) o installazione portatile su chiavetta USB.
- **Riduzione di una partizione** *(sperimentale)*: l'installatore può rimpicciolire una partizione Windows (NTFS) o Linux (ext2/3/4) per fare spazio; rifiuta BitLocker, Windows ibernato e file system con errori.
- **Cifratura del disco (LUKS2)** facoltativa in tutte le modalità, con **chiave di recupero** da 16 caratteri (si può scrivere con o senza spazi).
- **Aggiornamenti di sicurezza** inclusi nella ISO: kernel 6.18.62, OpenSSL 3.5.9, curl 8.23.0, OpenSSH 10.6p2, libpng 1.6.59, xz 5.8.4, git 2.47.3, zlib 1.3.2, Python 3.13.18.
- **Aggiornamenti del sistema** firmati, kernel compreso: kernel precedente come ripiego, **Annulla l'ultimo aggiornamento**, controllo giornaliero con notifica.
- **Nuove versioni di Maginat senza nuova ISO**: Aggiorna Maginat può portare la 0.1.3 alle versioni successive con un pacchetto firmato; le impostazioni che hai modificato restano (la versione nuova si salva accanto come `.nuovo-…`) e l'aggiornamento si può annullare
- **Spagnolo** (interfaccia, app, giochi, installatore, tastiera) accanto a italiano e inglese.
- **Sessione live** direttamente sul desktop, installatore con presentazione e barra di avanzamento vera, nuovo menu di avvio.
- **Wi-Fi Broadcom con b43**: avviso quando manca il firmware e utilità "Firmware Wi-Fi Broadcom" che lo scarica sul tuo computer.
- **Plug-in stampanti HP**: utilità nel menu che scarica il plug-in proprietario di HP (richiesto da alcune stampanti), ne controlla firma e integrità, mostra la licenza di HP e lo installa. Non ancora provata con una stampante HP vera.
- **Icone dei programmi Flatpak** mostrate correttamente nel menu, nel Negozio e nel dock.
- Driver di stampa **HPLIP** (solo stampa), **brlaser** (Brother), **splix** (Samsung, Xerox, Dell, Lexmark, Toshiba).
- Widget del desktop ridisegnati, finestre e menu chiari, 44 sfondi, nuova grafica per Ventuno e Poker, "M" animata prima del login.
- Thunderbird passa dalla ISO al Negozio; ISO di circa 1,4 GB.

### 0.1.2
- Kernel 6.18 LTS e **aggiornamenti di sistema** firmati (kernel e pacchetti, senza reinstallare).
- "M" di avvio animata, icone di sistema personalizzate, sito e documentazione nuovi.

### 0.1.1
- Stampa (CUPS) e stampanti di rete, Bluetooth con cuffie, firewall (nftables), orologio (chrony), grafica Mesa/LLVM.
- Microcode della CPU aggiornato all'avvio.

### 0.1
- Prima versione pubblica: Linux From Scratch 12.4, SysV init, Xfce, installatore grafico per BIOS e UEFI.

## English

### 0.1.3
- **Secure Boot**: signed shim and GRUB (Ubuntu 24.04), kernel signed with the Maginat key; enroll the key once per computer. Kernel lockdown.
- **Linux kernel 6.18.62 LTS**, with USB Attached SCSI and the older Broadcom Wi-Fi drivers (b43, brcmsmac).
- **Flexible installer**: whole disk, existing partition, free space next to another system (dual boot) or portable installation on a USB drive.
- **Partition shrinking** *(experimental)*: the installer can shrink a Windows (NTFS) or Linux (ext2/3/4) partition to make room; it refuses BitLocker, hibernated Windows and file systems with errors.
- Optional **disk encryption (LUKS2)** in every mode, with a 16-character **recovery key** (type it with or without spaces).
- **Security updates** in the ISO: kernel 6.18.62, OpenSSL 3.5.9, curl 8.23.0, OpenSSH 10.6p2, libpng 1.6.59, xz 5.8.4, git 2.47.3, zlib 1.3.2, Python 3.13.18.
- Signed **system updates**, kernel included: previous kernel kept as fallback, **Undo last update**, daily check with notification.
- **New Maginat versions without a new ISO**: Update Maginat can move 0.1.3 to later versions with a signed package; settings you changed are kept (the new version is saved next to them as `.nuovo-…`) and the update can be undone
- **Spanish** (interface, apps, games, installer, keyboard) next to Italian and English.
- **Live session** straight to the desktop, installer with slideshow and real progress bar, new boot menu.
- **Broadcom Wi-Fi with b43**: a warning when the firmware is missing and a "Broadcom Wi-Fi firmware" tool that downloads it to your computer.
- **HP printer plug-in**: a menu tool that downloads HP's proprietary plug-in (needed by some printers), checks its signature and integrity, shows HP's license and installs it. Not yet tested with a real HP printer.
- **Flatpak app icons** now show correctly in the menu, the Store and the dock.
- Printer drivers: **HPLIP** (print only), **brlaser** (Brother), **splix** (Samsung, Xerox, Dell, Lexmark, Toshiba).
- Redesigned desktop widgets, light windows and menus, 44 wallpapers, new Blackjack and Poker graphics, animated "M" before login.
- Thunderbird moved from the ISO to the Store; ISO of about 1.4 GB.

### 0.1.2
- Kernel 6.18 LTS and signed **system updates** (kernel and packages, no reinstall).
- Animated boot "M", custom system icons, new website and documentation.

### 0.1.1
- Printing (CUPS) and network printers, Bluetooth headsets, firewall (nftables), clock (chrony), Mesa/LLVM graphics.
- CPU microcode updated at boot.

### 0.1
- First public release: Linux From Scratch 12.4, SysV init, Xfce, graphical installer for BIOS and UEFI.

## Español

### 0.1.3
- **Secure Boot**: shim y GRUB firmados (Ubuntu 24.04), kernel firmado con la clave de Maginat; la clave se registra una vez por equipo. Lockdown del kernel.
- **Kernel Linux 6.18.62 LTS**, con USB Attached SCSI y los controladores Wi-Fi Broadcom más antiguos (b43, brcmsmac).
- **Instalador flexible**: disco entero, partición existente, espacio libre junto a otro sistema (arranque dual) o instalación portátil en una memoria USB.
- **Reducción de una partición** *(experimental)*: el instalador puede reducir una partición Windows (NTFS) o Linux (ext2/3/4) para hacer espacio; rechaza BitLocker, Windows hibernado y sistemas de archivos con errores.
- **Cifrado de disco (LUKS2)** opcional en todos los modos, con **clave de recuperación** de 16 caracteres (se puede escribir con o sin espacios).
- **Actualizaciones de seguridad** en la ISO: kernel 6.18.62, OpenSSL 3.5.9, curl 8.23.0, OpenSSH 10.6p2, libpng 1.6.59, xz 5.8.4, git 2.47.3, zlib 1.3.2, Python 3.13.18.
- **Actualizaciones del sistema** firmadas, kernel incluido: kernel anterior como respaldo, **Deshacer la última actualización**, comprobación diaria con notificación.
- **Versiones nuevas de Maginat sin nueva ISO**: Actualizar Maginat puede llevar la 0.1.3 a las versiones siguientes con un paquete firmado; los ajustes que hayas cambiado se conservan (la versión nueva se guarda al lado como `.nuovo-…`) y la actualización se puede deshacer
- **Español** (interfaz, aplicaciones, juegos, instalador, teclado) junto a italiano e inglés.
- **Sesión live** directa al escritorio, instalador con presentación y barra de progreso real, nuevo menú de arranque.
- **Wi-Fi Broadcom con b43**: aviso cuando falta el firmware y utilidad "Firmware Wi-Fi Broadcom" que lo descarga en tu ordenador.
- **Complemento de impresoras HP**: utilidad en el menú que descarga el complemento propietario de HP (necesario para algunas impresoras), comprueba su firma e integridad, muestra la licencia de HP y lo instala. Aún no probada con una impresora HP real.
- **Iconos de las aplicaciones Flatpak** que ahora se ven bien en el menú, la Tienda y el dock.
- Controladores de impresión: **HPLIP** (solo impresión), **brlaser** (Brother), **splix** (Samsung, Xerox, Dell, Lexmark, Toshiba).
- Widgets del escritorio rediseñados, ventanas y menús claros, 44 fondos, nuevos gráficos de Veintiuna y Póker, "M" animada antes del inicio de sesión.
- Thunderbird pasa de la ISO a la Tienda; ISO de unos 1,4 GB.

### 0.1.2
- Kernel 6.18 LTS y **actualizaciones del sistema** firmadas (kernel y paquetes, sin reinstalar).
- "M" de arranque animada, iconos de sistema personalizados, nuevo sitio web y documentación.

### 0.1.1
- Impresión (CUPS) e impresoras de red, auriculares Bluetooth, cortafuegos (nftables), reloj (chrony), gráficos Mesa/LLVM.
- Microcódigo de la CPU actualizado al arrancar.

### 0.1
- Primera versión pública: Linux From Scratch 12.4, SysV init, Xfce, instalador gráfico para BIOS y UEFI.
