#define MAX_ENTITY_COUNT 4096
#define MAX_FRAME_COUNT (1 << 16)
#define CAMERA_SCALE 7.5f
#define PLAYER_SPEED 4.5f
#define MAX_DIST_TO_PLAYER 20.0f
#define PLAYER_BULLET_SPEED 20.0f
#define BULLET_01_SPEED 2.0f
#define SHOOT_INTERVAL 0.1f
#define FLOW_UPDATE_INTERVAL 0.1f
#define PLAYER_HEALTHS 3
#define MAP_WIDTH 100
#define MAP_HEIGHT 100

#define MAX_FLOW_DIST 10

#define MONEY_MOVE_SPEED 10.0f
// bosses are like 100 - 1000
#define ENEMY_HEALTHS 20

#include "game_structures.c"
#include "globals.c"
#include "world_generation.c"

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

void entity_setup_item(entity_t* entity) {
    entity->entity_type = ENTITY_item;
}

void item_setup_gold(entity_t* entity) {
    entity->item_type = ITEM_gold;
    entity->sprite = sprite_get(SPRITE_gold);
    entity->radius = entity->sprite.size.x / 2;
}
// @cleanup add function that converts world space
// to map space. instead of all this get_flow get_block
flow_t get_flow_by_world_coord(Vector2 position) {
    position = v2_add(position, v2(MAP_WIDTH / 2, MAP_HEIGHT / 2)); 
    
    position = v2(clamp(position.x, 0, (f32)MAP_WIDTH), clamp(position.y, 0, (f32)MAP_HEIGHT));

    s32 x = (s32)position.x;
    s32 y = (s32)position.y;

    return world->flow_map[x + y * MAP_WIDTH];
}

block_t get_block_by_world_coord(Vector2 position) {
    // @cleanup change this to something that related to current map. like gen_conf struct
    position = v2_add(position, v2(MAP_WIDTH / 2, MAP_HEIGHT / 2)); 
    
    position = v2(clamp(position.x, 0, (f32)MAP_WIDTH), clamp(position.y, 0, (f32)MAP_HEIGHT));

    // @cleanup Vector2i will be here

    s32 x = (s32)position.x;
    s32 y = (s32)position.y;

    return world->world_map[x + y * MAP_WIDTH];
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

void scan_world_and_spawn_entitites(void) {
    for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
        int32_t x = i % MAP_WIDTH;
        int32_t y = i / MAP_WIDTH;

        switch (world->world_map[i].prototype.entity_type) {
            case ENTITY_player:
                player_entity = entity_create();
                entity_setup_player(player_entity);
                player_entity->position = v2(((f32)x - MAP_WIDTH / 2.0) + 0.5, ((f32)y - MAP_HEIGHT / 2.0) + 0.5);
                respawn_point = player_entity->position;
                break;
            case ENTITY_enemy:
                entity_t* enemy = entity_create();
                entity_setup_enemy(enemy);
                enemy->position = v2(((f32)x - MAP_WIDTH / 2.0) + 0.5, ((f32)y - MAP_HEIGHT / 2.0) + 0.5);
                enemy->timer = now_time;
                break;
            default:
                break;
        }
    }
}

bool check_collision_with_bullet(entity_t* bullet, entity_t* entity, circle_t bullet_circle, circle_t entity_circle) {
    switch (bullet->bullet_type) {
        case BULLET_player_01:
            if (check_collision_circle_to_circle(bullet_circle, entity_circle))
                return entity->entity_type == ENTITY_enemy;
            break;
        case BULLET_enemy_01:
            if (check_collision_circle_to_circle(bullet_circle, entity_circle))
                return entity->entity_type == ENTITY_player;
            break;
    }

    return false;
}

