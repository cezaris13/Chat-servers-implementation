serverFile = server.c
serverExecutable = server.o

build:
	gcc ${serverFile} -o ${serverExecutable}
	./${serverExecutable}
buildClient:
	javac ChatClient.java
	java ChatClient

launchClient:
	java ChatClient