all: user server

user: User.c
	gcc User.c -o User

server: DS.c
	gcc DS.c -o DS