entity_t* check_collision_with_relevant_entities(entity_t* entity) {

    for (u32 i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* collide_with = &(world->entities[i]);

        if (!collide_with->is_valid) continue;


        if (collide_with->entity_type == ENTITY_bullet) {
            circle_t entity_circle = circle(entity->radius, entity->position);
            circle_t bullet_circle = circle(collide_with->radius, collide_with->position);
            if (check_collision_with_bullet(collide_with, entity, bullet_circle, entity_circle))
                return collide_with;
        }

        if (collide_with->entity_type == ENTITY_item) {
            if (entity->entity_type != ENTITY_player) continue;

            circle_t entity_circle = circle(entity->radius * 10.0f, entity->position);
            circle_t item_circle   = circle(collide_with->radius, collide_with->position);

            if (check_collision_circle_to_circle(item_circle, entity_circle))
                return collide_with;
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
            bullet->shoot_by = player;

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
        
        program_state = GAME_menu;
        return;
    }

    if (is_key_down(KEY_SPACEBAR)) {
        player_shoot(player_entity);
    }

    Vector2 multiplier = v2(1, 1);
    if (is_key_down(KEY_CTRL)) multiplier = v2(1.75f, 1.75f);
    if (is_key_down(KEY_SHIFT)) multiplier = v2(0.35f, 0.35f);

    Vector2 input_axis = v2(0, 0);
    if (is_key_down('W')) input_axis.y += 1.0f;
    if (is_key_down('S')) input_axis.y -= 1.0f;
    if (is_key_down('A')) input_axis.x -= 1.0f;
    if (is_key_down('D')) input_axis.x += 1.0f;
    
    if (get_block_by_world_coord(v2(player_entity->position.x + input_axis.x / 10, player_entity->position.y)).type == BLOCK_wall)
        multiplier.x = 0;
    if (get_block_by_world_coord(v2(player_entity->position.x, player_entity->position.y + input_axis.y / 10)).type == BLOCK_wall)
        multiplier.y = 0;

    input_axis = v2_normalize(input_axis);
    input_axis = v2_mulf(v2_mul(input_axis, multiplier), delta_time * player_entity->movement_speed);
    
    player_entity->position = v2_add(player_entity->position, input_axis);

    entity_t* collision = check_collision_with_relevant_entities(player_entity);

    if (collision == 0) 
        return;
    switch (collision->entity_type) {
        case ENTITY_bullet:
            player_entity->healths -= 1;

            entity_clear_all_bullets(); 
            player_entity->position = respawn_point;
            break;
        case ENTITY_item:
            // add item, play sound
            { // audio_player_is_finished 
                spinlock_acquire_or_wait(&impact_player->sample_lock);
                assert(impact_player->frame_index <= impact_player->source.number_of_frames);

                if (impact_player->frame_index == impact_player->source.number_of_frames)
                    impact_player->frame_index = 0;

                spinlock_release(&impact_player->sample_lock);

            }
            collision->is_valid = 0;
            break;

        default:
            break;
    }
}

flow_output_t get_max_value_from_neighbour(s32 xi, s32 yi) {
    flow_output_t output = { 0, 0 };

    for (s32 yo = -1; yo < 2; yo++) {
        for (s32 xo = -1; xo < 2; xo++) {
            s32 x = xo + xi;
            s32 y = yo + yi;

            if (yo == 0 && xo == 0) continue;
            if ((yo != 0) && (xo != 0)) continue;

            if (!(x >= 0 && x < MAP_WIDTH)) continue;
            if (!(y >= 0 && y < MAP_HEIGHT)) continue;

            if (world->flow_map[x + y * MAP_WIDTH].approach > output.approach)
                output.approach = world->flow_map[x + y * MAP_WIDTH].approach;

            if (world->flow_map[x + y * MAP_WIDTH].danger > output.danger)
                output.danger   = world->flow_map[x + y * MAP_WIDTH].danger; 
        }
    }

    return output;
}

void update_flow_tile(s32 i) {
    flow_t current_tile = world->flow_map[i];
    s32 x = i % MAP_WIDTH;
    s32 y = i / MAP_WIDTH;

    flow_output_t value = get_max_value_from_neighbour(x, y);

    if (current_tile.approach > 0 || current_tile.approach < 0) {
        world->temp_map[i].approach = current_tile.approach;
    } else if (value.approach != 0) {
        world->temp_map[i].approach = value.approach - 1;
    }

    if (current_tile.danger > 0 || current_tile.danger < 0) {
        world->temp_map[i].danger = current_tile.danger;
    } else if (value.danger != 0) {
        world->temp_map[i].danger = value.danger - 1;
    }
}

void update_flow_map(void) {
    // clear 
    // get position of player
    // propogate flow map
    // profit
    
    // clear
    // @cleanup change this to something that related to current map. like gen_conf struct
    for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
        world->flow_map[i] = (flow_t) { 0 };
    }

    // adding player and enemies
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* ent = &(world->entities[i]);
        if (!ent->is_valid) continue;

        Vector2 position = v2_add(ent->position, v2(MAP_WIDTH / 2, MAP_HEIGHT / 2)); 

        position = v2(clamp(position.x, 0, (f32)MAP_WIDTH), clamp(position.y, 0, (f32)MAP_HEIGHT));

        s32 index = ((s32)position.x) + ((s32)position.y) * MAP_WIDTH;

        switch (ent->entity_type) {
            case ENTITY_player: world->flow_map[index].approach = MAX_FLOW_DIST; break;
            case ENTITY_enemy:  world->flow_map[index].danger   = MAX_FLOW_DIST; break;
            default: break;
        }

        if (world->world_map[index].type == BLOCK_wall) {
            world->flow_map[index].approach = -1;
        }
    }

    for (size_t steps = 0; steps < MAX_FLOW_DIST; steps++) {
        for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++)
            if (world->world_map[i].type == BLOCK_floor)
                update_flow_tile(i);

        for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
            world->flow_map[i] = world->temp_map[i];
            world->temp_map[i] = (flow_t){ 0 };
        }
    }
}

