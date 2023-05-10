_foto() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="-t --title -c --class -p --pos -s --size -b --bg -h --hotreload -B --borderless -u --transparent"

    case "${prev}" in
        -t|--title)
            COMPREPLY=()
            return 0
            ;;
        -c|--class)
            COMPREPLY=()
            return 0
            ;;
        -p|--pos)
            COMPREPLY=()
            return 0
            ;;
        -s|--size)
            COMPREPLY=()
            return 0
            ;;
        -b|--bg)
            COMPREPLY=()
            return 0
            ;;
        -h|--hotreload)
            COMPREPLY=()
            return 0
            ;;
        -B|--borderless)
            COMPREPLY=()
            return 0
            ;;
        -u|--transparent)
            COMPREPLY=()
            return 0
            ;;
        *)
            COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
            return 0
            ;;
    esac
}
complete -F _foto foto