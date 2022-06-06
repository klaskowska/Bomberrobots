CC = g++
CFLAGS = -std=gnu++20 -Wall -Wextra -Wconversion -Werror -O2 -pthread
 

 
robots-server: robots-server.o server-parser.o error-handler.o
	$(CC) $(CFLAGS) -o robots-server robots-server.o error-handler.o server-parser.o -lboost_program_options

 
robots-server.o: robots-server.cpp server-parser.h error-handler.h
	$(CC) $(CFLAGS) -c robots-server.cpp -lboost_program_options

server-parser.o: server-parser.cpp server-parser.h error-handler.h
	$(CC) $(CFLAGS) -c server-parser.cpp -lboost_program_options

error_handler.o: error_handler.h error_handler.cpp

clean: 
	rm *.o robots-server