void update_enemy_state(entity_t* enemy) {
    if (enemy->healths < 0) {
        return;
    }

    // movement
    {
        // based on timer
        // we need to add Vector2 dest to a enemy. 
        // And 
    }


    entity_t* bullet_collision = check_collision_with_relevant_entities(enemy);

    if (bullet_collision == 0) return;

    { // audio_player_is_finished 
        spinlock_acquire_or_wait(&impact_player->sample_lock);
        assert(impact_player->frame_index <= impact_player->source.number_of_frames);

        if (impact_player->frame_index == impact_player->source.number_of_frames)
            impact_player->frame_index = 0;

        spinlock_release(&impact_player->sample_lock);
    }

    enemy->healths -= 1;
    bullet_collision->is_valid = false; 
}

void update_enemies(void) {
    if ((flow_update_timer + FLOW_UPDATE_INTERVAL) <= now_time) {
        flow_update_timer = now_time;
        update_flow_map();
    }

    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* enemy = &(world->entities[i]);
        if (!enemy->is_valid) continue;
        if (enemy->entity_type != ENTITY_enemy) continue;

        update_enemy_state(enemy);

        if (enemy->healths < 0) {
            size_t count = get_random_int_in_range(2, 5);

            for (size_t i = 0; i < count; i++) {
                entity_t* gold = entity_create();
                if (gold == 0) break;

                entity_setup_item(gold);
                item_setup_gold(gold);

                float32 x = get_random_float32_in_range(-0.5, 0.5);
                float32 y = get_random_float32_in_range(-0.5, 0.5);

                gold->position = v2_add(enemy->position, v2(x, y));
            }

            enemy->is_valid = false;
            continue; 
        }

        if ((enemy->timer + SHOOT_INTERVAL * 8) < now_time) {
            if (v2_sqr_dist(player_entity->position, enemy->position) > (MAX_DIST_TO_PLAYER * MAX_DIST_TO_PLAYER))
                continue;

            enemy->timer = now_time;

            size_t count = get_random_int_in_range(2, 4);

            Vector2 position = v2_sub(enemy->position, player_entity->position);

            float32 shoot_angle = PI32 / 4.0;  

            for (size_t i = 0; i < count; i++) {
                entity_t* bullet = entity_create();

                if (bullet == 0) break;

                entity_setup_bullet_01(bullet);
                bullet->position = enemy->position;
                bullet->shoot_by = enemy;

                float32 normalized = (float32)i / (float32)(count - 1);
                float32 angle = normalized * shoot_angle 
                              + (PI32 / 2) - (shoot_angle / 2)
                              + atan2f(position.y, position.x)
                              + PI32 / 2;

                float32 y = sinf(angle);
                float32 x = cosf(angle);

                bullet->direction = v2(x, y);
            }
        }
        // move & shoot here!
    }
}

