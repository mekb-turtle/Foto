#!/usr/bin/bash

_foto_completion() {
	local opts3 opts2 opts1 opts0 opts
	local oldifs="$IFS"
	local isoldifs=
	if [ -v IFS ]; then isoldifs=true; fi

	function _foto_completion_contains_array() {
		local a="$1"
		local j
		shift
		for j in "$@"; do
			[[ "$a" == "$j" ]] && return 0
		done
		return 1
	}

	opts3=('-b' '--bg')
	opts2=("${opts3[@]}" '-p' '--pos' '-s' '--size')
	opts1=("${opts2[@]}" '-t' '--title' '-c' '--class' '-p' '--pos' '-s' '--size' '-b' '--bg')
	opts0=('-h' '--help' '-V' '--version' '-1' '--sigusr1' '-2' '--sigusr2')

	if _foto_completion_contains_array "${COMP_WORDS[COMP_CWORD-3]}" "${opts3[@]}" || \
		_foto_completion_contains_array "${COMP_WORDS[COMP_CWORD-2]}" "${opts2[@]}" || \
		_foto_completion_contains_array "${COMP_WORDS[COMP_CWORD-1]}" "${opts1[@]}"; then
		return
	fi

	local flag_done=
	for (( i=0; i<"${#COMP_WORDS[@]}"; i++ )); do
		if [[ "${COMP_WORDS[i]}" == "--" ]]; then flag_done=1; fi
		if [[ "$i" -eq "$COMP_CWORD" ]]; then break; fi
	done

	IFS=" "
	opts="-"
	if [[ -z "$flag_done" ]]; then
		opts="${opts3[*]} ${opts2[*]} ${opts1[*]} ${opts0[*]} -- $opts"
	fi
	IFS=$'\n'
	COMPREPLY=($(IFS=" " compgen -W "${opts}" -f -- "${COMP_WORDS[COMP_CWORD]}"))

	unset IFS
	if [[ -n "$isoldifs" ]]; then IFS="$oldifs"; fi
}

complete -F _foto_completion -o filenames -o nosort foto
