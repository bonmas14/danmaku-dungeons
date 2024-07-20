#define MAX_ENTITY_COUNT 4096
#define CAMERA_SCALE 5.0f
#define CAMERA_SPEED 1.0f
#define PLAYER_SPEED 4.5f
#define PLAYER_BULLET_SPEED 20.0f
#define BULLET_01_SPEED 10.0f
#define SHOOT_INTERVAL 0.1f
#define PLAYER_HEALTHS 3
#define ENEMY_HEALTHS 1 // bosses are like 100 - 1000
#define BULLET_DAMAGE 1

typedef enum entity_archetype_t { // do we need separate archetype for bullets? so we dont need swith (ent.entity_type) ??
    ENTITY_nil,
    ENTITY_enemy,
    ENTITY_player_bullet, 
    ENTITY_bullet_01, 
    ENTITY_bullet_02, 
    ENTITY_player, 
    ENTITY_camera, 
} entity_archetype_t;


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
    bool is_collidable;
    entity_archetype_t entity_type;

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

sprite_t sprites[SPRITE_MAX];

entity_t* player;
entity_t* camera;

float64 now_time = 0;
float64 delta_time = 0;

sprite_t sprite_get(sprite_id_t id) {
    if (id >= SPRITE_MAX || id <= SPRITE_error) return sprites[SPRITE_error];
    if (sprites[id].image == 0) return sprites[SPRITE_error];
    return sprites[id];
}

void entity_setup_player(entity_t* entity) {
    entity->entity_type = ENTITY_player;
    entity->is_visible = true;
    entity->is_collidable = true;
    entity->movement_speed = PLAYER_SPEED;
    entity->sprite = sprite_get(SPRITE_player);
    entity->healths = PLAYER_HEALTHS;
    entity->radius = entity->sprite.size.x / 2 / 10; // THIS IS DANMAKU GAME, player collider should be small!
}

void entity_setup_player_bullet(entity_t* entity) {
    entity->entity_type = ENTITY_player_bullet;
    entity->is_visible = true;
    entity->is_collidable = true;
    entity->movement_speed = PLAYER_BULLET_SPEED;
    entity->sprite = sprite_get(SPRITE_bullet_01);
    entity->radius = entity->sprite.size.x / 2;
}
void entity_setup_bullet_01(entity_t* entity) {
    entity->entity_type = ENTITY_bullet_01;
    entity->is_visible = true;
    entity->is_collidable = true;
    entity->movement_speed = BULLET_01_SPEED;
    entity->sprite = sprite_get(SPRITE_bullet_01);
    entity->radius = entity->sprite.size.x / 2;
}

void entity_setup_enemy(entity_t* entity) {
    entity->entity_type = ENTITY_enemy;
    entity->is_visible = true;
    entity->is_collidable = true;
    entity->movement_speed = PLAYER_SPEED;
    entity->sprite = sprite_get(SPRITE_enemy);
    entity->radius = entity->sprite.size.x / 2;
}

void entity_setup_camera(entity_t* entity) {
    entity->entity_type = ENTITY_camera;
    entity->is_visible = false;
    entity->is_collidable = false;
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

void entity_clear_all_enemy_bullets(void) {
    for (u32 i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);

        if (!bullet->is_valid) continue;

        switch (bullet->entity_type) {
            case ENTITY_bullet_01:
                bullet->is_valid = false;
                break;
            default:
            break;
        }
    }
}

void update_player(void) {
    Vector2 input_axis = v2(0, 0);

    if (is_key_down('W'))
        input_axis.y += 1.0f;
    if (is_key_down('S'))
        input_axis.y -= 1.0f;
    if (is_key_down('A'))
        input_axis.x -= 1.0f;
    if (is_key_down('D'))
        input_axis.x += 1.0f;

    input_axis = v2_normalize(input_axis);

    input_axis = v2_mulf(input_axis, player->movement_speed * delta_time);
    
    player->position = v2_add(player->position, input_axis);

    if (!is_key_down(KEY_SPACEBAR))
        return;

    if ((player->timer + SHOOT_INTERVAL) < now_time) {
        player->timer = now_time;

        size_t count = 3;

        float32 shoot_angle = PI32 / 16.0;
        for (size_t i = 0; i <= count; i++) {
            entity_t* ent = entity_create();

            if (ent == 0) break;

            entity_setup_player_bullet(ent);
            ent->position = player->position;

            float32 normalized = (float32)i / (float32)count;

            float32 angle = normalized * shoot_angle + PI32 / 2 - (shoot_angle / 2);

            float32 y = sinf(angle);
            float32 x = cosf(angle);

            ent->direction = v2(x, y);
        }
    }
}