// @feature bullets destroys when enemy dies
void update_bullets(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);

        if (!bullet->is_valid) continue;
        if (!(bullet->entity_type == ENTITY_bullet)) continue;
        
        if (bullet->timer > 10.0 || !bullet->shoot_by->is_valid) {
            bullet->is_valid = false;
            continue;
        }

        // each bullet has its unique movement pattern, but not right now as you see
        switch (bullet->bullet_type) { 
            case BULLET_enemy_01:
            case BULLET_player_01:
                Vector2 position = v2_add(bullet->position, v2_mulf(bullet->direction, bullet->movement_speed * delta_time));

                if (get_block_by_world_coord(position).type == BLOCK_wall) {
                    bullet->is_valid = false;
                } else {
                    bullet->position = position;
                    bullet->timer += delta_time;
                }

                break;
            default: 
                break;
        }
    }
}

void update_items(void) { 
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* item = &(world->entities[i]);

        if (!item->is_valid) continue;
        if (!(item->entity_type == ENTITY_item)) continue;

        Vector2 position = v2_sub(item->position, player_entity->position);

        if (v2_magn_greater_than(position, MAX_DIST_TO_PLAYER / 5))
            continue;

        f32 angle = atan2f(position.y, position.x);

        item->position.x -= cosf(angle) * MONEY_MOVE_SPEED * delta_time;
        item->position.y -= sinf(angle) * MONEY_MOVE_SPEED * delta_time;
    }
}

// @cleanup change MAP_WIDTH MAP_HEIGHT this to something that related to current map. like gen_conf struct
void draw_world_map(bool optimize) {
    for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
        int32_t x = i % MAP_WIDTH;
        int32_t y = i / MAP_WIDTH;

        Vector2 position = v2_sub(v2(x, y), v2(MAP_WIDTH / 2, MAP_HEIGHT / 2));

        if (optimize) {
            if (v2_dist_to_greater_than(player_entity->position, position, MAX_DIST_TO_PLAYER)) {
                continue;
            }
        }

        Vector4 color = hex_to_rgba(0x3f3f3fff);

        //color.g = (f32)world->flow_map[i].approach / (f32)MAX_FLOW_DIST;
        //color.r = (f32)world->flow_map[i].danger / (f32)MAX_FLOW_DIST;

        color.r = ((f32)world->flow_map[i].approach * (f32)world->flow_map[i].danger) / ((f32)MAX_FLOW_DIST * (f32)MAX_FLOW_DIST) / 2.0;
        color.g = ((f32)world->flow_map[i].approach * (f32)world->flow_map[i].danger) / ((f32)MAX_FLOW_DIST * (f32)MAX_FLOW_DIST) / 2.0;
        color.b = ((f32)world->flow_map[i].approach * (f32)world->flow_map[i].danger) / ((f32)MAX_FLOW_DIST * (f32)MAX_FLOW_DIST) / 2.0;

        switch (world->world_map[i].type) {
            case BLOCK_floor: 
                draw_rect(position, v2(1, 1), color);
                break;
            case BLOCK_wall: 
                draw_rect(position, v2(1, 1), hex_to_rgba(0x7f7f7fff));
                break;
            default:
                break;
        }
    
        // @debug
        //if (world->flow_map[i].approach > 0)
        //    draw_text(font, tprintf("%d", world->flow_map[i].approach), 30, position, v2(0.03, 0.03), COLOR_WHITE);
    }
}

