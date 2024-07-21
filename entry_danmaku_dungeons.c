#define MAX_ENTITY_COUNT 4096
#define CAMERA_SCALE 5.0f
#define CAMERA_SPEED 1.0f
#define PLAYER_SPEED 4.5f
#define PLAYER_BULLET_SPEED 20.0f
#define BULLET_01_SPEED 2.0f
#define SHOOT_INTERVAL 0.1f
#define PLAYER_HEALTHS 3
#define ENEMY_HEALTHS 20 // bosses are like 100 - 1000

typedef enum entity_archetype_t { 
    ENTITY_nil,
    ENTITY_enemy,
    ENTITY_bullet, 
    ENTITY_player, 
    ENTITY_camera, 
} entity_archetype_t;

typedef enum bullet_archetype_t {
    BULLET_player_01,
    BULLET_enemy_01,
} bullet_archetype_t;

typedef enum entity_alive_state_t {
    ENT_STATE_none,
    ENT_STATE_dead,
    ENT_STATE_alive,
} entity_alive_state_t;

typedef enum sprite_id_t {
    SPRITE_error,
    SPRITE_enemy,
    SPRITE_player,
    SPRITE_bullet_01,
    SPRITE_bullet_02,
    SPRITE_MAX,
} sprite_id_t;

typedef struct sprite_t {
    Gfx_Image* image;
    Vector2 size;
} sprite_t;

typedef struct entity_t {
    bool is_valid;
    bool is_visible;

    entity_archetype_t entity_type;
    bullet_archetype_t bullet_type;
    entity_alive_state_t state;
    sprite_t sprite;

    Vector2 position;
    Vector2 direction;

    float32 radius;
    int32_t healths;

    float64 timer; // gonna need this for bullets

    float64 movement_speed;
    float64 camera_scale;
} entity_t;

typedef struct world_t {
    entity_t entities[MAX_ENTITY_COUNT];
} world_t;

Gfx_Font* font = 0;
world_t* world = 0;

Range2f borders =  { 0 };

sprite_t sprites[SPRITE_MAX];

entity_t* player;
entity_t* camera;

float64 now_time = 0;
float64 delta_time = 0;

int32_t collision_checks = 0;

sprite_t sprite_get(sprite_id_t id) {
    if (id >= SPRITE_MAX || id <= SPRITE_error) return sprites[SPRITE_error];
    if (sprites[id].image == 0) return sprites[SPRITE_error];
    return sprites[id];
}

void entity_setup_player(entity_t* entity) {
    entity->entity_type = ENTITY_player;
    entity->is_visible = true;
    entity->movement_speed = PLAYER_SPEED;
    entity->sprite = sprite_get(SPRITE_player);
    entity->state = ENT_STATE_alive;
    entity->healths = PLAYER_HEALTHS;
    entity->radius = entity->sprite.size.x / 2 / 10; // THIS IS DANMAKU GAME, player collider should be small!
}

void entity_setup_player_bullet(entity_t* entity) {
    entity->entity_type = ENTITY_bullet;
    entity->bullet_type = BULLET_player_01;
    entity->is_visible = true;
    entity->movement_speed = PLAYER_BULLET_SPEED;
    entity->state = ENT_STATE_alive;
    entity->sprite = sprite_get(SPRITE_bullet_01);
    entity->radius = entity->sprite.size.x / 2;
}
void entity_setup_bullet_01(entity_t* entity) {
    entity->entity_type = ENTITY_bullet;
    entity->bullet_type = BULLET_enemy_01;
    entity->is_visible = true;
    entity->movement_speed = BULLET_01_SPEED;
    entity->state = ENT_STATE_alive;
    entity->sprite = sprite_get(SPRITE_bullet_01);
    entity->radius = entity->sprite.size.x / 2;
}

