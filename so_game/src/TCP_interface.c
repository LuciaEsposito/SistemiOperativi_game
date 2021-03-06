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
#include "TCP_interface.h"

// funzione per inviare un messaggio con TCP
void sendTCP(int TCP_socket, PacketHeader* packet){

    // salvo il messaggio e la sua lunghezza
    char* message = (char*)malloc(sizeof(char)*1000000);      // messaggio
    char* length = (char*)malloc(sizeof(char)*1000000);       // lunghezza del messaggio
    
	int len =  Packet_serialize(message, packet);       // messaggio: packet -> string, restituisce la sua lunghezza
    snprintf(length, 1000000 , "%d", len);      // salvo la lunghezza, trasformata in str

    // invio lunghezza
    int bytes = send(TCP_socket, length, sizeof(long int), 0);
    /* error check */ if(bytes < 0) { perror("\nTCP socket message sending failure"); exit(EXIT_FAILURE); }

    // invio messaggio
    while((bytes = send(TCP_socket, message, len, 0)) < 0){
        if(errno == EINTR) continue;
        /* error check */ if(bytes < 0) { perror("\nTCP socket message sending failure"); exit(EXIT_FAILURE); }
    }

	free(message);
	free(length);

}

// funzione per ricevere un messaggio con TCP
int receiveTCP(int TCP_socket, char* msg){

    //  ricevo la lunghezza del messaggio
    char* length_message = (char*)malloc(sizeof(char)*1000000);
    int bytes = recv(TCP_socket, length_message, sizeof(long int), 0);
    /* error check */ if(bytes < 0) { perror("\nTCP socket message reception failure"); exit(EXIT_FAILURE); }
    int to_receive = atoi(length_message);      // lunghezza del messaggio (atoi: str -> int)

    // ricevo il messaggio "vero"
    int total_bytes = 0;        // bytes ricevuti
    // ciclo utilizzato per prevenire errori dovuti allo scheduling (interruzione del processo)
    while(total_bytes < to_receive){
        bytes = recv(TCP_socket, msg + total_bytes, to_receive - total_bytes, 0);
        // la posizione del buffer su cui salvare il messaggio avanza a ogni ciclo
        if(bytes < 0 && errno == EINTR) continue;
        if(bytes < 0) return bytes;
        total_bytes += bytes;
        if(bytes==0) break;
    }

	free(length_message);
	return total_bytes;
}
