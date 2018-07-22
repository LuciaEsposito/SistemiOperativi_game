
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "so_game_protocol.h"
#include "TCP_interface.h"
#include "messages.h"
#include "client_update_functions.h"

pthread_t updater_thread;		// update receiver
pthread_t update_sender_thread;		// update sender
ImagePacket* surface_mesh_response;
ImagePacket* surfaceTexture_packet;
int sock; 			// TCP socket
World environment;		// environment
Vehicle* player_vehicle;	// The vehicle
int socket_UDP;		// UDP socket

void client_shutdown(int s) {
	if(s==SIGINT){
		printf("Ricevuto segnale di terminazione\n");
	}
	else if(s==SIGHUP){
		printf("Errore nella connessione con il server\n");
	}
	printf("\nshutting down\n");

	//free packet memory
	Packet_free(&(surface_mesh_response->header));
	Packet_free(&(surfaceTexture_packet->header));
	printf("\nimage memory cleaned\n");

	// chiudo il socket
	int res = close(sock);
	/* error check */ if(res < 0) { printf("\nTCP socket failed closing"); exit(EXIT_FAILURE); }
	res = close(socket_UDP);
	/* error check */ if(res < 0) { printf("\nUDP socket failed closing"); exit(EXIT_FAILURE); }
	printf("\nsockets closed\n");

	// libero la memoria
	World_destroy(&environment);
	// vehicle destruction
	Vehicle_destroy(player_vehicle);
	printf("\nengine memory released\n");

	// terminating updaters
	res = pthread_cancel(updater_thread);
	if(res!=0){
		perror("Errore nella terminazione dell'updater_thread");
		exit(EXIT_FAILURE);
	}
	res = pthread_cancel(update_sender_thread);
	if(res!=0){
		perror("Errore nella terminazione dell'update_sender_thread");
		exit(EXIT_FAILURE);
	}
	printf("\nupdaters terminated\n");
}

