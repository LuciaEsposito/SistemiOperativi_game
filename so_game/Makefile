CFLAGS= -Wall -g -std=gnu99 -Wstrict-prototypes
LIBS= -lglut -lGLU -lGL -lm -lpthread
CC=gcc
AR=ar


BINS=libso_game.a\
     server\
     client

OBJS = src/vec3.o\
       src/linked_list.o\
       src/surface.o\
       src/image.o\
       src/vehicle.o\
       src/world.o\
       src/world_viewer.o\
       src/so_game_protocol.o\
       src/TCP_interface.o\
       src/messages.o\
       src/server_threads.o\
       src/client_update_functions.o\
       src/address_list.o\

HEADERS=helpers.h\
	src/image.h\
	src/linked_list.h\
	src/so_game_protocol.h\
	src/surface.h\
	src/vec3.h\
	src/vehicle.h\
	src/world.h\
	src/world_viewer.h\
	src/TCP_interface.h\
	src/messages.h\
	src/server_threads.h\
	src/client_update_functions.h\
	src/address_list.h\


%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 


libso_game.a: $(OBJS) 
	$(AR) -rcs ./bin/$@ $^
	$(RM) $(OBJS)
	
server: ./src/so_game_server.c ./bin/libso_game.a
	$(CC) $(CCOPTS) -Ofast -o ./bin/$@ $^ $(LIBS)

client: ./src/so_game_client.c ./bin/libso_game.a
	$(CC) $(CCOPTS) -Ofast -o ./bin/$@ $^ $(LIBS) 

default_server:
	./bin/server ./images/arrow-right.ppm ./images/test.pgm ./images/test.ppm

local_client:
	./bin/client 127.0.0.1

delete:
	rm -rf *.o *~  ./bin/* 
