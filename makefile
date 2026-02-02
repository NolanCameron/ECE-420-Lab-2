CC = gcc
CFLAGS = -Wall -Werror -Wvla -lpthread -ggdb3 -lm
DEPS = timer.h common.h
OBJ = main.o client.o attacker.o

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
.PHONY: client
matrixgen: client.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: attacker
matrixgen: attacker.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: memtest
memtest: main client attacker
	valgrind --tool=memcheck --leak-check=yes ./main 1000 127.0.0.1 3000 &
	./client 1000 127.0.0.1 3000
	#./attacker 1000 127.0.0.1 3000

.PHONY: threadtest
threadtest: main client attacker
	valgrind --tool=helgrind ./main 1000 127.0.0.1 3000 &
	./client 1000 127.0.0.1 3000
	#./attacker 1000 127.0.0.1 3000

.PHONY: clean
clean:
	rm -f *.o main client attacker