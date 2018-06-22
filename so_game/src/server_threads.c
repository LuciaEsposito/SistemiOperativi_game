#include "server_threads.h"
#include <stdint.h>

// la connessione TCP viene usata solo all'inizio per scambiare i dati per il setup del giocatore (texture e ID)
// tuttavia rimane attiva per tutta la sessione. Quando il server nota che la connessione TCP con un client è interrotta
// rimuove il suo veicolo dall'engine.

void *connection_thread(void *params_ptr){

    // estraggo i parametri del thread e li inserisco in variabili

    params* thread_params = (params*)params_ptr;

    int connection_socket = thread_params->player_socket;      // TCP socket
    char data[1000000];                             // buffer per memorizzare i dati scambiati
    int player_ID = thread_params->player_ID;       // player ID
    World* environment = thread_params->environment;      // world
    Image* terrain_img = thread_params->terrain_img;        // texture superficie
    Image* map_elevation = thread_params->map_elevation;        // rilievo
    free(thread_params);

    while(1) {

        // uso la funzione in TCPinterface.c per ricevere dati
        int len = receiveTCP(connection_socket , data);
        /* error check */ if(len < 0) { printf("\nTCP message reception faiure\n"); exit(EXIT_FAILURE); }
        else if(len == 0) break; // la receive restituisce 0 quindi il client si è diconnesso, esco dal ciclo

        else {

            // estraggo l'header del pacchetto, per vedere di che tipo è
            PacketHeader* request = (PacketHeader*)Packet_deserialize(data , len);

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
            // immagine terreno
            else if (request->type == GetTexture && ((ImagePacket*)request)->id == 0){
                ImagePacket* texture_packet = CreateSurfaceTexturePacket(terrain_img);
                sendTCP(connection_socket, &texture_packet->header);

            }
            // rilievo
            else if (request->type == GetElevation) {
                ImagePacket *elevation_packet = CreateElevationMapPacket(map_elevation);
                sendTCP(connection_socket, &elevation_packet->header);
                free(elevation_packet);
            }
            else break;
        }
    }

    // connessione TCP terminata, il client è disconnesso, libero la memoria
    printf("\nDisconnected: %d\n", player_ID);
    Vehicle* disconnected_vehicle = World_getVehicle(environment, player_ID);
    World_detachVehicle(environment, disconnected_vehicle);
    pthread_exit(NULL);
}