void entity_setup_enemy(entity_t* entity) {
    entity->entity_type = ENTITY_enemy;
    entity->is_visible = true;
    entity->healths = ENEMY_HEALTHS;
    entity->movement_speed = PLAYER_SPEED;
    entity->state = ENT_STATE_alive;
    entity->sprite = sprite_get(SPRITE_enemy);
    entity->radius = entity->sprite.size.x / 2;
}

void entity_setup_camera(entity_t* entity) {
    entity->entity_type = ENTITY_camera;
    entity->is_visible = false;
    entity->state = ENT_STATE_none;
    entity->camera_scale = CAMERA_SCALE;
    entity->movement_speed = CAMERA_SPEED;
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

        collision_checks++;

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

void update_player(void) {
    Vector2 input_axis = v2(0, 0);

    float32 multiplier = 1.0f;

    if (is_key_down('W'))
        input_axis.y += 1.0f;
    if (is_key_down('S'))
        input_axis.y -= 1.0f;
    if (is_key_down('A'))
        input_axis.x -= 1.0f;
    if (is_key_down('D'))
        input_axis.x += 1.0f;


    if (is_key_down(KEY_SHIFT)) {
        multiplier = 0.5f;
    } 

    input_axis = v2_normalize(input_axis);

    input_axis = v2_mulf(input_axis, delta_time * multiplier * player->movement_speed);

    player->position = v2_add(player->position, input_axis);

    entity_t* bullet_collision = check_collision_with_relevant_entities(player);

    if (bullet_collision != 0) {
        player->healths -= 1;
        // bullet_collision->is_valid = false; 
        entity_clear_all_bullets(); 
        player->position = v2(0, -5);
    }

    if (player->healths < 0) {
        player->is_valid = false;
        return;
    }

    if (is_key_down(KEY_SPACEBAR)) {
        if ((player->timer + SHOOT_INTERVAL) < now_time) {
            player->timer = now_time;

            size_t count = 3;

            float32 shoot_angle = PI32 / 16.0;
            for (size_t i = 0; i < count; i++) {
                entity_t* bullet = entity_create();

                if (bullet == 0) break;

                entity_setup_player_bullet(bullet);
                bullet->position = player->position;

                float32 normalized = (float32)i / (float32)(count - 1);
                float32 angle = normalized * shoot_angle + (PI32 / 2) - (shoot_angle / 2);

                float32 y = sinf(angle);
                float32 x = cosf(angle);

                bullet->direction = v2(x, y);
            }
        }
    }
}

void update_enemy(entity_t* enemy) {
    if (enemy->healths < 0) {
        enemy->is_valid = false;
        return;
    }

    entity_t* bullet_collision = check_collision_with_relevant_entities(enemy);

    if (bullet_collision != 0) {
        enemy->healths -= 1;
        bullet_collision->is_valid = false; 
        return;
    }

    if ((enemy->timer + SHOOT_INTERVAL) > now_time) return;

    enemy->timer = now_time;

    size_t count = 5;

    // different patterns for different enemies. bosses are separate topic!

    for (size_t i = 0; i <= count; i++) {
        entity_t* bullet = entity_create();
        if (bullet == 0) break;

        entity_setup_bullet_01(bullet);
        bullet->position = enemy->position;

        float32 normalized = ((float32)i / (float32)count);

        float32 y = sinf(normalized * 2.0 * PI32 + 1 * now_time);
        float32 x = cosf(normalized * 2.0 * PI32 + 1 * now_time);

        bullet->direction = v2(x, y);
    }
}

void update_enemies(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* enemy = &(world->entities[i]);
        if (!enemy->is_valid) continue;
        if (enemy->entity_type != ENTITY_enemy) continue;

        update_enemy(enemy);
    }
}

