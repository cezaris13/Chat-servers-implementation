##
# Project Title
#
# @file
# @version 0.1

build:
#	gcc -g -rdynamic ./server.c -o server.o
	gcc server.c -o server.o
	./server.o
buildClient:
	javac PokalbiuKlientas.java
	java PokalbiuKlientas

launchClient:
	java PokalbiuKlientas

# end
