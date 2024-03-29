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
Foto is a simple image viewer written in C. It simply shows an image to the user, nothing more nothing less. It's designed to be used in scripts so there are no keybinds.
You can specify the position or size the window intially appears at, and the background colour.

<br />

## Installing
### Arch-based
If you use an AUR helper, use it instead, e.g `paru -S foto` or `paru -S foto-git`
```bash
git clone https://aur.archlinux.org/foto.git # use foto-git.git instead for latest commit
cd foto
makepkg -si
```

### Debian-based
```bash
sudo apt install build-essential libsdl2-dev libsdl2-image-dev pandoc
git clone https://github.com/mekb-turtle/foto.git
cd foto
git checkout "$(git describe --tags --abbrev=0)" # checkout to latest tag, omit for latest commit
sudo make install RELEASE=1
```

### Other distros
Find the following dependencies in your package manager or elsewhere:
- `sdl2`
- `sdl2_image`
- `pandoc` (only needed when building man pages)

```bash
git clone https://github.com/mekb-turtle/foto.git
cd foto
git checkout "$(git describe --tags --abbrev=0)" # checkout to latest tag, omit for latest commit
sudo make install RELEASE=1
```

Specify `INSTALL_DIR` in `make install` to specify where to install to, read `Makefile` for more information
