socketFile = server.c
socketExecutable = server.o
firstPort = 20000
secondPort = 10000
ip = 127.0.0.1
socketFolder = ./Server/
clientFolder = ./Client/
chatClientName = ChatClient
user1 = User1
user2 = User2

buildServers:
	gcc ${socketFolder}${socketFile} -o ${socketFolder}${socketExecutable}
	cd ${socketFolder}; \
	./${socketExecutable} ${ip} ${firstPort} ${secondPort} 0 & \
	./${socketExecutable} ${ip} ${secondPort} ${firstPort} 1 &

buildServer:
	gcc ${socketFolder}${socketFile} -o ${socketFolder}${socketExecutable}
	cd ${socketFolder}; \
	./${socketExecutable} ${ip} ${firstPort} ${secondPort} 0 & \
	./${socketExecutable} ${ip} ${secondPort} ${firstPort} 1 &

buildClient:
	cd ${clientFolder}; \
	javac ${chatClientName}.java; \
	cp ${chatClientName}.class ${user1}/${chatClientName}.class; \
	cd ${user1}; \
	java ${chatClientName} ${ip} ${firstPort}& \

buildClients:
	cd ${clientFolder}; \
	javac ${chatClientName}.java; \
	cp ${chatClientName}.class ${user1}/${chatClientName}.class; \
	cp ${chatClientName}.class ${user2}/${chatClientName}.class; \
	cd ${user1}; \
	java ${chatClientName} ${ip} ${firstPort}& \
	cd ..; \
	cd ${user2}; \
	java ${chatClientName} ${ip} ${secondPort}&

terminate:
	killall ${socketExecutable}


debug:
	cd ${socketFolder}; \
	gcc ${socketFile} -Wall -ggdb3 -g -o ${socketExecutable};\
	./${socketExecutable} ${ip} ${secondPort} ${firstPort} 1 & \
	valgrind --track-origins=yes --leak-check=full  ./${socketExecutable} ${ip} ${firstPort} ${secondPort} 0 &
