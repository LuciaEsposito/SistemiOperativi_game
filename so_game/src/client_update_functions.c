#include "client_update_functions.h"

// invia aggiornamenti del veicolo al server (funzione ausiliaria per updateSender)
void sendVehicleUpdate(int id, float rotationalForce, float translationalForce, int socket, const struct sockaddr* address){
   
	// creo messaggio VehicleUpdatePacket da inviare al server
    VehicleUpdatePacket* mex = (VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
    PacketHeader header;
    header.type = VehicleUpdate;
	mex->header = header;
    mex->id = id;
    mex->rotational_force = rotationalForce;
    mex->translational_force = translationalForce;
    
	// invio messaggio mex tramite sendto
    char* array = (char*)malloc(1000000*sizeof(char));	// array che memorizza il messaggio da inviare
    int dimMex = Packet_serialize(array, &(mex->header));
    int res = sendto(socket, array, dimMex , 0, address, sizeof(*address));
    if(res<0){ 
		perror("[Client] Errore nell'invio del messaggio UDP"); 
		exit(EXIT_FAILURE); 
	}
	free(array);	// rilascio memoria
}

// funzione che riceve gli update dal server
void* updateReceiver(void* args){

    client_thread* ct_args = (client_thread*)args;	// cast 

    World* world = ct_args->world;
    int socket_UDP = ct_args->socket_UDP;
    struct sockaddr* address_UDP = (struct sockaddr*)ct_args->address_UDP;

    char array[1000000];     // array per contenere i vari messaggi
    while(1) {
        memset(array , 0 , 1000000 * sizeof(char));	// ad ogni ciclo azzero il contenuto dell'array

        // ricevo messaggi dal server tramite recvfrom
        ssize_t lengthAddr = sizeof(*address_UDP);
        int dimMex = recvfrom(socket_UDP, array, 1000000, 0, (struct sockaddr*)address_UDP, (socklen_t*)&lengthAddr);
        if(dimMex<0){ 
			perror("[Client] Errore nella ricezione del messaggio UDP"); 
			exit(EXIT_FAILURE); 
		}

		// deserializzo il messaggio arrivato dal server per poter usare le varie informazioni e aggiornare i campi
        WorldUpdatePacket* mexServer = (WorldUpdatePacket*)Packet_deserialize(array, dimMex);

        int num_vehicles = mexServer->num_vehicles;
        int arrayIDvehicles[30];	// array che memorizza tutti gli ID dei veicoli presenti nel gioco
        int i;
        for(i=0; i<num_vehicles; i++){
            arrayIDvehicles[i] = mexServer->updates[i].id;
        }
        
		/* se c'è un nuovo giocatore */
        if(num_vehicles > world->vehicles.size){
            for(i=0; i<num_vehicles; i++){
                if(World_getVehicle(world, arrayIDvehicles[i])==NULL){	// cioè è il nuovo giocatore
                    // chiede al server l'immagine del nuovo veicolo
                    ImagePacket* vehicleImage = CreateVehicleTextureRequestPacket(arrayIDvehicles[i]);
                    sendTCP(ct_args->socket_TCP, &(vehicleImage->header));
					// riceve dal server l'immagine del nuovo veicolo
                    char* vehicleImageArray = (char*)malloc(1000000*sizeof(char));// array per contenere l'immagine serializzata del nuovo veicolo
                    int len = receiveTCP(ct_args->socket_TCP, vehicleImageArray);
                    if(len<0){ 
						perror("[Client] Errore nella ricezione del messaggio TCP dal server"); 
						exit(EXIT_FAILURE); 
					}
                    // crea il nuovo veicolo e lo aggiunge a world
                    vehicleImage = (ImagePacket*)Packet_deserialize(vehicleImageArray, len);
                    Vehicle* v = (Vehicle*)malloc(sizeof(Vehicle));
                    Vehicle_init(v, world, arrayIDvehicles[i], vehicleImage->image);
                    World_addVehicle(world, v);
                    printf("Giocatore connesso: %d\n", arrayIDvehicles[i]);
					free(vehicleImageArray);	// rilascio memoria
                }
            }
        }
        /* se c'è un giocatore in meno */
        else if(num_vehicles < world->vehicles.size){
            ListItem* item=world->vehicles.first;
            // cerco l'ID del veicolo che ha abbandonato il gioco
            start:
            while(item){
                Vehicle* v = (Vehicle*)item;
                int vehicle_id = v->id;
                for(int n=0; n<num_vehicles; n++){
                    if(arrayIDvehicles[n] == vehicle_id){
                        item = item->next;
                        goto start;
                    }
                }
                // trovato ID del veicolo che ha abbandonato il gioco -> si elimina da world
                World_detachVehicle(world, v);
                item = item->next;
                printf("Giocatore disconnesso: %d\n", vehicle_id );
				Vehicle_destroy(v);
            }
        }
        /* aggiorno le coordinate degli altri veicoli */
        for(i=0; i<world->vehicles.size; i++){
            Vehicle* v = World_getVehicle(world, mexServer->updates[i].id);
            v->x = mexServer->updates[i].x;
            v->y = mexServer->updates[i].y;
			v->theta = mexServer->updates[i].theta;
        }
        usleep(50000);	// suspends execution of the calling thread for (at  least) 50000 microseconds
    }
}

// funzione che invia gli update al server
void* updateSender(void* args){

    client_thread* ct_args = (client_thread*)args;	// cast

    int id = ct_args->ID_v;
    Vehicle* vehicle = ct_args->vehicle;
    int socket_UDP = ct_args->socket_UDP;
    const struct sockaddr* address_UDP = (struct sockaddr*)ct_args->address_UDP;

    char array[1000000];	// array per contenere i vari messaggi
    while(1) {
		// invia messaggio al server grazie alla funzione sendVehicleUpdate
        float rotationalForce = vehicle->rotational_force_update;
        float translationalForce = vehicle->translational_force_update;
        sendVehicleUpdate(id, rotationalForce, translationalForce, socket_UDP, address_UDP);

        memset(array, 0, 1000000 * sizeof(char));	// ad ogni ciclo azzero il contenuto dell'array

        usleep(50000);	// suspends execution of the calling thread for (at  least) 50000 microseconds
    }
}