int main(int argc, char **argv) {

	if (argc<2) {
		printf("\nPlease enter server IP and optionally your player texture (or server will assign one for you)\n");
		exit(-1);
	}

	struct sigaction signals;
	memset(&signals, '\0', sizeof(signals));
    signals.sa_handler = client_shutdown;
	if(sigaction(SIGINT, &signals, NULL)<0){
		perror("Non si può gestire SIGINT");
	}
	if(sigaction(SIGHUP, &signals, NULL)<0){
		perror("Non si può gestire SIGHUP");
	}

	// estraggo l'indirizzo del server dalla linea di comando
	char* server_address = argv[1];

	// connect to the server:
  	//   -1. get an id
  	//   -2. send your texture to the server (so that all can see you)
  	//   -3. get an elevation map
  	//   -4. get the texture of the surface

	// creo socket TCP
	sock = socket(AF_INET, SOCK_STREAM, 0);
	/* error check */ if(sock < 0) { perror("\nTCP socket initialization failure: "); exit(EXIT_FAILURE); }
	// imposto i parametri per la connessione TCP
	struct sockaddr_in address = {0};
	address.sin_port        = htons(2000);
	address.sin_family      = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");

	// apro connessione con il socket del server (indirizzo in address)
	int res = connect(sock, (struct sockaddr*) &address, sizeof(struct sockaddr_in));
	/* error check */ if(res < 0) { perror("\nTCP connection initialization falure: "); exit(EXIT_FAILURE); }

	// 1. Requesting an ID
	char* id_response_contents = (char*)calloc(1500000, sizeof(char));	// buffer per memorizzare l'ID ricevuto
	// invio richiesta
	IdPacket* id_request = id_packet_init(GetId, -1);		// pacchetto per richiesta ID
	sendTCP(sock , &id_request->header);		// invio su TCP
	Packet_free(&id_request->header);
	// analizzo la risposta
	int len = receiveTCP(sock , id_response_contents);
	/* error check */ if (len < 0) { perror("\nTCP reception failure in ID request: "); exit(EXIT_FAILURE); }
	IdPacket* id_response = (IdPacket*)Packet_deserialize(id_response_contents, len);	// str -> struct
	int my_id = id_response->id;
	Packet_free(&id_response->header);
	printf("\nConnessione al server.\nID: %d\n",my_id);
	free(id_response_contents);

    // 2. Requesting image to use as texture for the vehicle
	Image* vehicle_image;
	char* vehicle_texture_data = (char*)calloc(1500000, sizeof(char));
	if(argc == 3){		// se ci sono 3 argomenti allora la texture del giocatore va prelevata del pc e inviata
		vehicle_image = Image_load(argv[2]);		// recupera il file
		/* error check */ if(vehicle_image == 0) perror("\nImage load error: ");
		ImagePacket* vehicle_texture_response = CreateVehicleTexturePacket(vehicle_image, my_id);// inserisco nel pacchetto
		sendTCP(sock, &vehicle_texture_response->header);	// invio
	} else {		// non c'è un terzo argomento quindi chiedo al server la texture di default per i veicoli
		ImagePacket* vehicle_texture_request = CreateVehicleTextureRequestPacket(my_id);
		// pacchetto per la richiesta di texture
		sendTCP(sock, &vehicle_texture_request->header);
		int len = receiveTCP(sock, vehicle_texture_data);
		/* error check */ if (len < 0) { perror("\nTCP reception failure in veichle texture request: "); exit(EXIT_FAILURE); }
		Packet_free(&vehicle_texture_request->header);
		ImagePacket* vehicleTexture_response = (ImagePacket*)Packet_deserialize(vehicle_texture_data, len);
		// se l'ID è 0 allora il pacchetto immagine è riferito alla mappa
		if( vehicleTexture_response->id > 0 && (vehicleTexture_response->header).type == PostTexture) {
			vehicle_image = vehicleTexture_response->image;
		}
		else {
			perror("\nImage corrupted: ");
			exit(EXIT_FAILURE);
		}
		free(vehicle_texture_data);
    }

	// 3. Requesting an elevation map (rilievo)
	char* sm_data = (char*)calloc(1500000, sizeof(char));
	Image*	surface_mesh;		// indirizzo per memorizzare rilievo
	ImagePacket* surface_mesh_request = CreateElevationMapRequestPacket();	// pacchetto di richiesta
	sendTCP(sock, &surface_mesh_request->header);		//invio
	len = receiveTCP(sock, sm_data);		// recezione risposta
	/* error check */ if (len < 0) { perror("\nTCP reception failure in elevation map request: "); exit(EXIT_FAILURE); }
	Packet_free(&surface_mesh_request->header);
	surface_mesh_response = (ImagePacket*)Packet_deserialize(sm_data, len);	// str -> struct
	if( (surface_mesh_response->header).type == PostElevation && surface_mesh_response->id == 0) {
		surface_mesh = surface_mesh_response->image;		// inserisco nella variabile
	}
	else {
		perror("\nImage corrupted: ");
		exit(EXIT_FAILURE);
	}
	free(sm_data);

	// 4. Requesting the image used as surface texture
	char* st_data = (char*)calloc(1500000, sizeof(char));
	Image*  surface_image;
	ImagePacket* surfaceTexture_request_packet = CreateSurfaceTextureRequestPacket();
    sendTCP(sock, &surfaceTexture_request_packet->header);
    len = receiveTCP(sock, st_data);
	/* error check */ if (len < 0) { perror("\nTCP reception failure in surface texture request: "); exit(EXIT_FAILURE); }
    Packet_free(&surfaceTexture_request_packet->header);
    surfaceTexture_packet = (ImagePacket*)Packet_deserialize(st_data, len);
	if( surfaceTexture_packet->id == 0 && (surfaceTexture_packet->header).type == PostTexture) {
		surface_image = surfaceTexture_packet->image;
	}
	else {
		perror("Image corrupted: ");
		exit(EXIT_FAILURE);
	}
	free(st_data);


	// creo il world e il veicolo con le variabili settate prima
	World_init(&environment, surface_mesh, surface_image, 0.5, 0.5, 0.5);
	player_vehicle=(Vehicle*)malloc(sizeof(Vehicle));
	Vehicle_init(player_vehicle, &environment, my_id, vehicle_image);
	World_addVehicle(&environment, player_vehicle);

	// spawn a thread that will listen the update messages from
	// the server, and sends back the controls
  	// the update for yourself are written in the desired_*_force
  	// fields of the vehicle variable
  	// when the server notifies a new player has joined the game
  	// request the texture and add the player to the pool

	// creo socket UDP
	socket_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in address_UDP={
		.sin_family = AF_INET,
		.sin_port = htons(2001),
		.sin_addr.s_addr = inet_addr(server_address)
	};

	// inizializzo i parametri da passare alle funzioni dei thread
	client_thread runner_args={
		.ID_v = my_id,
		.socket_TCP = sock,
		.socket_UDP = socket_UDP,
		.address_UDP = &address_UDP,
		.vehicle = player_vehicle,
		.world = &environment,
		.image = vehicle_image
	};

	// creo i thread per ricevere e inviare gli update
	res = pthread_create(&updater_thread, NULL, updateReceiver, &runner_args);
	if(res!=0){
		perror("Errore creazione updater_thread");
		exit(EXIT_FAILURE);
	}
	res = pthread_create(&update_sender_thread, NULL, updateSender, &runner_args);
	if(res!=0){
		perror("Errore creazione update_sender_thread");
		exit(EXIT_FAILURE);
	}

	// launching the engine
	WorldViewer_runGlobal(&environment, player_vehicle, &argc, argv);

}
