#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "messages.h"

// populates image packet
ImagePacket* set_image_packet(PacketHeader header, int id, Image* image) {
    ImagePacket* new_packet = (ImagePacket*) malloc(sizeof(ImagePacket));
    new_packet->header = header;
    new_packet->id = id;
    new_packet->image = image;
    return new_packet;
}

// inizializza id packet
IdPacket* id_packet_init(Type header_type, int id){
    PacketHeader id_header;
    id_header.type = header_type;

    IdPacket* id_packet = (IdPacket*)malloc(sizeof(IdPacket));
    id_packet->header = id_header;
    id_packet->id = id;
    return id_packet;
}

/*-------------------------- funzioni create per inviare pacchetti da SERVER a CLIENT --------------------------------------------*/

// pacchetto usato per assegnare surface texture
ImagePacket* CreateSurfaceTexturePacket(Image* selected_image) {
    PacketHeader packet_header;
    packet_header.type = PostTexture;
    ImagePacket* new_packet = set_image_packet(packet_header, 0, selected_image);
    return new_packet;
}

// pacchetto usato per assegnare surface elevation
ImagePacket* CreateElevationMapPacket(Image* selected_image) {
    PacketHeader packet_header;
    packet_header.type = PostElevation;
    ImagePacket* new_packet = set_image_packet(packet_header, 0, selected_image);
    return new_packet;
}

// pacchetto usato per assegnare vehicle texture
ImagePacket* CreateVehicleTexturePacket(Image* selected_image, int client_id) {
    PacketHeader packet_header;
    packet_header.type = PostTexture;
    ImagePacket* new_packet = set_image_packet(packet_header, client_id, selected_image);
    return new_packet;
}

// pacchetto usato per assegnare vehicle texture nel caso in cui il client non scelga la texture e quindi venga assegnata dal server (default)
ImagePacket* CreateDefaultTexturePacket(int client_id) {
    PacketHeader packet_header;
    packet_header.type = PostTexture;
    ImagePacket* new_packet = set_image_packet(packet_header, client_id, NULL);
    return new_packet;
}


/*-------------------------- funzioni create per inviare pacchetti da CLIENT a SERVER --------------------------------------------*/

// pacchetto usato per richiedere surface texture
ImagePacket* CreateSurfaceTextureRequestPacket(void) {
    PacketHeader packet_header;
    packet_header.type = GetTexture;
    ImagePacket* new_packet = set_image_packet(packet_header, 0, NULL);
    return new_packet;
}

// pacchetto usato per richiedere surface elevation
ImagePacket* CreateElevationMapRequestPacket(void) {
    PacketHeader packet_header;
    packet_header.type = GetElevation;
    ImagePacket* new_packet = set_image_packet(packet_header, 0, NULL);
    return new_packet;
}

// pacchetto usato per richiedere vehicle texture
ImagePacket* CreateVehicleTextureRequestPacket(int client_id) {
    PacketHeader packet_header;
    packet_header.type = GetTexture;
    ImagePacket* new_packet = set_image_packet(packet_header, client_id, NULL);
    return new_packet;
}
