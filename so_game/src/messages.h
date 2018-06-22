#include "so_game_protocol.h"    
#include "world.h"               

ImagePacket* CreateSurfaceTexturePacket(Image* selected_image);

ImagePacket* CreateElevationMapPacket(Image* selected_image);

ImagePacket* CreateVehicleTexturePacket(Image* selected_image, int client_id);

ImagePacket* CreateDefaultTexturePacket(int client_id);

ImagePacket* CreateVehicleTextureRequestPacket(int client_id);

ImagePacket* CreateElevationMapRequestPacket(void);

ImagePacket* CreateSurfaceTextureRequestPacket(void);

IdPacket* id_packet_init(Type header_type, int id);

