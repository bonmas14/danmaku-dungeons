typedef enum entity_archetype_t { 
    ENTITY_null,
    ENTITY_camera,
    ENTITY_enemy,
    ENTITY_item,
    ENTITY_bullet, 
    ENTITY_player, 
} entity_archetype_t;

typedef enum bullet_archetype_t {
    BULLET_player_01,
    BULLET_enemy_01,
} bullet_archetype_t;

typedef enum item_archetype_t {
    ITEM_gold,
} item_archetype_t;

typedef enum entity_state_t {
    ENT_STATE_none,
    ENT_STATE_dead,
    ENT_STATE_alive,
} entity_state_t;

typedef enum sprite_id_t {
    SPRITE_error,
    SPRITE_enemy,
    SPRITE_gold,
    SPRITE_player,
    SPRITE_bullet_01,
    SPRITE_bullet_02,
    SPRITE_MAX,
} sprite_id_t;

typedef enum block_archetype_t {
    BLOCK_empty,
    BLOCK_floor,
    BLOCK_wall,
    BLOCK_MAX,
} block_archetype_t;

typedef enum game_state_t {
    GAME_init,
    GAME_menu,
    GAME_editor,
    GAME_scene,
} game_state_t;

typedef struct sprite_t {
    Gfx_Image* image;
    Vector2 size;
} sprite_t;

typedef struct entity_t {
    bool is_valid;

    Vector2 position;
    Vector2 direction;

    entity_state_t state;

    entity_archetype_t entity_type;
    bullet_archetype_t bullet_type;
    item_archetype_t item_type;

    int32_t healths;
    float32 radius;

    sprite_t sprite;

    float64 timer; 
    float32 movement_speed;
} entity_t;

typedef struct block_t {
    block_archetype_t type;
    entity_t prototype_entity;
} block_t;

typedef struct world_t {
    entity_t entities[MAX_ENTITY_COUNT];

    block_t world_map[MAP_WIDTH * MAP_HEIGHT];
} world_t;
