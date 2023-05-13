#compdef foto

local -a options
options=(
    '-h[Shows help text]'
    '--help[Shows help text]'
    '-V[Shows the current version]'
    '--version[Shows the current version]'
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
    '-r[Reloads image when it is modified]'
    '--hotreload[Reloads image when it is modified]'
    '-B[Removes the window border]'
    '--borderless[Removes the window border]'
    '-u[Make the windows transparency match the image transparancy]'
    '--transparent[Make the windows transparency match the image transparency]'
    '-1[Allows the SIGUSR1 signal to resize the window to the size of the image]'
    '--sigusr1[Allows the SIGUSR1 signal to resize the window to the size of the image]'
    '-2[Allows the SIGUSR2 signal to reload the image on demand]'
    '--sigusr2[Allows the SIGUSR2 signal to reload the image on demand]'
)

_arguments "${options[@]}"
