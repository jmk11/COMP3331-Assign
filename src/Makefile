CC	= gcc
CFLAGS	= -Wall -Wextra -pedantic -g
#-pthread
# should everything be compiled with the --pthread? prob no !!

.PHONY:	all
all:	server client

server:				server.o user.o helpers.o protocol.o message.o commands.o blockedusers.o serverClients.o clients.o
					$(CC) $(CFLAGS) -pthread -o server server.o user.o helpers.o protocol.o message.o commands.o blockedusers.o serverClients.o clients.o
server.o:			server.c
user.o:				user.c
message.o:			message.c
commands.o: 		commands.c
blockedusers.o: 	blockedusers.c
#p2prequests.o:		p2prequests.c
serverClients.o:	serverClients.c
clients.o:			clients.c

client:				client.o helpers.o protocol.o p2pConnections.o
					$(CC) $(CFLAGS) -pthread -o client client.o helpers.o protocol.o p2pConnections.o
client.o:			client.c

helpers.o:			helpers.c
protocol.o:			protocol.c
p2pConnections.o:	p2pConnections.c


.PHONY: clean
clean:
	-rm *.o server client
	
# it seems files can't have capital names in Makefile ... for unknown reasons
