# Maginat

Distribuzione Linux indipendente costruita da zero seguendo **Linux From Scratch 12.4** (init SysV, senza systemd), con il desktop **Xfce**.

**Ultima versione: [0.1.2](https://github.com/Aleps31/Maginat/releases/tag/v0.1.2)** (pre-release) · Sito: https://aleps31.github.io/Maginat/ · English: [README.md](README.md)

## Stato

Maginat 0.1.2 è un'anteprima. È stata provata in macchine virtuali e su un portatile (Dell Vostro 15-3568, sessione live da USB).

## Schermate

![Desktop](screenshots/desktop-0.1.2.png)
![Menu dei widget](screenshots/widget-0.1.2.png)
![Negozio Maginat](screenshots/negozio-0.1.2.png)
![Giochi](screenshots/giochi-0.1.2.png)

## Cosa include

- Xfce con tema, cursori, barra in alto e dock personalizzati
- Mullvad Browser, Thunderbird, LibreOffice, Tutanota Desktop, mtPaint
- Widget del desktop a scelta: orologio, calendario, meteo, monitor di sistema, batteria e rete, collegamenti rapidi, note
- Stampa e scansione (anche stampanti e scanner senza driver, USB e di rete)
- Wi-Fi, Bluetooth, audio e gestione energia; firewall attivo di default
- Negozio Maginat (Flatpak/Flathub) e Aggiorna Maginat
- Giochi Ventuno e Poker
- Installatore grafico per BIOS e UEFI (italiano e inglese): scelta di tastiera, fuso orario, nome utente, password e di un intero disco o di una sola partizione

## Novità della 0.1.2

- Kernel Linux 6.18.58 LTS (era 6.16.1) con nuovi driver
- ISO ridotta a circa 1,4 GB (era 2,1 GB); il compilatore è un download facoltativo dal Negozio
- Circa 460 MB di RAM a riposo (live su hardware reale)
- Widget del desktop, barra in alto unificata (volume, luminosità, batteria, Bluetooth), cursori propri, schermata di avvio animata, 26 sfondi
- Voce di avvio "grafica sicura" ora funzionante
- Tasti Fn del volume funzionanti; l'icona della batteria apre le impostazioni di risparmio energia
- Aggiorna Maginat: aggiorna le app Flatpak, controlla le nuove release su GitHub e installa aggiornamenti di sistema firmati (kernel e pacchetti) su Maginat installata
- Negozio più ricco; nuovi Ventuno e Poker con animazioni e mano automatica
- Stampa e scansione senza driver (ipp-usb, sane-airscan)
- Dischi USB di ogni formato (anche NTFS ed exFAT) montati da soli, con notifica all'inserimento; sospensione automatica con batteria quasi scarica

## Download e installazione

Scarica la ISO dalla pagina [Releases](https://github.com/Aleps31/Maginat/releases), verifica lo SHA-256 (`sha256sum -c SHA256SUMS`), scrivila su una chiavetta USB e avvia. Il Secure Boot va disattivato.

| File | Note |
|---|---|
| maginat-0.1.2.iso | live + installer avviabile |
| maginat-sviluppo-0.1.2.tar.xz | strumenti di sviluppo, scaricati dal Negozio (non rinominare) |
| maginat-0.1.2-sources.tar | sorgenti dei pacchetti nuovi o cambiati, configurazione del kernel |
| SHA256SUMS | checksum |

## Limiti noti

- Niente Secure Boot (va disattivato nel BIOS/UEFI per avviare la ISO)
- Aggiornamenti di sistema (kernel e pacchetti, firmati) da Aggiorna Maginat su sistema installato; per una nuova versione di Maginat si scarica la nuova ISO. Anche le app del Negozio si aggiornano da Aggiorna Maginat
- Alcune icone Flatpak possono apparire generiche (manca ancora il supporto alle icone SVG) e le icone della barra non usano ancora lo standard StatusNotifier
- Niente driver stampante proprietari (per esempio HPLIP): funzionano le stampanti con IPP e le scansioni senza driver
- Solo X11, senza Wayland

## Codice sorgente

Gli script di costruzione e la configurazione sono nelle cartelle di ogni versione (`0.1/`: `build/` e `system/`; `0.1.1/` e `0.1.2/`: script in `build/` e file in `overlay/`). Gli archivi dei sorgenti sono allegati a ogni release: i sorgenti di base nella release 0.1, i pacchetti più recenti in quelle successive. I binari ufficiali provengono dai progetti originali elencati nell'archivio dei sorgenti.

## Crediti

Creato da Aleps31. La base LFS, il codice Python e l'integrazione del desktop sono opera dell'autore; gli script sono stati sviluppati con Claude (Anthropic).

## Licenza

Codice originale: GPL-2.0 (vedi `LICENSE`). Ogni pacchetto mantiene la propria licenza (vedi `LICENZE.txt` nel sistema).

## Avvertenza

Maginat non è affiliata ai progetti che include.
