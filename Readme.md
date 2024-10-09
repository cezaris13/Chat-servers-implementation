# Chat server implementation using sockets for Computer networks course in Vilnius university (2022, spring semester)

## Launching the program
```bash
make buildServers # launches 2 server instances with ports 20000 and 10000
make buildClients # launches 2 client instances where User1 connects to Socket1 and User2 connects to Socket2)
```

## Terminate program
```bash
make terminate
```

## Features
- Sending messages
- Reading file content which is stored in the server (ex. #get `filename` ...) will read the file of the socket to which the user is connected.
- File sending to specified socket(ex. @Socket1 `filename`), after sending the file, the destination socket sends the message to the users of that socket that the file has been received.

