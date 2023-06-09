#!/bin/fish

complete -c foto -s h -l help -d "Shows help text"
complete -c foto -s V -l version -d "Shows the current version"
complete -c foto -s t -l title -d "Sets the window title" -x
complete -c foto -s c -l class -d "Sets the window class" -x
complete -c foto -s p -l pos -d "Sets the window position, syntax: x,y" -x
complete -c foto -s s -l size -d "Sets the window size, syntax: w,h" -x
complete -c foto -s b -l bg -d "Sets the background color, syntax: r,g,b" -x
complete -c foto -s r -l hotreload -d "Reloads image when it is modified"
complete -c foto -s B -l borderless -d "Removes the window border"
complete -c foto -s u -l transparent -d "Make the windows transparency match the image transparancy"
complete -c foto -s 1 -l sigusr1 -d "Allows the SIGUSR1 signal to resize the window to the size of the image"
complete -c foto -s 2 -l sigusr2 -d "Allows the SIGUSR2 signal to reload the image on demand"
