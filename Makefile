all: user

user: User.c
	gcc User.c -o User
