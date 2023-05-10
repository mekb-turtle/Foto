# <img alt="Foto" src="assets/Foto.png" width="128"/>
### Simple image viewer written in C

Foto is a simple image viewer in C. It simply shows an image to the user, it is designed to be used in scripts, there are no keybinds.
You can specify the position or size the window intially appears at, or the background colour.
It uses DevIL to read the image, and Cairo and Xlib to display the image to the user.

### Dependencies:
- `libbsd`
- `cairo`
- `devil`
- `libx11`
- `libxext`
- `pandoc` (only needed when building man pages)

### Installing:
Arch Linux AUR: [foto](https://aur.archlinux.org/packages/foto), [foto-git](https://aur.archlinux.org/packages/foto-git)

Manual:
- `git clone https://github.com/mekb-turtle/foto`
- `cd foto`
- `git checkout 1.0`\
omit if you want latest commit
- `make RELEASE=1 install`\
install will run `build` and `man`\
add `INSTALL_DIR=/path/to/dir` to specify where to install to, (`BIN_INSTALL_DIR` for binary or `MAN_INSTALL_DIR` for man pages)
- `foto /path/to/image.png`
