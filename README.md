# Maginat

The folder `0.1.1/` contains the new and changed files of version 0.1.1: `overlay/` (scripts, C sources, configuration files) and `build/` (image and ISO build scripts).

Maginat is an independent Linux distribution built from scratch, following
[Linux From Scratch](https://www.linuxfromscratch.org/) 12.4 (SysV init), with the Xfce desktop.

Italian version: [README.it.md](README.it.md)

> **Status: 0.1.1 preview.** Tested in virtual machines and on one real laptop (Dell Vostro 15-3568).
> Expect rough edges. See [Known limitations](#known-limitations).

## Screenshots

![Desktop](screenshots/desktop-0.1.1.png)

![Settings menu](screenshots/impostazioni-0.1.1.png)

![Maginat Store](screenshots/negozio-0.1.1.png)

## What is included

- Xfce desktop, with a custom theme, a top bar (volume, network, notifications, printers, battery, Bluetooth) and a dock
- Mullvad Browser, Thunderbird, LibreOffice and Tutanota Desktop
- Printing and scanning with graphical tools (CUPS, SANE)
- Wi-Fi, Bluetooth, sound and power management
- Firewall enabled by default
- Maginat Store: a graphical tool to install extra applications
- Graphical installer for BIOS and UEFI computers, with Italian and English interface

## What is new in 0.1.1

- Firewall enabled by default (nftables): blocks incoming connections
- Updated graphics drivers (Mesa 25.1.8 with LLVM 20): Intel (iris, crocus, i915), AMD (radeonsi, r300, r600), NVIDIA (nouveau), virtual machines (virgl) and software rendering (llvmpipe)
- Intel and AMD CPU microcode loaded early at boot
- Automatic time synchronization (chrony)
- Printers and scanners: new graphical tools, launcher in the top bar
- Screen lock (Applications, Log Out, Lock Screen)
- Weekly automatic TRIM for SSDs (installed system only)
- Installation on a whole disk or on an existing partition, BIOS and UEFI
- The "Install Maginat" icon no longer appears on the installed system

Tested on QEMU (BIOS and UEFI installs) and on a Dell Vostro 15-3568 (Intel HD Graphics 620), live: Wi-Fi, Bluetooth, battery, graphics, brightness, suspend and microcode update.

## Download

Get the ISO from the [Releases](../../releases) page and check it against the SHA-256 shown in the release notes.

## Try it or install it

Write the ISO to a USB stick and boot from it. Choose "Try Maginat without installing" to use the live system, or "Install Maginat on the disk" to install it. The installer can use a whole disk (erasing it) or an existing partition. Secure Boot must be turned off.

## Known limitations

- No Secure Boot
- No "safe graphics" boot entry (planned for 0.1.2)
- Log out in the live session may leave the screen black until a key is pressed
- Volume keys (Fn) do not change the volume; use the tray icon
- The battery icon is an indicator only
- No proprietary printer drivers (HPLIP), no driverless USB printing (ipp-usb) or network scanning (airscan)
- Experimental: try it in a virtual machine or from the live USB first

## Source code

The folders `build/` and `system/` contain the build scripts and the customization files. The source archives of the packages are attached to each release: `maginat-0.1-sources.tar` and `maginat-0.1.1-sources.tar` (new and changed packages). The license of each package is shipped in the ISO, under `/usr/share/licenses`.

Programs distributed as official binaries, whose source code is available from the original projects:
- LibreOffice 26.8.1 (MPL-2.0): https://www.libreoffice.org/download/source-code/
- Mullvad Browser 15.0.24: https://github.com/mullvad/mullvad-browser
- Thunderbird 157.0.1 (MPL-2.0): https://ftp.mozilla.org/pub/thunderbird/releases.

## Credits

Created by Aleps31: the Linux From Scratch base, the Python code and the desktop integration. Scripts developed with Claude (Anthropic).

## License

The original code in this repository is released under the GPL-2.0 (see `LICENSE`). Each package keeps its own license.

## Disclaimer

Maginat is an independent project and is not affiliated with the projects listed above.
