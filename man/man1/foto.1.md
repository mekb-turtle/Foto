% FOTO(1)
% mekb the turtle <mekb111@pm.me>
% foto-1.0

# NAME

**foto** - Simple image viewer written in C

# SYNOPSIS

**foto** [-t title] [-c class] [\--pos x y] [\--size w h] [\--bg r g b] [\--hotreload] [\--borderless] [\--transparent] file

# DESCRIPTION

Foto is a simple image viewer in C. It simply shows an image to the user, it is designed to be used in scripts, there are no keybinds.

You can specify the position or size the window intially appears at, or the background colour.

It uses DevIL to read the image, and Cairo and Xlib to display the image to the user.

# OPTIONS

-t, \--title *title*
: set window title

-c, \--class *class*
: set window class name

-p, \--pos *x* *y*
: set window position, default to center

-s, \--size *w* *h*
: set window size, default to image size

-b, \--bg *r* *g* *b*
: set background colour where there is transparency

-h, \--hotreload
: reload image when it is changed, will not work with stdin

-B, \--borderless
: remove the border from the window

-u, \--transparent
: window's transparency will match the image

# BUGS

[Report bugs here](https://github.com/mekb-turtle/foto/issues)
