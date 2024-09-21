firstServerPort = 20000
secondServerPort = 10000

build:
	gcc -g -rdynamic ./server.c -o server.o
	./server.o
buildClient:
	javac ChatClient.java
	java ChatClient

launchClient:
	java ChatClient