#include "server_threads.h"
#include <stdint.h>

// la connessione TCP viene usata solo all'inizio per scambiare i dati per il setup del giocatore (texture e ID)
// tuttavia rimane attiva per tutta la sessione. Quando il server nota che la connessione TCP con un client è interrotta
// rimuove il suo veicolo dall'engine.

void* connection_thread(void* params_ptr){

    // estraggo i parametri del thread e li inserisco in variabili
    params* thread_params = (params*)params_ptr;

    int connection_socket = thread_params->player_socket;      	// TCP socket
    char data[1000000];                             			// buffer per memorizzare i dati scambiati
    int player_ID = thread_params->player_ID;       			// player ID
    World* environment = thread_params->environment;      		// world
    Image* terrain_img = thread_params->terrain_img;        	// texture superficie
    Image* map_elevation = thread_params->map_elevation;        // elevation map
    free(thread_params);

    while(1) {

        // uso la funzione in TCP_interface.c per ricevere dati
        int len = receiveTCP(connection_socket, data);
        /* error check */ if(len < 0) { perror("\nTCP message reception faiure: "); exit(EXIT_FAILURE); }
        else if(len == 0) break; // la receive restituisce 0 quindi il client si è diconnesso, esco dal ciclo

        else {

            // estraggo l'header del pacchetto, per vedere di che tipo è
            PacketHeader* request = (PacketHeader*)Packet_deserialize(data, len);

            // richiesta ID
            if (request->type == GetId) {
                    IdPacket* id_packet = id_packet_init(GetId, player_ID);
                    sendTCP(connection_socket, &id_packet->header);
                    free(id_packet);
            }
            // immagine giocatore di default
            else if (request->type == GetTexture && ((ImagePacket*)request)->id != 0) {
                Vehicle *v = World_getVehicle(environment, ((ImagePacket*) request)->id);
                ImagePacket* texture_packet = CreateVehicleTexturePacket(v->texture, ((ImagePacket*) request)->id);
                sendTCP(connection_socket, &texture_packet->header);
            }
            // immagine giocatore personalizzata
            else if (request->type == PostTexture) {
                Vehicle *v = World_getVehicle(environment, ((ImagePacket*) request)->id);
                v->texture = ((ImagePacket*) request)->image;
            }
            // surface texture
            else if (request->type == GetTexture && ((ImagePacket*)request)->id == 0){
                ImagePacket* texture_packet = CreateSurfaceTexturePacket(terrain_img);
                sendTCP(connection_socket, &texture_packet->header);

            }
            // elevation map
            else if (request->type == GetElevation) {
                ImagePacket *elevation_packet = CreateElevationMapPacket(map_elevation);
                sendTCP(connection_socket, &elevation_packet->header);
                free(elevation_packet);
            }
            else break;
        }
    }

    // connessione TCP terminata, il client è disconnesso, libero la memoria
    printf("\nDisconnesso: %d\n", player_ID);
    Vehicle* disconnected_vehicle = World_getVehicle(environment, player_ID);
    World_detachVehicle(environment, disconnected_vehicle);
    pthread_exit(NULL);
}

void* updater_thread(void* params_ptr){

    params* p_args = (params*)params_ptr;

    // ogni volta che il server riceve un update da un client, salva l'indirizzo del client per inviargli altri update
	int client_socket = p_args->player_socket; 					 // socket usato per ricevere gli update di un client
    addressListHead* listaClientAddr = p_args->player_addresses; // lista di tutti gli indirizzi dei client
	World* world = p_args->environment;

    int res;
    char array[1000000];	// array usato per memorizzare i messaggi ricevuti dal client

    while(1){
        // server riceve i messaggi di update dal client
        struct sockaddr clientAddr;
        socklen_t len = sizeof(clientAddr);
        res = recvfrom(client_socket, array, 1000000, 0, &clientAddr, &len);
        if(res<0){
            perror("[Server] Errore nella ricezione del messaggio UDP");
            exit(EXIT_FAILURE);
        }
		// deserializza il messaggio ricevuto dal client
        VehicleUpdatePacket* mexClient = (VehicleUpdatePacket*)Packet_deserialize(array, res);
		// inserisce l'indirizzo del client nella lista degli indirizzi dei client (se non era già presente)
		if(getAddress(listaClientAddr, clientAddr)==NULL){
            insertAddress(listaClientAddr, clientAddr);
		}
        // aggiorno la posizione del veicolo
        int id = mexClient->id;
        Vehicle* v = World_getVehicle(world, id);
        v->rotational_force_update = mexClient->rotational_force;
        v->translational_force_update = mexClient->translational_force;
        World_update(world);
    }
}

void* player_updater_thread(void* params_ptr){

    params* p_args = (params*)params_ptr;

    int client_socket = p_args->player_socket;
    addressListHead* listaClientAddr = p_args->player_addresses;
	World* world = p_args->environment;

    while(1) {
        // server invia update a tutti i client -> serve WorldUpdatePacket
        WorldUpdatePacket* mexWorldUpdate = (WorldUpdatePacket*)malloc(sizeof(WorldUpdatePacket));
        PacketHeader header;
        header.type = WorldUpdate;
        mexWorldUpdate->header = header;
        mexWorldUpdate->num_vehicles = world->vehicles.size;

        ClientUpdate* clientUpdate = (ClientUpdate*)malloc(mexWorldUpdate->num_vehicles*sizeof(ClientUpdate));

		// aggiorno i vari client
        ListItem* vehicle = world->vehicles.first;
        int i = 0;
        while(vehicle!=NULL){
            Vehicle* v = (Vehicle*)vehicle;
            clientUpdate[i].id = v->id;
            clientUpdate[i].x = v->x;
            clientUpdate[i].y = v->y;
            clientUpdate[i].theta = v->theta;
            i++;
            vehicle = vehicle->next;
        }
        mexWorldUpdate->updates = clientUpdate;

		// invia messaggio di update ai vari client connessi
        addressListItem* client = listaClientAddr->first;
        char array[1000000];	// array usato per memorizzare i messaggi ricevuti dal client
        int bytes = Packet_serialize(array, &(mexWorldUpdate->header));
        while(client!=NULL){
            struct sockaddr clientAddr = client->address;
            int res = sendto(client_socket, array, bytes, 0, &clientAddr, sizeof(clientAddr));
            if(res<0){
                perror("[Server] Errore nell'invio del messaggio UDP");
                exit(EXIT_FAILURE);
            }
            client = client->next;
        }

        usleep(50000); // per essere sincronizzato
    }
}
