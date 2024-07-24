Gfx_Image* load_screen = 0;
Gfx_Font* font = 0;

Audio_Player* impact_player;

sprite_t sprites[SPRITE_MAX];

world_t* world = 0;

game_state_t program_state = GAME_init;

float64 now_time = 0;
float64 delta_time = 0;

entity_t* player_entity;

struct {
    Vector2 position;
    float32 scale;
} camera;

struct {
    int32_t min_tunnel_count; 
    int32_t max_tunnel_count; 

    int32_t min_room_size; 
    int32_t max_room_size; 
} gen_conf;
