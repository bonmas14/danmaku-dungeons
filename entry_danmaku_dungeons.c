#define MAX_ENTITY_COUNT 4096
#define MAX_FRAME_COUNT (1 << 16)
#define CAMERA_SCALE 5.0f
#define CAMERA_SPEED 1.0f
#define PLAYER_SPEED 4.5f
#define PLAYER_BULLET_SPEED 20.0f
#define BULLET_01_SPEED 1.0f
#define SHOOT_INTERVAL 0.1f
#define PLAYER_HEALTHS 3

#define RESPAWN_POINT v2(0, -4)
// bosses are like 100 - 1000
#define ENEMY_HEALTHS 20

typedef enum entity_archetype_t { 
    ENTITY_null,
    ENTITY_camera,
    ENTITY_enemy,
    ENTITY_bullet, 
    ENTITY_player, 
} entity_archetype_t;

typedef enum bullet_archetype_t {
    BULLET_player_01,
    BULLET_enemy_01,
} bullet_archetype_t;

typedef enum entity_state_t {
    ENT_STATE_none,
    ENT_STATE_dead,
    ENT_STATE_alive,
} entity_state_t;

typedef enum sprite_id_t {
    SPRITE_error,
    SPRITE_enemy,
    SPRITE_player,
    SPRITE_bullet_01,
    SPRITE_bullet_02,
    SPRITE_MAX,
} sprite_id_t;

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

    entity_archetype_t entity_type;
    bullet_archetype_t bullet_type;
    entity_state_t state;

    int32_t healths;
    float32 radius;

    sprite_t sprite;

    float64 timer; 
    float32 movement_speed;
} entity_t;

typedef struct world_t {
    entity_t entities[MAX_ENTITY_COUNT];
} world_t;

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

sprite_t sprite_get(sprite_id_t id) {
    if (id >= SPRITE_MAX || id <= SPRITE_error) return sprites[SPRITE_error];
    if (sprites[id].image == 0) return sprites[SPRITE_error];
    return sprites[id];
}

void entity_setup_player(entity_t* entity) {
    entity->entity_type = ENTITY_player;
    entity->movement_speed = PLAYER_SPEED;
    entity->sprite = sprite_get(SPRITE_player);
    entity->state = ENT_STATE_alive;
    entity->healths = PLAYER_HEALTHS;
    entity->radius = entity->sprite.size.x / 2 / 10; // THIS IS DANMAKU GAME, player collider should be small!
}

void entity_setup_player_bullet(entity_t* entity) {
    entity->entity_type = ENTITY_bullet;
    entity->bullet_type = BULLET_player_01;
    entity->movement_speed = PLAYER_BULLET_SPEED;
    entity->state = ENT_STATE_alive;
    entity->sprite = sprite_get(SPRITE_bullet_01);
    entity->radius = entity->sprite.size.x / 2;
}

void entity_setup_bullet_01(entity_t* entity) {
    entity->entity_type = ENTITY_bullet;
    entity->bullet_type = BULLET_enemy_01;
    entity->movement_speed = BULLET_01_SPEED;
    entity->state = ENT_STATE_alive;
    entity->sprite = sprite_get(SPRITE_bullet_01);
    entity->radius = entity->sprite.size.x / 2;
}

void entity_setup_enemy(entity_t* entity) {
    entity->entity_type = ENTITY_enemy;
    entity->healths = ENEMY_HEALTHS;
    entity->movement_speed = PLAYER_SPEED;
    entity->state = ENT_STATE_alive;
    entity->sprite = sprite_get(SPRITE_enemy);
    entity->radius = entity->sprite.size.x / 2;
}

entity_t* entity_create(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        if (!world->entities[i].is_valid) {
            entity_t* ent = &(world->entities[i]);
            memset(ent, 0, sizeof(entity_t));
            ent->is_valid = true;
            return ent;
        }
    }

    return 0;
}

void entity_reset_all(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* ent = &(world->entities[i]);
        memset(ent, 0, sizeof(entity_t));
    }
}

void entity_clear_all_bullets(void) {
    for (u32 i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);
        if (!bullet->is_valid) continue;

        if (bullet->entity_type == ENTITY_bullet)
            bullet->is_valid = false;
    }
}

entity_t* check_collision_with_relevant_entities(entity_t* entity) {
    circle_t entity_circle = circle(entity->radius, entity->position);

    for (u32 i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);


        if (!bullet->is_valid) continue;

        if (bullet->entity_type != ENTITY_bullet)
            continue;

        circle_t bullet_cirlce = circle(bullet->radius, bullet->position);

        switch (bullet->bullet_type) {
            case BULLET_player_01:
                if (entity->entity_type != ENTITY_enemy) break;

                if (check_collision_circle_to_circle(bullet_cirlce, entity_circle))
                    return bullet;
                break;
            case BULLET_enemy_01:
                if (entity->entity_type != ENTITY_player) break;

                if (check_collision_circle_to_circle(bullet_cirlce, entity_circle))
                    return bullet;
                break;
        }
    }

    return 0;
}

