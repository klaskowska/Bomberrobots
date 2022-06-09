CC = g++
CFLAGS = -std=gnu++20 -Wall -Wextra -Wconversion -Werror -O2 -pthread
 

 
robots-server: robots-server.o game-engine.o byte-parser.o server-parser.o server-tcp-handler.o game-messages.o error-handler.o
	$(CC) $(CFLAGS) -o robots-server robots-server.o error-handler.o byte-parser.o game-messages.o server-parser.o server-tcp-handler.o game-engine.o -lboost_program_options

 
robots-server.o: robots-server.cpp game-engine.h server-parser.h server-tcp-handler.h game-messages.h error-handler.h
	$(CC) $(CFLAGS) -c robots-server.cpp -lboost_program_options

game-engine.o: game-engine.cpp game-engine.h server-parser.h byte-parser.h server-tcp-handler.h game-messages.h error-handler.h
	$(CC) $(CFLAGS) -c game-engine.cpp -lboost_program_options

server-parser.o: server-parser.cpp server-parser.h error-handler.h
	$(CC) $(CFLAGS) -c server-parser.cpp -lboost_program_options

server-tcp-handler.o: server-tcp-handler.cpp server-tcp-handler.h game-messages.h error-handler.h
	$(CC) $(CFLAGS) -c server-tcp-handler.cpp

byte-parser.o: byte-parser.cpp byte-parser.h game-messages.h
	$(CC) $(CFLAGS) -c byte-parser.cpp

game-messages.o: game-messages.h game-messages.cpp
	$(CC) $(CFLAGS) -c game-messages.cpp

error-handler.o: error-handler.h error-handler.cpp
	$(CC) $(CFLAGS) -c error-handler.cpp

clean: 
	rm *.o robots-server