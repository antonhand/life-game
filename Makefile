CC = gcc
CFLAGS = -m32 -O2 -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self -Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith -Wtype-limits -Wempty-body -Wlogical-op -Wstrict-prototypes -Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type -Wmissing-field-initializers -Wnested-externs -Wno-pointer-sign -std=gnu11 -lm
BIN = ./bin
OBJ = ./obj

all: $(BIN)/life-server $(BIN)/life-client

$(BIN)/life-server: $(OBJ)/life-server.o $(OBJ)/lifefld.o $(OBJ)/lifeop.o | $(BIN)
		$(CC) -lm -o $(BIN)/life-server $(OBJ)/life-server.o $(OBJ)/lifefld.o $(OBJ)/lifeop.o 

$(OBJ)/life-server.o: life-server.c lifefld.h lifeop.h | $(OBJ)
		$(CC) $(CFLAGS) -c -o $(OBJ)/life-server.o life-server.c

$(OBJ)/lifefld.o: lifefld.c lifefld.h lifeop.h | $(OBJ)
		$(CC) $(CFLAGS) -c -o $(OBJ)/lifefld.o lifefld.c

$(OBJ)/lifeop.o: lifeop.c lifefld.h lifeop.h | $(OBJ)
		$(CC) $(CFLAGS) -c -o $(OBJ)/lifeop.o lifeop.c

$(BIN)/life-client: $(OBJ)/life-client.o | $(BIN)
		$(CC) -o $(BIN)/life-client $(OBJ)/life-client.o
		
$(OBJ)/life-client.o: life-client.c | $(OBJ)
		$(CC) $(CFLAGS) -c -o $(OBJ)/life-client.o life-client.c

$(BIN):
	mkdir -p $(BIN)

$(OBJ):
	mkdir -p $(OBJ)
	
clean:
		-rm $(OBJ)/*
		-rm $(BIN)/*