lib : 
	cc -Wall -c msocket.c
	ar rcs libmtp.a msocket.o
	gcc -Wall -L. user1.c -lmtp
	gcc -Wall -L. user2.c -lmtp