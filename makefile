socketFile = server.c
socketExecutable = server.o
firstPort = 20000
secondPort = 10000
socketFolder = ./Server/
clientFolder = ./Client/
chatClientName = ChatClient
user1 = User1
user2 = User2

build:
	gcc ${socketFolder}${socketFile} -o ${socketFolder}${socketExecutable}
	${socketFolder}./${socketExecutable} ${firstPort} ${secondPort} 1 &
	${socketFolder}./${socketExecutable} ${secondPort} ${firstPort} 0 &

client:
	cd ${clientFolder}; \
	javac ${chatClientName}.java; \
	cp ${chatClientName}.class ${user1}/${chatClientName}.class; \
	cp ${chatClientName}.class ${user2}/${chatClientName}.class; \
	cd ${user1}; \
	java ${chatClientName}& \
	cd ..; \
	cd ${user2}; \
	java ${chatClientName}&
