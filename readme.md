Instant messaging client and server. Multithreaded CLI applications developed for COMP3331 Computer Networks & Applications UNSW.

#### Build and run
```bash
CC=gcc meson --prefix $PWD build
cd build
ninja install
cd ../bin
cp ../src/server/credentials.txt .
cp ../src/server/welcome.txt .
```

```bash
./server <server_port> <block_duration> <timeout>
```

```bash
./client <server_ip> <server_port>
```

#### Client commands
```
message <user> <msg>:      Message specific user
broadcast <msg>:           Message all online users
whoelse:                   List all other online users
whoelsesince <seconds>:    List all other users active in the last n seconds
block <user>:              Block a certain user
unblock <user>:            Unblock a previously blocked user
startprivate <user>        Start P2P connection with specific user
private <user> <msg>       Send message over P2P connection to specific user
stopprivate <user>         End P2P connection with specific user
logout:                    Logout
```
