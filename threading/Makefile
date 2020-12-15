server = serv.out
server_src = $(wildcard server.c my_queue.c)

server_objects = $(server_src:.c=.o)
flags = -g -Wall -Werror -Wextra -Wpedantic -std=c11 -pthread

$(server): $(server_objects)
	gcc $(server_objects) $(flags) -o $(server)

%.o: %.c %.h
	gcc -c $(flags) $< -o $@

clean:
	-rm *.out
	-rm *.o

lint:
	clang-tidy *.c *.h