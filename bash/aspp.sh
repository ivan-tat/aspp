#!/bin/bash
#
# aspp - Simple assembler source file preprocessor.
#
# Author: Ivan Tatarinov, <ivan-tat@ya.ru>, 2019-2020.
#
# This is free and unencumbered software released into the public domain.
# For more information, please refer to <http://unlicense.org>.
#
# Home page: <https://gitlab.com/ivan-tat/aspp>
#
set -e

PROGRAM_NAME=\
'aspp'
PROGRAM_VERSION=\
'0.1'
PROGRAM_DESCRIPTION=\
'Simple assembler source file preprocessor.'
PROGRAM_LICENSE=\
'License: public domain, <http://unlicense.org>
This is free software; you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.'
PROGRAM_AUTHORS=\
'Author: Ivan Tatarinov, <ivan-tat@ya.ru>, 2019-2020.'
PROGRAM_CONTACTS=\
'Home page: <https://gitlab.com/ivan-tat/aspp>'
HELP_HINT=\
"Use '-h' or '--help' to get help."

DEBUG=0

# Naming convention:
#   *_real - real file/path name
#   *_base - real file/path base name
#   *_user - user file/path name

declare -a errors
declare    v_act
declare -i v_act_show_help
declare -i v_act_preprocess
declare -i v_act_make_rule
declare    v_base_path_real
declare -a v_include_paths_real
declare -a v_include_paths_base
declare -a v_include_paths_user
declare -a v_input_sources_real
declare -a v_input_sources_base
declare -a v_input_sources_user
declare -a v_sources_real
declare -a v_sources_base
declare -a v_sources_user
declare -a v_sources_parse  # 1 (must be parsed) or 0 (no parsing)
declare -a v_target_names
declare    v_output_name
declare -a v_prerequisites
declare    inc_real
declare    inc_base
declare    inc_user

# $1=message
_dbg() {
    if [[ $DEBUG -ne 0 ]]; then
        echo "DEBUG:${FUNCNAME[1]}():${BASH_LINENO[0]}: $1" >&2
    fi
}