void player_shoot(entity_t* player) {
    if ((player->timer + SHOOT_INTERVAL) < now_time) {
        player->timer = now_time;

        size_t count = 3; // defined by player power up

        Vector2 position = screen_to_world(v2(input_frame.mouse_x, input_frame.mouse_y), v2(window.width, window.height), draw_frame);

        position = v2_sub(player->position, position);

        float32 shoot_angle = PI32 / 16.0; // player concentration

        for (size_t i = 0; i < count; i++) {
            entity_t* bullet = entity_create();

            if (bullet == 0) break;

            entity_setup_player_bullet(bullet);
            bullet->position = player->position;

            float32 normalized = (float32)i / (float32)(count - 1);
            float32 angle = normalized * shoot_angle + (PI32 / 2) - (shoot_angle / 2) + atan2f(position.y, position.x) + PI32 / 2;

            float32 y = sinf(angle);
            float32 x = cosf(angle);

            bullet->direction = v2(x, y);
        }
    }
}

void update_player(void) {
    if (player_entity->healths < 0) {
        player_entity->is_valid = false;
        return;
    }

    float32 multiplier = 1.0f;
    if (is_key_down(KEY_SHIFT)) multiplier = 0.35f;

    Vector2 input_axis = v2(0, 0);
    if (is_key_down('W')) input_axis.y += 1.0f;
    if (is_key_down('S')) input_axis.y -= 1.0f;
    if (is_key_down('A')) input_axis.x -= 1.0f;
    if (is_key_down('D')) input_axis.x += 1.0f;

    input_axis = v2_normalize(input_axis);
    input_axis = v2_mulf(input_axis, multiplier * delta_time * player_entity->movement_speed);

    player_entity->position = v2_add(player_entity->position, input_axis);

    entity_t* bullet_collision = check_collision_with_relevant_entities(player_entity);

    if (bullet_collision != 0) {
        player_entity->healths -= 1;

        if (audio_player_get_current_progression_factor(impact_player) >= .99) {
            audio_player_set_progression_factor(impact_player, 0);
        }

        entity_clear_all_bullets(); 
        player_entity->position = RESPAWN_POINT;
    }

    if (is_key_down(KEY_SPACEBAR)) {
        player_shoot(player_entity);
    }
}

void update_enemy_state(entity_t* enemy) {
    if (enemy->healths < 0) {
        enemy->is_valid = false;
        return;
    }

    entity_t* bullet_collision = check_collision_with_relevant_entities(enemy);

    if (bullet_collision == 0) return;

    if (audio_player_get_current_progression_factor(impact_player) >= .99) {
        audio_player_set_progression_factor(impact_player, 0);
    }

    enemy->healths -= 1;
    bullet_collision->is_valid = false; 
}

void update_enemies(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* enemy = &(world->entities[i]);
        if (!enemy->is_valid) continue;
        if (enemy->entity_type != ENTITY_enemy) continue;
        // so here we will read our action list a do stuff
        update_enemy_state(enemy);

        if (!enemy->is_valid) continue;


        if ((enemy->timer + SHOOT_INTERVAL * 8) < now_time) {
            enemy->timer = now_time;

            size_t count = get_random_int_in_range(2, 4);

            Vector2 position = v2_sub(enemy->position, player_entity->position);

            float32 shoot_angle = PI32 / 4.0;  

            for (size_t i = 0; i < count; i++) {
                entity_t* bullet = entity_create();

                if (bullet == 0) break;

                entity_setup_bullet_01(bullet);
                bullet->position = enemy->position;

                float32 normalized = (float32)i / (float32)(count - 1);
                float32 angle = normalized * shoot_angle + (PI32 / 2) - (shoot_angle / 2) + atan2f(position.y, position.x) + PI32 / 2;

                float32 y = sinf(angle);
                float32 x = cosf(angle);

                bullet->direction = v2(x, y);
            }
        }

        // move & shoot here!
    }
}

void update_bullets(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);

        if (!bullet->is_valid) continue;
        if (!(bullet->entity_type == ENTITY_bullet)) continue;

        if (bullet->timer > 10.0) {
            bullet->is_valid = false;
            continue;
        }

        // each bullet has its unique movement pattern, but not right now as you see
        switch (bullet->bullet_type) { 
            case BULLET_enemy_01:
            case BULLET_player_01:
                bullet->position = v2_add(bullet->position, v2_mulf(bullet->direction, bullet->movement_speed * delta_time));
                bullet->timer += delta_time;
                break;
            default: 
                break;
        }
    }
}


