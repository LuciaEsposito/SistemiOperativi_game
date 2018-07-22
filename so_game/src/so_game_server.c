
// #include <GL/glut.h> // not needed here
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
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
#include "server_threads.h"

pthread_t updater;
World environment;
Image* elevation;
Image* terrain_img;
Image* player_img;
struct sockaddr_in* client_addr;
params* updater_params;
ListHead sockets;
int socket_UDP;
addressListHead player_addresses;
pthread_t players_updater;

void server_shutdown(int s) {

	printf("\nshutting down\n");

	// terminating updater thread
	printf("\nterminating updater\n");
	int res = pthread_cancel(updater);
	/* error check */ if(res != 0) { printf("\nWorld update thread termination failure\n"); exit(EXIT_FAILURE); }
	close(socket_UDP);

	// closing sockets
	ListItem* current_socket = sockets.first;
	while(current_socket != NULL){
		List_detach(&sockets, current_socket);
		close(current_socket->info);
		current_socket = current_socket->next;
	}

	// environment deletion
	World_destroy(&environment);

	// releasing memory
	printf("\nreleasing memory\n");
	Image_free(elevation);
	Image_free(terrain_img);
	Image_free(player_img);
	free(client_addr);
	free(updater_params);

	printf("\nserver closing\n");
	exit(EXIT_SUCCESS);

	raise(SIGINT);      // interrompo il server in maniera sicura
}

