#include <GL/glut.h> // not needed here
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

// for the udp_socket
#include <arpa/inet.h>
#include <sys/socket.h>

#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "so_game_protocol.h"
#include "TCP_interface.h"
#include "linked_list.h"
#include "messages.h"

// parameters used to set up server threads
typedef struct params {
    World* environment;     // the engine environment
    int player_ID;          // player identifier
    int player_socket;      // socket used to communicate with the player
    Image* map_elevation;   // image used to produce the surface mesh
    Image* terrain_img;     // image used as surface texture
} params;

void *connection_thread(void* params_ptr);      // thread used for connection via TCP

