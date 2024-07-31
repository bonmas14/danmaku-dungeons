Gfx_Image* load_screen = 0;
Gfx_Font* font = 0;
Audio_Player* menu_music_player;
Audio_Player* impact_player;

sprite_t sprites[SPRITE_MAX];

world_t* world = 0;

game_state_t program_state = GAME_init;

float64 now_time = 0;
float64 flow_update_timer = 0;
float64 delta_time = 0;

entity_t* player_entity;

Vector2 respawn_point;

struct {
    Vector2 position;
    float32 scale;
} camera;