void update_enemies(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t enemy = world->entities[i];

        if (!enemy.is_valid) continue;


        if (enemy.entity_type != ENTITY_enemy) continue;

        if ((enemy.timer + SHOOT_INTERVAL * 2.0) > now_time)
            continue;

        enemy.timer = now_time;

        size_t count = 10;

        for (size_t i = 0; i <= count; i++) {
            entity_t* bullet = entity_create();
            if (bullet == 0) break;

            entity_setup_bullet_01(bullet);
            bullet->position = enemy.position;
            //bullet->direction = v2_normalize(v2(get_random_float64_in_range(-1, 1), get_random_float64_in_range(-1, 1)));

            float32 normalized = ((float32)i / (float32)count);

            float32 y = sinf(normalized * 2.0 * PI32 + 1 * now_time);
            float32 x = cosf(normalized * 2.0 * PI32 + 1 * now_time);

            bullet->direction = v2(x, y);
        }

        world->entities[i] = enemy;
    }
}

void update_bullets(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);

        if (!bullet->is_valid) continue;

        // each bullet has its unique movement pattern, but not right now as you see

        switch (bullet->entity_type) { 
            case ENTITY_bullet_01:
                if (bullet->timer > 1.0) { // just for tests, it should break as soon as it go over screen
                    bullet->is_valid = false;
                    continue;
                } 

                bullet->position = v2_add(bullet->position, v2_mulf(bullet->direction, bullet->movement_speed * delta_time));
                bullet->timer += delta_time;
                break; 
            case ENTITY_player_bullet:
                if (bullet->timer > 1.0) { // just for tests, it should break as soon as it go over screen
                    bullet->is_valid = false;
                    continue;
                } 

                bullet->position = v2_add(bullet->position, v2_mulf(bullet->direction, bullet->movement_speed * delta_time));
                bullet->timer += delta_time;
                break;
            default: break;
        }

        // collisions
        switch (bullet->entity_type) { 
            case ENTITY_bullet_01:
                if (check_collision_circle_to_circle(circle(player->radius, player->position), circle(bullet->radius, bullet->position))) {
                    player->healths -= BULLET_DAMAGE;
                    bullet->is_valid = false; // play cool animation and delete all bullets instead
    
                    entity_clear_all_enemy_bullets();
                    return;
                }
                break;
            case ENTITY_player_bullet:
                break;
                // if we want attack enemies, we need to scan all enemies and bullets N^2
                // that would be bad so we need to separate bullets from entities.
                // it will make live easier
                //
                // if (check_collision_circle_to_circle(circle(player->radius, player->position), circle(bullet->radius, bullet->position))) { }
            default: break;
        } 
    }
}

int entry(int argc, char **argv) {
	window.title = STR("Danmaku dungeons");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	//window.clear_color = hex_to_rgba(0x6495EDff);
	window.clear_color = hex_to_rgba(0x0f0f0fff);

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
	enemy->position  = v2(2, 2);

    font = load_font_from_disk(STR("res/fonts/jacquard.ttf"), get_heap_allocator());

    circle_t circ = { 0 };
	float64 prew_time = os_get_current_time_in_seconds();

	while (!window.should_close) {
		reset_temporary_storage();

		now_time = os_get_current_time_in_seconds();
		delta_time = now_time - prew_time;

		if (is_key_just_pressed(KEY_ESCAPE)) window.should_close = true;
    
        update_bullets();
        update_player();
        update_enemies();

        // ---- rendering ---- //

        reset_draw_frame(&draw_frame);

        float64 scale = max(0.01f, camera->camera_scale);
        draw_frame.view = m4_make_scale(v3(scale, scale, scale));
		draw_frame.view = m4_translate(draw_frame.view, v3(camera->position.x, camera->position.y, 1));

        for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
            entity_t ent = world->entities[i];

            if (!ent.is_valid) continue;
            if (!ent.is_visible) continue;

            Vector4 color = COLOR_WHITE;

            Matrix4 xform = m4_scalar(1.0);
            Vector2 offset = v2_mulf(ent.sprite.size, -0.5f);
            xform = m4_translate(xform, v3(offset.x, offset.y, 0));
            xform = m4_translate(xform, v3(ent.position.x, ent.position.y, 0));

            switch (ent.entity_type) {
                case ENTITY_player_bullet:
                    color = hex_to_rgba(0xFFE1BF7F);
                    break;
                default:
                    color = COLOR_WHITE;
                    break;
            }

            draw_image_xform(ent.sprite.image, xform, ent.sprite.size, color);
        }

        draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); // topleft
        draw_frame.view = m4_make_scale(v3(1, 1, 1));

        draw_text(font, tprintf("healths: %d", player->healths), 48, v2(0, -24), v2(1, 1), COLOR_WHITE);

        os_update(); 
		gfx_update();

		prew_time = now_time;
	}

	return 0;
}
