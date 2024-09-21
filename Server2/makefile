build:
	gcc -g -rdynamic ./server.c ../Lib/Functions.c -o server.o
	./server.o
buildClient:
	javac ChatClient.java
	java ChatClient

launchClient:
	java ChatClient