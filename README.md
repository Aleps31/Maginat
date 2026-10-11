# Maginat

Maginat is an independent Linux distribution built from scratch, following
[Linux From Scratch](https://www.linuxfromscratch.org/) 12.4 (SysV init), with the Xfce desktop.

Italian version: [README.it.md](README.it.md)

> **Status: 0.1 preview.** Tested in virtual machines and on one real laptop (Dell Vostro 15-3568).
> Expect rough edges. See [Known limitations](#known-limitations).

## What is included

- Linux kernel 6.16.1, glibc 2.42, GCC 15.2.0
- Xfce 4.20 desktop with LightDM
- Wi-Fi and Ethernet via NetworkManager (tray applet), VPN support (WireGuard and OpenVPN)
- Bluetooth (BlueZ) with a tray icon, battery indicator and power management, PulseAudio sound
- Software installation through **Flatpak** (Flathub is preconfigured) and **AppImage**
- Office and internet apps: LibreOffice, Thunderbird, Mullvad Browser, Tutanota Desktop
- Graphical installer and a USB-writing tool
- Live session with optional **persistence** on the USB stick
- English and Italian, chosen in the first boot menu

## Download

- ISO: https://github.com/Aleps31/Maginat/releases/tag/v0.1
- SHA-256: `987adaf5f162f35770eea5519a7d96860c115a61910feaaa8e71d6b90d8b1257`

Size: about 1.7 GB. Hybrid image: boots in BIOS and UEFI mode.

## Quick start

1. Write the ISO to a USB stick (at least 8 GB), for example with:
   `sudo dd if=maginat-0.1.iso of=/dev/sdX bs=4M conv=fsync status=progress`
   (replace `/dev/sdX` with your stick; this erases it. Double-check the device name.)
2. In your firmware settings, **turn Secure Boot off** (Maginat is not signed).
3. Boot from the stick and choose **Maginat 0.1** in the menu.
4. Live session login: user `utente`, password `maginat` (sudo without password in the live session only).

## Installing to disk

Start the installer from the live session (menu launcher, or `sudo maginat-installa`).

> **Warning: the installer erases the entire disk you select.** Back up your data first.

The installed system uses the user name and password you choose; the live defaults are removed.

## Persistence (keep files on the USB stick)

By default the live session runs in RAM and forgets everything at shutdown.
To keep your files, settings and installed Flatpaks:

1. Boot the live session and run `sudo maginat-persistenza`. It creates a partition in the free
   space of the stick (the ISO content is not touched). Do this once.
2. Reboot and choose **Maginat 0.1 (with persistence)** in the boot menu.

## Installing software

Maginat has no `apt`-style package manager. Use Flatpak:

    flatpak install flathub org.gnome.Calculator

or run AppImages directly. In the plain live session (RAM only) large Flatpak runtimes may not
fit; use persistence or install Maginat to disk.

## Known limitations

- Secure Boot is not supported.
- Bluetooth audio uses A2DP only (headphones and speakers); headset profiles (HSP/HFP, microphone over Bluetooth) are not supported yet.
- The live session is in RAM unless persistence is set up.
- Requires an x86-64 CPU (x86-64-v2 or newer).
- The installer always uses the whole disk.

## Source code

The source code of the open source software in Maginat is published in
https://github.com/Aleps31/Maginat/releases/tag/v0.1 (see `SOURCE-CODE.txt` inside the ISO). Package licenses are in
`/usr/share/licenses` in the system.

## License

Maginat is a collection of software under their own licenses (GPL, LGPL, MPL, BSD, MIT and others).
Each package keeps its license. See `/usr/share/licenses` and the source repository.

## Disclaimer

Maginat is an independent project. It is provided without warranty, as is.
It is not affiliated with the projects and trademarks mentioned here.

## Contact

Open an issue: https://github.com/Aleps31/Maginat/issues

## Credits

The basic system, desktop integration, and 'python' coded parts were built by Aleps31 following the book Linux From Scratch and captured encodings The additional software and installer were developed with Claude (Anthropic).
