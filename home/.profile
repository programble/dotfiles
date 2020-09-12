_PATH=$PATH PATH=
path() { [ -d "$1" ] && PATH="${PATH}${PATH:+:}${1}"; }
for prefix in '' /usr/local /usr /opt/local ~/.local; do
	path "${prefix}/sbin"
	path "${prefix}/bin"
done
path /usr/games

export PAGER=less
export LESS=FRX
export EDITOR=nvim
export MANPAGER='nvim +Man!'
export MANSECT=2:3:1:8:6:5:7:4:9
export CLICOLOR=1
export GPG_TTY=$(tty)
export NETHACKOPTIONS='pickup_types:$!?+/=, color, DECgraphics'

[ -e /usr/share/mk/sys.mk ] || export CFLAGS=-O
[ -d /usr/home ] && cd

export ENV=~/.shrc
