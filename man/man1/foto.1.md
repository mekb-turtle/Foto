---
title: "FOTO(1)"
date: "foto-INSERT_VERSION_HERE"
---

# NAME

**foto** - Simple image viewer written in C

# SYNOPSIS

**foto** [OPTION]... [FILE]

# DESCRIPTION

A simple image viewer written in C. It simply shows an image to the user, nothing more nothing less. It's designed to be used in scripts so there are no keybinds.

# OPTIONS

-h, \--help
: Shows help text

-V, \--version
: Shows the current version

-t, \--title *title*
: Sets the window title

-p, \--position *x*,*y*
: Sets the window position

-s, \--size *w*,*h*
: Sets the window size, defaults to the size of the image

-b, \--background *r*,*g*,*b*
: Sets the background colour (0-255)

-S, \--stretch
: Allows the image to stretch instead of fitting to the window

-r, \--hotreload
: Reloads image when it is modified, will not work with stdin

-1, \--sigusr1
: Allows the SIGUSR1 signal to resize the window to the size of the image

-2, \--sigusr2
: Allows the SIGUSR2 signal to reload the image on demand

# BUGS

[Report bugs here](https://github.com/mekb-turtle/foto/issues)

# AUTHORS

mekb <mekb111@pm.me>\
CallMeEchoCodes <romanbarrettsarpi@pm.me>
