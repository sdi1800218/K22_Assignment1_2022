# Compile options. Το -I<dir> χρειάζεται για να βρει ο gcc τα αρχεία .h
CFLAGS = -Wall -Werror -g -c
COMPILER = gcc
LDFLAGS  = -lpthread

# Αρχεία .o, εκτελέσιμο πρόγραμμα και παράμετροι
OBJS = userland.o child_worker.o
EXEC = lenux_userland

child_worker.o: child_worker.c common.h
		$(COMPILER) $(CFLAGS) child_worker.c common.h

userland.o: userland.c common.h
		$(COMPILER) $(CFLAGS) userland.c common.h

lenux_userland: $(OBJS)
		$(COMPILER) $(OBJS) -o $(EXEC) $(LDFLAGS)

clean:
		rm -f $(OBJS) $(EXEC) *.gch

run: $(EXEC)
		./$(EXEC) input.txt 5 10