<div align="center">
    <img alt="Foto" src="assets/Foto.png" width="128"/>
    <h3 align="center">A simple image viewer written in C</h3>
    <img alt="Version" src="https://img.shields.io/github/v/release/mekb-turtle/foto?display_name=tag&style=for-the-badge" />
    <img alt="Stars" src="https://img.shields.io/github/stars/mekb-turtle/foto?display_name=tag&style=for-the-badge" />
    <img alt="License" src="https://img.shields.io/github/license/mekb-turtle/foto?style=for-the-badge" />
</div>

---
<br/>

## About The Project
Foto is a simple image viewer in written C. It simply shows an image to the user, nothing more nothing less. It's designed to be used in scripts so there are no keybinds.
You can specify the position or size the window intially appears at, or even the background colour.
It uses DevIL to read the image, then it uses Cairo and Xlib to display it to the user.

<br />

## Installing
### Arch-based
```bash
# You can use an AUR helper if you like
git clone https://aur.archlinux.org/foto.git
cd foto
makepkg -si
```

### Debian-based
```bash
# This will install the latest git commit! There may be bugs.
# A DEB package is planned for an easy install
apt install build-essential libcairo2-dev libdevil-dev libx11-dev libxext-dev libbsd-dev pandoc
git clone https://github.com/mekb-turtle/foto.git
cd foto
make install RELEASE=1
```