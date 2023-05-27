#compdef foto

local -a options
options=(
    '-h[Shows help text]'
    '--help[Shows help text]'
    '-V[Shows the current version]'
    '--version[Shows the current version]'
	'-t[Sets the window title]:title:()'
	'--title[Sets the window title]:title:()'
	'-c[Sets the window class]:class:()'
	'--class[Sets the window class]:class:()'
	'-p[Sets the window position, syntax: x,y]:position:()'
	'--pos[Sets the window position, syntax: x,y]:position:()'
	'-s[Sets the window size, syntax: w,h]:size:()'
	'--size[Sets the window size, syntax: w,h]:size:()'
	'-b[Sets the background color, syntax: r,g,b]:color:()'
	'--bg[Sets the background color, syntax: r,g,b]:color:()'
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
