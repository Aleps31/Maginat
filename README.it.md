# Maginat

Maginat è una distribuzione Linux indipendente costruita da zero, seguendo
[Linux From Scratch](https://www.linuxfromscratch.org/) 12.4 (init SysV), con desktop Xfce.

English version: [README.md](README.md)

> **Stato: anteprima 0.1.** Provata in macchine virtuali e su un portatile reale (Dell Vostro 15-3568).
> Aspettati qualche asperità. Vedi [Limiti noti](#limiti-noti).

## Cosa include

- Kernel Linux 6.16.1, glibc 2.42, GCC 15.2.0
- Desktop Xfce 4.20 con LightDM
- Wi-Fi ed Ethernet con NetworkManager (icona nella barra), supporto VPN (WireGuard e OpenVPN)
- Bluetooth (BlueZ) con icona nella barra, indicatore batteria, gestione energia e audio PulseAudio
- Installazione di programmi con **Flatpak** (Flathub già configurato) e **AppImage**
- Programmi per ufficio e internet: LibreOffice, Thunderbird, Mullvad Browser, Tutanota Desktop
- Installatore grafico e strumento per scrivere su chiavetta USB
- Sessione live con **persistenza** opzionale sulla chiavetta
- Inglese e italiano, da scegliere nel primo menu di avvio

## Download

- ISO: https://github.com/Aleps31/Maginat/releases/tag/v0.1
- SHA-256: `987adaf5f162f35770eea5519a7d96860c115a61910feaaa8e71d6b90d8b1257`

Dimensione: circa 1,7 GB. Immagine ibrida: si avvia in modalità BIOS e UEFI.

## Avvio rapido

1. Scrivi la ISO su una chiavetta USB (almeno 8 GB), per esempio con:
   `sudo dd if=maginat-0.1.iso of=/dev/sdX bs=4M conv=fsync status=progress`
   (sostituisci `/dev/sdX` con la tua chiavetta; la cancella. Controlla bene il nome del dispositivo.)
2. Nelle impostazioni del firmware, **disattiva il Secure Boot** (Maginat non è firmata).
3. Avvia dalla chiavetta e scegli **Maginat 0.1** nel menu.
4. Accesso nella sessione live: utente `utente`, password `maginat` (sudo senza password solo nella live).

## Installazione su disco

Avvia l'installatore dalla sessione live (icona nel menu, oppure `sudo maginat-installa`).

> **Attenzione: l'installatore cancella tutto il disco che selezioni.** Fai prima un backup.

Il sistema installato usa nome utente e password che scegli tu; le credenziali della live vengono rimosse.

## Persistenza (conservare i file sulla chiavetta)

Di default la sessione live lavora in RAM e dimentica tutto allo spegnimento.
Per conservare file, impostazioni e Flatpak installati:

1. Avvia la live e lancia `sudo maginat-persistenza`. Crea una partizione nello spazio libero
   della chiavetta (il contenuto della ISO non viene toccato). Si fa una volta sola.
2. Riavvia e scegli **Maginat 0.1 (con persistenza)** nel menu di avvio.

## Installare programmi

Maginat non ha un gestore di pacchetti stile `apt`. Usa Flatpak:

    flatpak install flathub org.gnome.Calculator

oppure esegui direttamente gli AppImage. Nella live semplice (solo RAM) i grandi runtime Flatpak
potrebbero non starci: usa la persistenza oppure installa Maginat su disco.

## Limiti noti

- Il Secure Boot non è supportato.
- L'audio Bluetooth usa solo A2DP (cuffie e altoparlanti); i profili auricolare (HSP/HFP, microfono Bluetooth) non sono ancora supportati.
- La sessione live è in RAM, salvo persistenza.
- Serve una CPU x86-64 (x86-64-v2 o successiva).
- L'installatore usa sempre l'intero disco.

## Codice sorgente

I sorgenti del software open source di Maginat sono pubblicati in
https://github.com/Aleps31/Maginat/releases/tag/v0.1 (vedi `SOURCE-CODE.txt` dentro la ISO). Le licenze dei pacchetti sono in
`/usr/share/licenses` nel sistema.

## Licenza

Maginat è una raccolta di software con le rispettive licenze (GPL, LGPL, MPL, BSD, MIT e altre).
Ogni pacchetto mantiene la propria licenza. Vedi `/usr/share/licenses` e il repository dei sorgenti.

## Avvertenza

Maginat è un progetto indipendente. È fornito senza garanzia, così com'è.
Non è affiliato ai progetti e ai marchi citati.

## Contatti

Apri una segnalazione (issue): https://github.com/Aleps31/Maginat/issues
