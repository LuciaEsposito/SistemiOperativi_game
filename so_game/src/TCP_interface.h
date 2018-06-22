#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "so_game_protocol.h"
#include "world.h"

void sendTCP(int socket_desc, PacketHeader* packet);

int receiveTCP(int socket_desc , char* msg);