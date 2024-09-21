serverFile = server.c
serverExecutable = server.o
firstPort = 20000
secondPort = 10000

build:
	gcc ${serverFile} -o ${serverExecutable}
	./${serverExecutable} ${firstPort} ${secondPort} 1
buildClient:
	javac ChatClient.java
	java ChatClient

launchClient:
	java ChatClient