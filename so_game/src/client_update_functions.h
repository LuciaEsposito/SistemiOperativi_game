#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>

#include "image.h"
#include "world.h"
#include "vehicle.h"
#include "so_game_protocol.h"
#include "world_viewer.h"
#include "TCP_interface.h"
#include "messages.h"
#include "surface.h"

// struttura del thread usato dal client
typedef struct {
    int ID_v;				// ID del veicolo
    int socket_TCP;
    int socket_UDP;
    struct sockaddr_in* address_UDP;
    Vehicle* vehicle;
    World* world;
    Image* image;
} client_thread;

// funzioni svolte dai thread
void* updateReceiver(void* args);
void* updateSender(void* args);