void draw_entities(bool optimize) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t ent = world->entities[i];

        if (!ent.is_valid) continue;

        if (optimize) {
            if (v2_dist_to_greater_than(player_entity->position, ent.position, MAX_DIST_TO_PLAYER)) {
                continue;
            }
        }

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

}

void update_menu(void) {
    reset_draw_frame(&draw_frame);

    if (is_key_just_pressed(KEY_ESCAPE))
        window.should_close = true;

    if (is_key_just_pressed(KEY_SPACEBAR)) {
        entity_reset_all();
        generate_map();
        scan_world_and_spawn_entitites();
        camera.scale = CAMERA_SCALE;
        program_state = GAME_scene;
    }

 
    float64 scale = camera.scale;

    draw_frame.view = m4_make_translation(v3(camera.position.x, camera.position.y, 0));
    draw_frame.view = m4_scale(draw_frame.view, v3(scale, scale, scale));
    
    draw_world_map(false);
    draw_entities(false);

    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); // topleft
    draw_frame.view = m4_make_scale(v3(1, 1, 1));

    draw_text(font, STR("You're in game menu. \nPress SPACE to start\nPress ESC to exit"), 30, v2(0, -window.pixel_height / 2), v2(1, 1), COLOR_WHITE);
    draw_text(font, tprintf("fps\t: %0.2f", 1.0 / delta_time), 30, v2(0, -60), v2(1, 1), COLOR_WHITE);
}
void update_editor(void) {
    reset_draw_frame(&draw_frame);

    float32 multiplier = 1.0f;
    if (is_key_down(KEY_SHIFT)) multiplier = 5.0f;

    Vector2 input_axis = v2(0, 0);
    if (is_key_down('W')) input_axis.y += 1.0f;
    if (is_key_down('S')) input_axis.y -= 1.0f;
    if (is_key_down('A')) input_axis.x -= 1.0f;
    if (is_key_down('D')) input_axis.x += 1.0f;

    input_axis = v2_normalize(input_axis);
    input_axis = v2_mulf(input_axis, multiplier * delta_time * PLAYER_SPEED);

	for (u64 i = 0; i < input_frame.number_of_events; i++) {
		Input_Event e = input_frame.events[i];
		
		switch (e.kind) {
			case INPUT_EVENT_SCROLL:
                camera.scale += -e.yscroll * delta_time * 200.0;
                break;
            default:
                break;
		}
	}

    camera.position = v2_add(camera.position, input_axis);

            
    if (is_key_down(KEY_SHIFT)) {
        if (is_key_down(KEY_F8)) {
            entity_reset_all();
            generate_map();
            scan_world_and_spawn_entitites();
        }
    } else {
        if (is_key_just_pressed(KEY_F8)) {
            entity_reset_all();
            generate_map();
            scan_world_and_spawn_entitites();
        }
    }

    if (is_key_just_pressed(KEY_F7)) {
        camera.scale = CAMERA_SCALE;
        program_state = GAME_scene;
    }
 
    float64 scale = camera.scale;

    draw_frame.view = m4_make_translation(v3(camera.position.x, camera.position.y, 0));
    draw_frame.view = m4_scale(draw_frame.view, v3(scale, scale, scale));

    Vector2 mouse_wp = screen_to_world(v2(input_frame.mouse_x, input_frame.mouse_y), v2(window.width, window.height), draw_frame);
    // placing / deleting block
    
    int32_t x = (int32_t)roundf(mouse_wp.x - 0.5f) + MAP_WIDTH / 2;
    int32_t y = (int32_t)roundf(mouse_wp.y - 0.5f) + MAP_HEIGHT / 2;

    if ((x >= 0 && x < MAP_WIDTH) && (y >= 0 && y < MAP_HEIGHT)) {
        if (is_key_down(MOUSE_BUTTON_LEFT)) {
            world->world_map[x + y * MAP_WIDTH].type = BLOCK_wall;
        }
        if (is_key_down(MOUSE_BUTTON_RIGHT)) {
            world->world_map[x + y * MAP_WIDTH].type = BLOCK_floor;
        }
    }

    draw_world_map(false);
    draw_entities(false);

    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); // topleft
    draw_frame.view = m4_make_scale(v3(1, 1, 1));

    draw_text(font, STR("editor. f8 regenerate map. f7 exit editor"), 30, v2(0, -30), v2(1, 1), COLOR_WHITE);
    draw_text(font, tprintf("fps\t: %0.2f", 1.0 / delta_time), 30, v2(0, -60), v2(1, 1), COLOR_WHITE);
}

