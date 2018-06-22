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
#include "./TCP_interface.h"


void sendTCP(int TCP_socket, PacketHeader* packet){

    // salvo il messaggio e la sua lunghezza
    char message[1000000];      // messaggio
    char length[1000000];       // lunghezza del messaggio
    int len =  Packet_serialize(message, packet);       // messaggio: packet -> string, restutisce la sua lunghezza
    snprintf(length, 1000000 , "%d", len);      // salvo la lunghezza, trasformata in str

    // invio lunghezza
    int bytes = send(TCP_socket, length, sizeof(long int) , 0);
    /* error check */ if(bytes < 0) { printf("\nTCP socket message sending failure\n"); exit(EXIT_FAILURE); }

    // invio messaggio
    while((bytes = send(TCP_socket, message, len , 0)) < 0){
        if(errno == EINTR) continue;
        /* error check */ if(bytes < 0) { printf("\nTCP socket message sending failure"); exit(EXIT_FAILURE); }
    }

}

int receiveTCP(int TCP_socket , char* msg) {

    //  ricevo la lunghezza del messaggio "vero"
    char length_message[1000000];
    int bytes = recv(TCP_socket , length_message , sizeof(long int) , 0);
    /* error check */ if(bytes < 0) { printf("\nTCP socket message reception failure\n"); exit(EXIT_FAILURE); }
    int to_receive = atoi(length_message);      // lunghezza del messaggio (atoi: str -> int)

    // ricevo il messaggio "vero"
    int total_bytes = 0;        // bytes ricevuti
            // ciclo utilizzato per prevenire erroro dovuti allo scheduling (interruzione del processo)
    while(total_bytes < to_receive){
        bytes = recv(TCP_socket , msg + total_bytes , to_receive - total_bytes , 0);
        // la posizione del buffer su cui salvare il messaggio avanza a ogni ciclo
        if(bytes < 0 && errno == EINTR) continue;
        if(bytes < 0) return bytes;
        total_bytes += bytes;
        if(bytes==0) break;
    }
    return total_bytes;
}