#compdef foto

local -a options
options=(
	'(-h --help)'{-h,--help}'[Shows help text]'
	'(-V --version)'{-V,--version}'[Shows the current version]'
	'(-t --title)'{-t,--title}'[Sets the window title]:title:()'
	'(-p --position)'{-p,--position}'[Sets the window position: x,y]:position:()'
	'(-s --size)'{-s,--size}'[Sets the window size: w,h]:size:()'
	'(-b --background)'{-b,--background}'[Sets the background color: r,g,b]:color:()'
	'(-S --stretch)'{-S,--stretch}'[Allows the image to stretch instead of fitting to the window]'
	'(-r --hotreload)'{-r,--hotreload}'[Reloads image when it is modified]'
	'(-1 --sigusr1)'{-1,--sigusr1}'[Allows the SIGUSR1 signal to resize the window to the size of the image]'
	'(-2 --sigusr2)'{-2,--sigusr2}'[Allows the SIGUSR2 signal to reload the image on demand]'
)

_arguments -s -w "${options[@]}"