// @gameupdate
void update_game_scene(void) {
    reset_draw_frame(&draw_frame);

    Vector2 target_pos = player_entity->position;
    animate_v2_to_target(&camera.position, target_pos, delta_time, 30.0f);

    float64 scale = camera.scale;
    draw_frame.view = m4_make_translation(v3(camera.position.x, camera.position.y, 0));
    draw_frame.view = m4_scale(draw_frame.view, v3(scale, scale, scale));

    update_items();
    update_bullets();
    update_enemies();
    update_player();

    if (is_key_just_pressed(KEY_ESCAPE)) {
        program_state = GAME_menu;
    }
    if (is_key_just_pressed(KEY_F7)) {
        program_state = GAME_editor;
    }

    draw_world_map(true);
    draw_entities(true);

    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, -window.pixel_height, 0, -1, 10); // topleft
    draw_frame.view = m4_make_scale(v3(1, 1, 1));

    int32_t offset_counter = 2;
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t ent = world->entities[i];

        if (!ent.is_valid) continue;

        if (ent.entity_type == ENTITY_player) {
            draw_text(font, tprintf("player health\t: %00d", ent.healths), 30, v2(0, -30 * offset_counter++), v2(1, 1), COLOR_WHITE);
        }

        //if (ent.entity_type == ENTITY_enemy) {
        //    draw_text(font, tprintf("enemy health\t: %00d", ent.healths), 30, v2(0, -30 * offset_counter++), v2(1, 1), COLOR_WHITE);
        //}

        if (offset_counter > 5) {
            break;
        }
    }

    draw_text(font, tprintf("fps\t: %0.2f", 1.0 / delta_time), 30, v2(0, -30), v2(1, 1), COLOR_WHITE);
}

void update_loading_screen(void) {
    reset_draw_frame(&draw_frame);
    draw_frame.projection = m4_make_orthographic_projection(0, window.pixel_width, 0, window.pixel_height, -1, 10); 
    draw_frame.view = m4_make_scale(v3(1, 1, 1));
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

// @resources
void game_late_init(void) {
    world = alloc(get_heap_allocator(), sizeof(world_t));

    //font = load_font_from_disk(STR("res/fonts/jacquard.ttf"), get_heap_allocator());
    font = load_font_from_disk(STR("res/fonts/arial.ttf"), get_heap_allocator());

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
    sprites[SPRITE_gold]      = (sprite_t) {.image = load_image_from_disk(STR("res/graphics/items/gold.png"),        get_heap_allocator()), .size = v2(0.2, 0.2) };
    
    generator_init();
}

int entry(int argc, char **argv) {
    seed_for_random = rdtsc();
    game_early_init();

    float64 prew_time = os_get_current_time_in_seconds();

    while (!window.should_close) {
        reset_temporary_storage();

        now_time = os_get_current_time_in_seconds();
        delta_time = now_time - prew_time;

        switch (program_state) {
            case GAME_init: 
                update_loading_screen(); 
                break;
            case GAME_menu:
                update_menu();
                break;
            case GAME_editor: 
                update_editor();
                break;
            case GAME_scene: 
                update_game_scene();
                break;
        }

        os_update(); 
        gfx_update();

        if (program_state == GAME_init) { 
            game_late_init(); 
            program_state = GAME_menu;
        } 

        prew_time = now_time;
    }

    return 0;
}
