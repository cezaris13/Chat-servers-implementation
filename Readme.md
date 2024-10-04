# Chat server implementation using sockets for Computer networks course in Vilnius university (2022, spring semester)

## Launching the program
```bash
make server # launches 2 server instances with ports 20000 and 10000
make client # launches 2 client instances
```

## Terminate program
```bash
make terminate
```

## Features
- Sending messages
- Reading file content which is stored in the server (ex. #get `filename` ...) will read the file of the socket to which the user is connected.
- File sending to specified socket(ex. @Socket1 `filename`)

