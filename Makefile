build: client/client.cpp client/client.h server/server.cpp server/server.cpp include/include.h
	@g++ client/client.cpp client/client.h -o client.out -pthread -I include 
	@g++ server/server.cpp server/server.h -o server.out -pthread -I include

clean:
	-rm client.out
	-rm server.out