void update_game_scene(void) {
    reset_draw_frame(&draw_frame);

    Vector2 target_pos = player_entity->position;
    animate_v2_to_target(&camera.position, target_pos, delta_time, 30.0f);

    float64 scale = camera.scale;
    
    //camera.position = player_entity->position;

    draw_frame.view = m4_make_translation(v3(camera.position.x, camera.position.y, 0));
    draw_frame.view = m4_scale(draw_frame.view, v3(scale, scale, scale));

    update_bullets();
    update_enemies();
    update_player();

    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t ent = world->entities[i];

        if (!ent.is_valid) continue;

        Vector4 color = COLOR_WHITE;

        if (ent.entity_type == ENTITY_bullet) {
            switch (ent.bullet_type) {
                case BULLET_player_01:
                    color = hex_to_rgba(0xFFE1BF7F);
                    break;
                default:
                    color = hex_to_rgba(0xFFFFFFFF);
                    break;
            }
        }

        Vector2 offset = v2_mulf(ent.sprite.size, -0.5f);
        draw_image(ent.sprite.image, v2_add(ent.position, offset), ent.sprite.size, color);
        
        // cheat
        if (ent.entity_type == ENTITY_player) {
            Vector2 size = v2_mulf(v2(ent.radius, ent.radius), 2.0f);
            offset = v2_mulf(size, -0.5f);
            draw_circle(v2_add(ent.position, offset), size, hex_to_rgba(0xFFFFFF7F)); 
        }
    }

    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); // topleft
    draw_frame.view = m4_make_scale(v3(1, 1, 1));


    int32_t offset_counter = 2;
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t ent = world->entities[i];

        if (!ent.is_valid) continue;

        if (ent.entity_type == ENTITY_player) {
            draw_text(font, tprintf("player health\t: %00d", ent.healths), 48, v2(0, -30 * offset_counter++), v2(1, 1), COLOR_WHITE);
        }

        if (ent.entity_type == ENTITY_enemy) {
            draw_text(font, tprintf("enemy health\t: %00d", ent.healths), 48, v2(0, -30 * offset_counter++), v2(1, 1), COLOR_WHITE);
        }

    }
    draw_text(font, tprintf("frametime\t: %0.2f", 1.0 / delta_time), 48, v2(0, -30), v2(1, 1), COLOR_WHITE);
}

void update_loading_screen(void) {
    reset_draw_frame(&draw_frame);
    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); 
    draw_frame.view = m4_make_scale(v3(1, 1, 1));

    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, 0, window.pixel_height, -1, 10); 
    draw_image(load_screen, v2(0,0), v2(window.pixel_width, window.pixel_height), COLOR_WHITE);
}

void game_early_init(void) {
    window.title = STR("Danmaku dungeons");
    window.x = 200;
    window.y = 90;

    window.scaled_width = 1280; 
    window.scaled_height = 720; 
    //window.enable_vsync = true;

    window.clear_color = hex_to_rgba(0x0f0f0fff);

    camera.scale = CAMERA_SCALE;

    load_screen = load_image_from_disk(STR("res/graphics/loading.png"), get_heap_allocator());
}

void game_late_init(void) {
    world = alloc(get_heap_allocator(), sizeof(world_t));

    font = load_font_from_disk(STR("res/fonts/jacquard.ttf"), get_heap_allocator());

    Audio_Source source = { 0 };
    impact_player = audio_player_get_one();

    audio_open_source_stream(&source, STR("res/audio/sfx/bullet_impact_01.wav"), get_heap_allocator());
    audio_player_set_source(impact_player, source, false);
    audio_player_set_state(impact_player, AUDIO_PLAYER_STATE_PLAYING);

    impact_player->position = v3(0, 0, 0);
    impact_player->release_when_done = false;

    sprites[SPRITE_error]     = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/error.png"),             get_heap_allocator()), .size = v2(1, 1) };
    sprites[SPRITE_player]    = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/players/player_01.png"), get_heap_allocator()), .size = v2(1, 1) };
    sprites[SPRITE_enemy]     = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/enemies/enemy_01.png"),  get_heap_allocator()), .size = v2(1, 1) };
    sprites[SPRITE_bullet_01] = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/bullets/bullet_01.png"), get_heap_allocator()), .size = v2(0.2, 0.2) };

    //os_sleep(1000); // loading kinda works
}

int entry(int argc, char **argv) {
    game_early_init();

    float64 prew_time = os_get_current_time_in_seconds();

    while (!window.should_close) {
        reset_temporary_storage();

        now_time = os_get_current_time_in_seconds();
        delta_time = now_time - prew_time;

        if (is_key_just_pressed(KEY_ESCAPE)) window.should_close = true;

        switch (program_state) {
            case GAME_init: 
                update_loading_screen(); 
                break;
            case GAME_menu:
                break;
            case GAME_editor: 
                break;
            case GAME_scene: 
                update_game_scene();
                break;
        }

        os_update(); 
        gfx_update();

        if (program_state == GAME_init) { 
            game_late_init(); 

            player_entity = entity_create();
            entity_setup_player(player_entity);
            player_entity->position = v2(0, 0);
        
            for (size_t i = 0; i < 10; i++) {
                entity_t* enemy = entity_create();
                entity_setup_enemy(enemy);
                enemy->position  = v2(get_random_float32_in_range(-10, 10), get_random_float32_in_range(-10, 10));
                enemy->timer = now_time;
            }

            program_state = GAME_scene;
        } 
        prew_time = now_time;
    }

    return 0;
}
