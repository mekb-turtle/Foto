---
title: "FOTO(1)"
date: "foto-1.1.0"
---

# NAME

**foto** - Simple image viewer written in C

# SYNOPSIS

**foto** [--help] [--version] [-t title] [-c class] [\--pos x y] [\--size w h] [\--bg r g b] [\--hotreload] [\--borderless] [\--transparent] file

# DESCRIPTION

Foto is a simple image viewer in written C. It simply shows an image to the user, nothing more nothing less.

It's designed to be used in scripts so there are no keybinds.
You can specify the position or size the window intially appears at, or even the background colour.

It uses DevIL to read the image, then it uses Cairo and Xlib to display it to the user.

# OPTIONS

-h, \--help
: Shows help text

-V, \--version
: Shows the current version

-t, \--title *title*
: Sets the window title

-c, \--class *class*
: Sets the window class name

-p, \--pos *x* *y*
: Sets the window position

-s, \--size *w* *h*
: Sets the window size, defaults to the size of the image

-b, \--bg *r* *g* *b*
: Sets the background colour

-r, \--hotreload
: Reloads image when it is modified, will not work with stdin

-B, \--borderless
: Removes the border from the window

-u, \--transparent
: Makes the transparency of the window match the image

# BUGS

[Report bugs here](https://github.com/mekb-turtle/foto/issues)

# AUTHORS

mekb the turtle <mekb111@pm.me>\
CallMeEchoCodes <romanbarrettsarpi@pm.me>