_dbg_dump_vars() {
    local -i i N
    if [[ -n $DEBUG ]]; then
        i=0
        N=${#v_include_paths_real[@]}
        if [[ $N -eq 0 ]]; then _dbg "No include paths."; fi
        while [[ $i -lt $N ]]; do
            _dbg "Include path #$i: user path = '${v_include_paths_user[$i]}'"
            _dbg "Include path #$i: base path = '${v_include_paths_base[$i]}'"
            _dbg "Include path #$i: real path = '${v_include_paths_real[$i]}'"
            let i=i+1
        done
        i=0
        N=${#v_input_sources_real[@]}
        if [[ $N -eq 0 ]]; then _dbg "No input sources."; fi
        while [[ $i -lt $N ]]; do
            _dbg "Input source #$i: user file = '${v_input_sources_user[$i]}'"
            _dbg "Input source #$i: base path = '${v_input_sources_base[$i]}'"
            _dbg "Input source #$i: real file = '${v_input_sources_real[$i]}'"
            let i=i+1
        done
        i=0
        N=${#v_target_names[@]}
        if [[ $N -eq 0 ]]; then _dbg "No target names."; fi
        while [[ $i -lt $N ]]; do
            _dbg "Target #$i: user file = '${v_target_names[$i]}'"
            let i=i+1
        done
        _dbg "Output file name = '$v_output_name'"
    fi
}

# $1=message
add_error() {
    errors[${#errors[@]}]="$1"
}

# $1=argument name, $2=argument index
add_missing_arg_error() {
    add_error "Missing parameter for $1 (argument #$2)."
}

show_errors() {
    local -i i
    local -i N
    i=0
    N=${#errors[@]}
    while [[ $i -lt $N ]]; do
        echo "${errors[$i]}" >&2;
        let i=i+1
    done
}

exit_on_errors() {
    local -i N
    N=${#errors[@]}
    if [[ $N -ne 0 ]]; then
        echo "Errors: $N. Stopped." >&2
        exit 1
    fi
}

# $1=message
error_exit() {
    if [[ -n "$1" ]]; then
        echo "$1" >&2
    fi
    exit 1
}

# $1=real path, $2=base path, $3=user path
# $4=(refname) index
add_include_path() {
    local -n index=$4
    local -i i
    i=${#v_include_paths_real[@]}
    v_include_paths_real[$i]="$1"
    v_include_paths_base[$i]="$2"
    v_include_paths_user[$i]="$3"
    index=$i
    _dbg "Added new include path #$i:"
    _dbg "Include path #$i: user path = '$3'"
    _dbg "Include path #$i: base path = '$2'"
    _dbg "Include path #$i: real path = '$1'"
    return 0
}

# $1=target name
add_target_name() {
    local -i i
    i=${#v_target_names[@]}
    v_target_names[$i]="$1"
    _dbg "Added new target name #$i: '$1'"
    return 0
}

# $1=real path
# $2=(refname) index or -1
# returns: 0=success or 1=fail
find_include_path_real() {
    local -n index=$2
    local -i i
    local -i N
    local f
    local r
    i=0
    N=${#v_include_paths_real[@]}
    while [[ $i -lt $N ]]; do
        r="${v_include_paths_real[$i]}"
        if [[ "$r" == "$1" ]]; then
            index=$i
            f="${v_include_paths_user[$i]}"
            _dbg "found '$f' as '$r' at #$i."
            return 0
        fi
        let i=i+1
    done
    index=-1
    _dbg "Failed to find real path '$1'."
    return 1
}

# $1=user path
# $2=(refname) index or -1
# returns: 0=success or 1=fail
find_include_path_user() {
    local -n index=$2
    local -i i
    local -i N
    local f
    local r
    i=0
    N=${#v_include_paths_real[@]}
    while [[ $i -lt $N ]]; do
        f="${v_include_paths_user[$i]}"
        if [[ "$f" == "$1" ]]; then
            index=$i
            r="${v_include_paths_real[$i]}"
            _dbg "Found user path '$f' (real path '$r') at #$i."
            return 0
        fi
        let i=i+1
    done
    index=-1
    _dbg "Failed to find user path '$1'."
    return 1
}

# $1=user path
# $2=(refname) index or -1
# returns: 0=success or 1=fail
add_include_dir_with_check() {
    local f="$1"
    local -n index=$2
    local -i i
    local r
    if [[ "${f:0:1}" == '/' ]]; then
        # absolute path
        r="$f"
        if ! find_include_path_real "$r" i; then
            add_include_path "$r" '' "$f" i
        fi
        index=$i
    else
        # relative path
        if find_include_path_user "$f" i; then
            index=$i
        else
            r=`realpath -s "$v_base_path_real/$f"`
            if [[ -d "$r" ]]; then
                add_include_path "$r" "$v_base_path_real" "$f" i
                index=$i
            else
                index=-1
                _dbg "Failed to find user path '$f'."
                return 1
            fi
        fi
    fi
    return 0
}

# $1=real file, $2=base path, $3=user file
# $4=(refname) index
add_input_source() {
    local -n index=$4
    local -i i
    i=${#v_input_sources_real[@]}
    v_input_sources_real[$i]="$1"
    v_input_sources_base[$i]="$2"
    v_input_sources_user[$i]="$3"
    index=$i
    _dbg "Added new input source #$i:"
    _dbg "Input source #$i: user file = '$3'"
    _dbg "Input source #$i: base path = '$2'"
    _dbg "Input source #$i: real file = '$1'"
    return 0
}

# $1=real file
# $2=(refname) index or -1
# returns: 0=success or 1=fail
find_input_source_real() {
    local -n index=$2
    local -i i
    local -i N
    local f
    local r
    i=0
    N=${#v_input_sources_user[@]}
    while [[ $i -lt $N ]]; do
        r="${v_input_sources_real[$i]}"
        if [[ "$r" == "$1" ]]; then
            index=$i
            f="${v_input_sources_user[$i]}"
            _dbg "Found user file '$f' (real file '$r') at #$i."
            return 0
        fi
        let i=i+1
    done
    index=-1
    _dbg "Failed to find real file '$1'."
    return 1
}

# $1=user file
# $2=(refname) index or -1
# returns: 0=success or 1=fail
find_input_source_user() {
    local -n index=$2
    local -i i
    local -i N
    local f
    local r
    i=0
    N=${#v_input_sources_real[@]}
    while [[ $i -lt $N ]]; do
        f="${v_input_sources_user[$i]}"
        if [[ "$f" == "$1" ]]; then
            r="${v_input_sources_real[$i]}"
            index=$i
            _dbg "Found user file '$f' (real file '$r') at #$i."
            return 0
        fi
        let i=i+1
    done
    index=-1
    _dbg "Failed to find user file '$1'."
    return 1
}

# $1=user file
# $2=(refname) file index or -1
# returns: 0=success or 1=fail
add_input_source_with_check() {
    local f="$1"
    local -n index=$2
    local -i i
    local f
    local r
    if [[ "${f:0:1}" == '/' ]]; then
        # absolute path
        #r=`realpath "$f"`
        r="$f"
        # first - check if it is already added
        if ! find_input_source_real "$r" i; then
            add_input_source "$r" '' "$f" i
        fi
        index=$i
    else
        # relative path
        # first - check if it is already added
        if find_input_source_user "$f" i; then
            index=$i
        else
            # trying real path - we are lucky
            #r=`realpath "$f"`
            r="$v_base_path_real/$f"
            if [[ -f "$r" ]]; then
                add_input_source "$r" "$v_base_path_real" "$f" i
                index=$i
            else
                index=-1
                _dbg "Failed to find user file '$f'."
                return 1
            fi
        fi
    fi
    return 0
}

# $1=real file, $2=base path, $3=user file, $4=parse flag
add_source() {
    local -i i
    i=${#v_sources_real[@]}
    v_sources_real[$i]="$1"
    v_sources_base[$i]="$2"
    v_sources_user[$i]="$3"
    v_sources_parse[$i]="$4"
    _dbg "Added new source #$i:"
    _dbg "Source #$i: parse     = $4"
    _dbg "Source #$i: user name = '$3'"
    _dbg "Source #$i: base path = '$2'"
    _dbg "Source #$i: real name = '$1'"
    return 0
}

# $1=real file
# $2=(refname) index or -1
# returns: 0=success or 1=fail
find_source_real() {
    local -n index=$2
    local -i i
    local -i N
    local f
    local r
    i=0
    N=${#v_sources_real[@]}
    while [[ $i -lt $N ]]; do
        r="${v_sources_real[$i]}"
        if [[ "$r" == "$1" ]]; then
            index=$i
            f="${v_sources_user[$i]}"
            _dbg "Found user file '$f' (real file '$r') at #$i."
            return 0
        fi
        let i=i+1
    done
    index=-1
    _dbg "Failed to find real file '$1'."
    return 1
}

# $1=user file
# $2=(refname) index or -1
# returns: 0=success or 1=fail
find_source_user() {
    local -n index=$2
    local -i i
    local -i N
    local f
    local r
    i=0
    N=${#v_sources_real[@]}
    while [[ $i -lt $N ]]; do
        f="${v_sources_user[$i]}"
        if [[ "$f" == "$1" ]]; then
            index=$i
            r="${v_sources_real[$i]}"
            _dbg "Found user file '$f' (real file '$r') at #$i."
            return 0
        fi
        let i=i+1
    done
    index=-1
    _dbg "Failed to find user file '$1'."
    return 1
}

# $1=prerequisite
add_prerequisite() {
    local -i i
    i=${#v_prerequisites[@]}
    v_prerequisites[$i]="$1"
    _dbg "Added new prerequisite #$i: '$1'"
    return 0
}

print_prerequisites() {
    local -i i
    local -i N
    i=0
    N=${#v_prerequisites[@]}
    while [[ $i -lt $N ]]; do
        echo -n " ${v_prerequisites[$i]}"
        let i=i+1
    done
    return 0
}

show_title() {
    cat <<EOT
$PROGRAM_NAME (version $PROGRAM_VERSION) - $PROGRAM_DESCRIPTION
$PROGRAM_LICENSE
$PROGRAM_AUTHORS
$PROGRAM_CONTACTS
EOT
}

show_help() {
    cat <<EOT

Usage:
    `basename $0` [options] [filename ...] [options]

Options (GCC-compatible):
-h, --help      show this help and exit
-E              preprocess
-I <path>       include directory
-M[M]           output autodepend make rule
-MF <file>      autodepend output name
-MT <target>    autodepend target name (can be specified multiple times)
EOT
}

# $1=relative file
# $2=(refname) relative file
# $3=(refname) base directory
# $4=(refname) real file
resolve_file() {
    local f_loc="$1"
    local -n _user=$2
    local -n _base=$3
    local -n _real=$4
    local -i i=0
    local -i N=${#v_include_paths_real[@]}
    local f_user
    local f_base
    local f_real
    while [[ $i -lt $N ]]; do
        f_user="$f_loc"
        f_base="${v_include_paths_real[$i]}"
        f_real="$f_base/$f_user"
        _dbg "checking '$f_user' at '$f_base'..."
        if [[ -f "$f_real" ]]; then
            _user="$f_user"
            _base="$f_base"
            _real="$f_real"
            _dbg "found '$f_user' at '$f_base'."
            return 0
        else
            _dbg "not found, skipped."
        fi
        let i=i+1
    done
    _dbg "'$f_loc' is not resolved."
    return 1
}

# $1=index
parse_source() {
    local tmp
    local -i N
    local src_user=${v_sources_user[$1]}
    local src_base
    local f_loc
    local -i inc_parse
    _dbg "parsing source #$1 ('$src_user')..."
    if [[ -f "${v_sources_user[$1]}" ]]; then
        tmp=`mktemp`
        grep -E -e '^[[:space:]]*(include|incbin)[[:space:]]+"[^"]+"' "${v_sources_user[$1]}" | \
        sed -r 's,^[[:space:]]*(include|incbin)[[:space:]]+"[[:space:]]*([^"]+)[[:space:]]*".*,\1 \2,' > $tmp
        N=`wc -l < $tmp | cut -d : -f 1`
        _dbg "parsed '$src_user' with $N entries."
        while read f_loc; do
            case "${f_loc%% *}" in
            include)
                inc_parse=1
                ;;
            incbin)
                inc_parse=0
                ;;
            esac
            f_loc="${f_loc#* }"
            inc_user="$f_loc"
            if [[ "${f_loc:0:1}" == '/' ]]; then
                # absolute source's path - use it as is
                _dbg "checking '$f_loc'..."
                if [[ -f "$f_loc" ]]; then
                    inc_real="$f_loc"
                    inc_base=`dirname "$f_loc"`
                    add_source "$inc_real" "$inc_base" "$inc_user" $inc_parse
                else
#                   rm -f $tmp
#                   add_error "Specified '$f_loc' not found."
#                   _dbg "'$f_loc' not found."
#                   return 1
                    inc_real="$f_loc"
                    inc_base=`dirname "$f_loc"`
                    add_source "$inc_real" "$inc_base" "$inc_user" 0
                fi
            else
                # realtive source's path - try to resolve real name
                src_base=`dirname "$src_user"`
                if [[ "${src_user:0:1}" == '/' ]]; then
                    inc_real="$src_base/$src_user"
                    inc_base="$src_base"
                else
                    inc_real=`dirname "${v_sources_real[$1]}"`
                    inc_real="$inc_real/$f_loc"
                    inc_base="${v_sources_base[$1]}"
                    if [[ "$src_base" != '.' ]]; then
                        inc_user="$src_base/$inc_user"
                    fi
                fi
                if [[ -f "$inc_real" ]]; then
                    add_source "$inc_real" "$inc_base" "$inc_user" $inc_parse
                else
                    _dbg "'$f_loc' not found, resolving..."
                    if resolve_file "$f_loc" inc_user inc_base inc_real; then
                        add_source "$inc_real" "$inc_base" "$inc_user" $inc_parse
                    else
#                       rm -f $tmp
#                       add_error "Specified '$f_loc' not found."
#                       _dbg "'$f_loc' not found."
#                       return 1
                        add_source "$inc_real" "$inc_base" "$inc_user" 0
                    fi
                fi
            fi
        done < $tmp
        rm -f $tmp
    else
        add_error "'$src_user' not found."
        _dbg "'$src_user' not found."
        return 1
    fi
    return 0
}

make_rule() {
    local -i i
    local -i N
    local _real
    local _base
    local _rel
    local _user

    i=0
    N=${#v_input_sources_real[@]}
    while [[ $i -lt $N ]]; do
        _real="${v_input_sources_real[$i]}"
        _base="${v_input_sources_base[$i]}"
        _user="${v_input_sources_user[$i]}"
        add_source "$_real" "$_base" "$_user" 1
        let i=i+1
    done

    i=0
    N=${#v_sources_real[@]}
    while [[ $i -lt $N ]]; do
        while [[ $i -lt $N ]]; do
            case "x${v_sources_parse[$i]}" in
            x1)
                if parse_source $i; then
                    add_prerequisite "${v_sources_user[$i]}"
                else
                    show_errors
                    exit_on_errors
                fi
                ;;
            *)
                add_prerequisite "${v_sources_user[$i]}"
                ;;
            esac
            let i=i+1
        done
        i=N
        N=${#v_sources_real[@]}
    done
    return 0
}

# $1=output file
write_rule() {
    echo -n "${v_target_names[*]}:" > "$1"
    print_prerequisites >> "$1"
    echo '' >> "$1"
}

if [[ $# -eq 0 ]]; then
    error_exit "No parameters. $HELP_HINT"
fi

v_base_path_real="$PWD"
_dbg "base path = '$v_base_path_real'"

i=1
while [[ $# -gt 0 ]]; do
    case "$1" in
    -h|--help)
        v_act_show_help=1
        shift 1
        ;;
    -E)
        v_act_preprocess=1
        shift 1
        ;;
    -I)
        shift 1
        if [[ $# -eq 0 ]]; then
            add_missing_arg_error -I $i
            break
        fi
        add_include_dir_with_check "$1" i
        shift 1
        ;;
    -M|-MM)
        v_act_make_rule=1
        shift 1
        ;;
    -MF)
        shift 1
        if [[ $# -eq 0 ]]; then
            add_missing_arg_error -MF $i
            break
        fi
        v_output_name=$1
        shift 1
        ;;
    -MT)
        shift 1
        if [[ $# -eq 0 ]]; then
            add_missing_arg_error -MT $i
            break
        fi
        add_target_name "$1"
        shift 1
        ;;
    -*)
        add_error "Unknown option '$1' (#$i)."
        shift 1
        ;;
    *)
        if [[ ${#v_input_sources_user[@]} -gt 1 ]]; then
            add_error "Don't know what to do with input file '$1' (#$i)."
        else
            if ! add_input_source_with_check $1; then
                error_exit "$1 is not found."
            fi
        fi
        shift 1
        ;;
    esac
    let i=i+1
done

if [[ $v_act_show_help -ne 0 ]]; then
    if [[ $(( v_act_preprocess + v_act_make_rule + ${#v_include_paths_user[@]} + ${#v_sources_user[@]} )) -ne 0 ]]; then
        add_error "Other arguments were ignored."
    fi
    v_act=show_help
else
    if [[ $(( v_act_preprocess + v_act_make_rule )) -ne 2 ]]; then
        add_error "The only supported mode is when both options -E and -M are specified."
    fi
    v_act=make_rule
fi

if [[ ${#errors[@]} -ne 0 ]]; then
    if [[ $v_act_show_help -ne 0 ]]; then
        show_title
    fi
    show_errors
    if [[ $v_act_show_help -ne 0 ]]; then
        show_help
        exit 0
    else
        exit_on_errors
    fi
fi

case $v_act in
show_help)
    show_title
    show_help
    exit 0
    ;;
make_rule)
    if [[ ${#v_target_names[@]} -eq 0 ]]; then
        add_error "No target name was specified."
    fi
    if [[ -z "${v_output_name}" ]]; then
        add_error "No output name was specified."
    fi
    if [[ ${#v_input_sources_real[@]} -eq 0 ]]; then
        add_error "No source files were specified."
    fi
    if [[ ${#errors[@]} -ne 0 ]]; then
        show_errors
        exit_on_errors
    fi
    if [[ ${#v_include_paths_user[@]} -eq 0 ]]; then
        add_include_dir_with_check '.' i
    fi
    _dbg_dump_vars
    make_rule
    write_rule "$v_output_name"
    ;;
*)
    error_exit "Action $v_act is not implemented yet."
    ;;
esac
