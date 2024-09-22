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
	cd ${socketFolder}; \
	./${socketExecutable} ${firstPort} ${secondPort} 0 & \
	./${socketExecutable} ${secondPort} ${firstPort} 1 &


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


terminate: 
	killall ${socketExecutable}


debug:
	cd ${socketFolder}; \
	gcc ${socketFile} -Wall -ggdb3 -g -o ${socketExecutable};\
	valgrind --track-origins=yes ./${socketExecutable} ${firstPort} ${secondPort} 0 &
	./${socketExecutable} ${secondPort} ${firstPort} 1 &