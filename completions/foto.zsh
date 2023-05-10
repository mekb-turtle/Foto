#compdef foto

local -a options
options=(
    '-t[Sets the window title]'
    '--title[Sets the window title]'
    '-c[Sets the window class]'
    '--class[Sets the window class]'
    '-p[Sets the window position]'
    '--pos[Sets the window position]'
    '-s[Sets the window size]'
    '--size[Sets the window size]'
    '-b[Sets the background color]'
    '--bg[Sets the background color]'
    '-h[Reload the image when it is modified]'
    '--hotreload[Reload the image when it is modified]'
    '-B[Remove the window border]'
    '--borderless[Remove the window border]'
    '-u[Make the windows transparency match the image transparency]'
    '--transparent[Make the windows transparency match the image transparency]'
)

_arguments "${options[@]}"