void update_bullets(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);

        if (!bullet->is_valid) continue;
        if (!(bullet->entity_type == ENTITY_bullet)) continue;

        if (!range2f_contains(borders, bullet->position)) {
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

int entry(int argc, char **argv) {
    window.title = STR("Danmaku dungeons");
    window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
    window.scaled_height = 720; 
    window.x = 200;
    window.y = 90;
    window.clear_color = hex_to_rgba(0x0f0f0fff);

    borders = range2f_make(v2(-4, -5), v2(4, 5));

    sprites[SPRITE_error] = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/error.png"), get_heap_allocator()), .size = v2(1, 1) };
    sprites[SPRITE_player] = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/players/player_01.png"), get_heap_allocator()), .size = v2(1, 1) };
    sprites[SPRITE_enemy] = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/enemies/enemy_01.png"), get_heap_allocator()), .size = v2(1, 1) };
    sprites[SPRITE_bullet_01] = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/bullets/bullet_01.png"), get_heap_allocator()), .size = v2(0.2, 0.2) };

    world = alloc(get_heap_allocator(), sizeof(world_t));

    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t *ent = &(world->entities[i]);
        memset(ent, 0, sizeof(entity_t));
    }

    player = entity_create();
    camera = entity_create();
    entity_t* enemy = entity_create();

    entity_setup_player(player);
    entity_setup_camera(camera);
    entity_setup_enemy(enemy);

    player->position = v2(0, 0);
    camera->position = v2(0, 0);
    enemy->position  = v2(0, 0);

    font = load_font_from_disk(STR("res/fonts/jacquard.ttf"), get_heap_allocator());

    circle_t circ = { 0 };
    float64 prew_time = os_get_current_time_in_seconds();
    enemy->timer = prew_time;

    while (!window.should_close) {
        collision_checks = 0;
        reset_temporary_storage();

        now_time = os_get_current_time_in_seconds();
        delta_time = now_time - prew_time;

        if (is_key_just_pressed(KEY_ESCAPE)) window.should_close = true;

        update_bullets();
        update_enemies();
        update_player();

        // ---- rendering ---- //

        reset_draw_frame(&draw_frame);

        float64 scale = max(0.01f, camera->camera_scale);
        draw_frame.view = m4_make_scale(v3(scale, scale, scale));
        draw_frame.view = m4_translate(draw_frame.view, v3(camera->position.x, camera->position.y, 1));

        draw_rect(borders.min, v2_mulf(borders.max, 2.0f), hex_to_rgba(0x7f7f7f3f));

        for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
            entity_t ent = world->entities[i];

            if (!ent.is_valid) continue;
            if (!ent.is_visible) continue;

            Vector4 color = COLOR_WHITE;

            Matrix4 xform = m4_scalar(1.0);
            Vector2 offset = v2_mulf(ent.sprite.size, -0.5f);
            xform = m4_translate(xform, v3(offset.x, offset.y, 0));
            xform = m4_translate(xform, v3(ent.position.x, ent.position.y, 0));

            if (ent.entity_type == ENTITY_bullet) {
                switch (ent.bullet_type) {
                    case BULLET_player_01:
                        color = hex_to_rgba(0xFFE1BF7F);
                        break;
                    default:
                        color = COLOR_WHITE;
                        break;
                }
            }

            draw_image_xform(ent.sprite.image, xform, ent.sprite.size, color);
        }

        draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); // topleft
        
        draw_frame.view = m4_make_scale(v3(1, 1, 1));

        draw_text(font, tprintf("player health\t: %00d", player->healths), 48, v2(0, -30), v2(1, 1), COLOR_WHITE);
        draw_text(font, tprintf("enemy health\t: %00d", enemy->healths), 48, v2(0, -60), v2(1, 1), COLOR_WHITE);
        draw_text(font, tprintf("collision checks\t: %00d", collision_checks), 48, v2(0, -90), v2(1, 1), COLOR_WHITE);
        draw_text(font, tprintf("frametime\t: %0.2f", 1.0 / delta_time), 48, v2(0, -120), v2(1, 1), COLOR_WHITE);

        os_update(); 
        gfx_update();

        prew_time = now_time;
    }

    return 0;
}