int main(int argc, char **argv) {

	if (argc<4) {
		printf("\nPlease enter vehicle default texture, elevation image and texture image\n");
		exit(-1);
	}

	struct sigaction signals;
	memset(&signals, '\0', sizeof(signals));
    signals.sa_handler = server_shutdown;
	if(sigaction(SIGINT, &signals, NULL)<0){
		perror("Non si può gestire SIGINT");
	}

	// creo socket UDP
	socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(socket_UDP, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){	// (evita l'errore "Address already in use")
    	printf("setsockopt(SO_REUSEADDR) failed");
	}
    if(socket_UDP<0){
		perror("Errore nella creazione del socket UDP");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	(&address)->sin_family = AF_INET;
	(&address)->sin_port = htons(2001);					// converte in network byte order
	(&address)->sin_addr.s_addr = htonl(INADDR_ANY);	// converte in network byte order

	// connetto socket_UDP all'inidrizzo del server
	int res = bind(socket_UDP, (struct sockaddr*) &address, sizeof(address));
	if(res<0){
		perror("Errore nel bind del socket UDP");
		exit(EXIT_FAILURE);
	}

    // preparo l'indirizzo del socket
    struct sockaddr_in TCP_address = {0};       // dichiaro la struttura indirizzo
   	TCP_address.sin_addr.s_addr = INADDR_ANY; // accetto connessioni da tutte le interfacce
	TCP_address.sin_port = htons(2000);     // porta del server socket TCP principale
	TCP_address.sin_family = AF_INET;       // famiglia

	// preparo il socket
	int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);		// socket fd
	/* error check */ if(TCP_socket < 0) { perror("\nTCP socket initialization failure"); exit(EXIT_FAILURE); }
    if (setsockopt(TCP_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");       // configuro il socket per utilizzare lo stesso indirizzo
	res = bind(TCP_socket, (struct sockaddr *)&TCP_address, sizeof(struct sockaddr_in)); // leggo socket e address
	/* error check */ if(res < 0) { perror("\nTCP socket binding failure"); exit(EXIT_FAILURE); }

	// avvio il socket per accettare connessioni (max 10)
	res = listen(TCP_socket, 10);
	/* error check */ if(res < 0) { perror("\nTCP socket listening failure"); exit(EXIT_FAILURE); }

    // carico le immagini dagli uri sulla riga di comando
    char* v_t_uri=argv[1];      // vehicle texture URI
    char* s_e_uri=argv[2];      // surface elevation URI
    char* s_t_uri=argv[3];      // surface texture URI
	elevation = Image_load(s_e_uri);
	/* error check */ if(elevation == 0) { perror("\nSurface mesh loading error"); exit(EXIT_FAILURE); }
	terrain_img = Image_load(s_t_uri);
	/* error check */ if(terrain_img == 0) { perror("\nSurface texture loading error"); exit(EXIT_FAILURE); }
	player_img = Image_load(v_t_uri);
	/* error check */ if(player_img == 0) { perror("\nVehicle texture loading error"); exit(EXIT_FAILURE); }

	// lancio l'engine
	World_init(&environment, elevation, terrain_img, 0.5, 0.5, 0.5);

	// inizializzo la lista di socket TCP
	List_init(&sockets);

	// player addresses list
	addressList_init(&player_addresses);

	// Initialising a new thread to handle the players
	params* updater_params = (params*)malloc(sizeof(params));
	updater_params->player_socket = socket_UDP;
	updater_params->player_ID = -1;
	updater_params->environment = &environment;
	updater_params->player_addresses = &player_addresses;
	res = pthread_create(&updater, NULL, updater_thread, (void*)updater_params);
	/* error check */ if(res != 0) { perror("\nserver updater thread launch failure"); exit(EXIT_FAILURE); }
	res = pthread_create(&players_updater, NULL, player_updater_thread, (void*)updater_params);
	/* error check */ if(res != 0) { perror("\nplayers updater thread launch failure"); exit(EXIT_FAILURE); }
	client_addr = calloc(1, sizeof(struct sockaddr_in));

	printf("\nRunning server on port:2000\n");

	int current_ID = 1;		// primo ID giocatore da usare
	int current_socket;		// primo socket da usare (descrittore)
	while (1) {
		// ciclo per accettare le connessioni. L'interruzione è gestita dal gestore segnali.
        // L'interruzione è gestita "esternamente" poichè il ciclo contiene chiamate bloccanti che non ne
        // permetterebbero la chiusura in tempo reale.
		socklen_t address_length = sizeof(struct sockaddr_in);
		// accetto una nuova connessione al socket TCP principale e la inoltro al current socket
		current_socket = accept(TCP_socket, (struct sockaddr*)client_addr, &address_length);
		if (current_socket == -1 && errno == EINTR) continue; // controllo interruzioni
		/* error check */ if(current_socket < 0) { perror("\nTCP socket opening failure"); exit(EXIT_FAILURE); }
		insertSockFD(&sockets , current_socket);    // inserisco il nuovo socket nella lista sockets

		// creo un nuovo veicolo e lo inserisco nell'engine
		Vehicle* new_vehicle=(Vehicle*)malloc(sizeof(Vehicle));
		Vehicle_init(new_vehicle, &environment, current_ID, player_img);
		World_addVehicle(&environment, new_vehicle);

		// creo e lancio un nuovo thread per gestire l'inserimento del nuovo giocatore

        // imposto la struttura con i parametri che servono al nuovo giocatore
		params* ct_params = (params*)malloc(sizeof(params));
		ct_params->player_socket = current_socket;      // socket del player
		ct_params->player_ID = current_ID;  // ID del giocatore
		ct_params->environment = &environment;  // world
		ct_params->terrain_img = terrain_img;   // surface texture 
		ct_params->map_elevation = elevation;   // elevation map

		printf("\nConnesso: %d\n", current_ID);

		// creo e lancio un nuovo thread con la funzione connection_thread (in server_threads.c)
        // per gestire la connessione
        pthread_t connection_handler;
		res = pthread_create(&connection_handler, NULL, connection_thread, (void*)ct_params);
		/* error check */ if(res != 0) { perror("\nThread launch failure"); exit(EXIT_FAILURE); }
		res = pthread_detach(connection_handler);
		/* error check */ if(res != 0) { perror("\nThread detach failure"); exit(EXIT_FAILURE); }

		current_ID = current_ID +1;
	}
}
