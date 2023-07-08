all: server client

server: server.cpp common.h
	g++ server.cpp -lpthread -o server

client: client.cpp common.h
	g++ client.cpp -lpthread -o client

.PHONY: clean
clean:
	rm server client