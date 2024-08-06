#define MAX_ENTITY_COUNT 4096
#define MAX_FRAME_COUNT (1 << 16)
#define CAMERA_SCALE 7.5f
#define PLAYER_SPEED 4.5f
#define ENEMY_SPEED 0.25f
#define MAX_DIST_TO_PLAYER 18.0f
#define MAX_GRAB_DISTANCE 5.0f
#define PLAYER_BULLET_SPEED 20.0f
#define BULLET_01_SPEED 2.0f
#define SHOOT_INTERVAL 0.1f
#define FLOW_UPDATE_INTERVAL 0.1f
#define PLAYER_HEALTHS 3
#define MAP_WIDTH 100
#define MAP_HEIGHT 100

#define MAX_FLOW_DIST 15

#define MONEY_MOVE_SPEED 10.0f
// bosses are like 100 - 1000
#define ENEMY_HEALTHS 20


#define FPS_METER_OUTPUT_ID 64

// MACRO POWER
#define TILE_SIZE 18.0
#define TILEMAP_SIZE 4.0
#define TILEMAP_PIXEL_SIZE v2(1.0 / (TILE_SIZE * TILEMAP_SIZE), 1.0 / (TILE_SIZE * TILEMAP_SIZE))

#define TILE_START(x, y) v2_expand(v2_add(v2((x / TILEMAP_SIZE), y / TILEMAP_SIZE), TILEMAP_PIXEL_SIZE))
#define TILE_STOP(x, y)  v2_expand(v2_sub(v2((x + 1.0) / TILEMAP_SIZE, (y + 1.0) / TILEMAP_SIZE), TILEMAP_PIXEL_SIZE))

#define TILE(x, y) v4(TILE_START(x, y), TILE_STOP(x, y))

#define TILE_WALL_TL TILE(0.0, 3.0)
#define TILE_WALL_TC TILE(1.0, 3.0)
#define TILE_WALL_TR TILE(2.0, 3.0)

#define TILE_WALL_CL TILE(0.0, 2.0)
#define TILE_WALL_CR TILE(2.0, 2.0)

#define TILE_WALL_BL TILE(0.0, 1.0)
#define TILE_WALL_BC TILE(1.0, 1.0)
#define TILE_WALL_BR TILE(2.0, 1.0)

#define TILE_FLOOR TILE(3.0, 3.0)
#define TILE_VOID  TILE(1.0, 2.0)

#define TILE_VOID_CORNER_TL TILE(0.0, 0.0)
#define TILE_VOID_CORNER_TR TILE(1.0, 0.0)
#define TILE_VOID_CORNER_BL TILE(2.0, 0.0)
#define TILE_VOID_CORNER_BR TILE(3.0, 0.0)

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

void clear_destroyed_entities(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* ent = &(world->entities[i]);

        if (ent->is_destroyed) 
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
    size_t count = 3; // defined by player power up
                      
    if ((shoot_audio_timer + SHOOT_INTERVAL / 3) < now_time) {
        shoot_audio_timer = now_time;

        shoot_player->config.playback_speed = get_random_float64_in_range(0.80, 1.1);
        audio_player_set_progression_factor(shoot_player, 0);
    }
                      
    if ((player->timer + SHOOT_INTERVAL) < now_time) {
        player->timer = now_time;

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
        player_entity->is_destroyed = true;
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
            collect_player->config.playback_speed = get_random_float64_in_range(0.95, 1.05);
            audio_player_set_progression_factor(collect_player, 0);

            collision->is_destroyed = true;
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


    // @todo optimize this cellular automata
    for (size_t steps = 0; steps < MAX_FLOW_DIST; steps++) {
        for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
            Vector2i pos = v2i(i % MAP_WIDTH, i / MAP_WIDTH);

            //if (v2_sqr_dist(player_entity->position, v2_sub(v2i_to_v2(pos), v2(MAP_WIDTH / 2, MAP_HEIGHT / 2))) > (MAX_DIST_TO_PLAYER * MAX_DIST_TO_PLAYER))
            //    continue;

            if (world->world_map[i].type == BLOCK_floor)
                update_flow_tile(i);
        }

        for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
            world->flow_map[i] = world->temp_map[i];
            world->temp_map[i] = (flow_t){ 0 };
        }
    }
}

void update_enemy_shoot(entity_t* enemy) {
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
}

void kill_enemy(entity_t* enemy) {
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

    enemy->is_destroyed = true;
}

void update_enemy_state(entity_t* enemy) {
    if (enemy->healths < 0) {
        return;
    }

    // movement
    {
        enemy->destination = player_entity->position;
        // - Check state
        // > - Set destination
        // > - Set attack / flee / shoot state

    }


}

void update_enemies(void) {
    bool update = false;
    if ((flow_update_timer + FLOW_UPDATE_INTERVAL) <= now_time) {
        flow_update_timer = now_time;
        update = true;
        update_flow_map();
    }

    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* enemy = &(world->entities[i]);
        if (!enemy->is_valid) continue;
        if (enemy->entity_type != ENTITY_enemy) continue;

        if (v2_sqr_dist(player_entity->position, enemy->position) > (MAX_DIST_TO_PLAYER * MAX_DIST_TO_PLAYER ))
            continue;
        
        if (update) {
            update_enemy_state(enemy);
        }
        // @cleanup @bug known issue that this func returns only one relevant collision
        // if it doesnt update on every frame or frames are too long, you will get more
        // pronounced bugs, that bullets come through enemy
        entity_t *bullet_collision = check_collision_with_relevant_entities(enemy);

        if (bullet_collision != 0) {
            impact_player->config.playback_speed = get_random_float64_in_range(0.85, 1.15);
            audio_player_set_progression_factor(impact_player, 0);

            enemy->healths -= 1;
            bullet_collision->is_valid = false;

            if (enemy->healths < 0) {
                kill_enemy(enemy);
                continue;
            }
        }

        enemy->direction = v2_normalize(v2_sub(enemy->destination, enemy->position));

        if (get_block_by_world_coord(v2(enemy->position.x + enemy->direction.x / 2, enemy->position.y)).type == BLOCK_wall)
            enemy->direction.x = 0;
        if (get_block_by_world_coord(v2(enemy->position.x, enemy->position.y + enemy->direction.y / 2)).type == BLOCK_wall)
            enemy->direction.y = 0;

        enemy->position = v2_add(enemy->position, v2_mulf(enemy->direction, ENEMY_SPEED * delta_time));

        update_enemy_shoot(enemy);
    }
}

// @feature bullets destroys when enemy dies
void update_bullets(void) {
    for (size_t i = 0; i < MAX_ENTITY_COUNT; i++) {
        entity_t* bullet = &(world->entities[i]);

        if (!bullet->is_valid) continue;
        if (!(bullet->entity_type == ENTITY_bullet)) continue;
        
        if (bullet->timer > 10.0 ) {
            bullet->is_destroyed = true;
            continue;
        }
        if (bullet->shoot_by->is_destroyed) {
            entity_setup_item(bullet);
            item_setup_gold(bullet);
        }

        // each bullet has its unique movement pattern, but not right now as you see
        switch (bullet->bullet_type) { 
            case BULLET_enemy_01:
            case BULLET_player_01:
                Vector2 position = v2_mulf(bullet->direction, bullet->movement_speed * delta_time);
                position = v2_add(position, bullet->position);

                // @todo 
                // we need to check direction of block after, 
                // and change collider a bit if it is pointing upwards. 
                // this is because our upwards blocks are not full, 
                // and bullet can and should move throught them
                if (get_block_by_world_coord(position).type == BLOCK_wall) {
                    bullet->is_destroyed = true;
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

        if (v2_magn_greater_than(position, MAX_GRAB_DISTANCE))
            continue;

        f32 angle = atan2f(position.y, position.x);

        item->position.x -= cosf(angle) * MONEY_MOVE_SPEED * delta_time;
        item->position.y -= sinf(angle) * MONEY_MOVE_SPEED * delta_time;
    }
}

Vector4 get_wallvoid_uv_with_direciton(block_direction_t direction) {
    switch (direction) {
        case DIRECTION_up_left:
            return TILE_VOID_CORNER_TL;
        case DIRECTION_up_right:
            return TILE_VOID_CORNER_TR;
        case DIRECTION_down_left:
            return TILE_VOID_CORNER_BL;
        case DIRECTION_down_right:
            return TILE_VOID_CORNER_BR;

        case DIRECTION_none:
        case DIRECTION_center:
        default:
            return TILE_VOID;
    }
}
Vector4 get_wall_uv_with_direction(block_direction_t direction) {
    switch (direction) {
        case DIRECTION_left:
            return TILE_WALL_CL;
        case DIRECTION_right:
            return TILE_WALL_CR;
        case DIRECTION_up:
            return TILE_WALL_TC;
        case DIRECTION_down:
            return TILE_WALL_BC;

        case DIRECTION_up_left:
            return TILE_WALL_TL;
        case DIRECTION_up_right:
            return TILE_WALL_TR;
        case DIRECTION_down_left:
            return TILE_WALL_BL;
        case DIRECTION_down_right:
            return TILE_WALL_BR;

        case DIRECTION_none:
        case DIRECTION_center:
        default:
            return TILE_VOID;
    }
}

Vector4 get_block_uv_with_direction(size_t index) {
    size_t xi = index % MAP_WIDTH;
    size_t yi = index / MAP_WIDTH;

    s32 left = 0, right = 0, top = 0, down = 0;

    block_direction_t dir = DIRECTION_none;

    // find direction where theres no wall but not void
    //
    // we have 4 directions for example.
    // we have 4 vars that increment if theres floor on this cell
    // we set direction to be the highest var.
    // if one of two are equal it means that we have subdirections
    for (s32 y = -1; y <= 1; y++) {
        for (s32 x = -1; x <= 1; x++) {
            s32 xo = x + xi;
            s32 yo = y + yi;
            
            if (xo < 0 || yo < 0 || xo >= MAP_WIDTH || yo >= MAP_HEIGHT) continue;

            if (world->world_map[xo + yo * MAP_WIDTH].type != BLOCK_floor) continue;

            if (y == -1) top++; else if (y == 1) down++;
            if (x == -1) left++; else if (x == 1) right++;
        }
    }
    bool corner_wall = false;
    // kinda works but be need to process all DIRECTION_center after, to make proper "VOID" tiles
    if ((left - right) > 1) {
        if ((top - down) < -1)     dir = DIRECTION_up_left;
        else if ((top - down) > 1) dir = DIRECTION_down_left;
        else                       dir = DIRECTION_left;
    } else if ((left - right) < -1) {
        if ((top - down) < -1)     dir = DIRECTION_up_right;
        else if ((top - down) > 1) dir = DIRECTION_down_right;
        else                       dir = DIRECTION_right;
    } else {
        if ((top - down) < -1)     dir = DIRECTION_up;
        else if ((top - down) > 1) dir = DIRECTION_down;
        else                       dir = DIRECTION_center;
    }

    if (dir == DIRECTION_center) {
        corner_wall = true;
        if (left < right) {
            if (top > down)      dir = DIRECTION_up_left;
            else if (top < down) dir = DIRECTION_down_left;
            else                 dir = DIRECTION_left;
        } else if (left > right) {
            if (top > down)      dir = DIRECTION_up_right;
            else if (top < down) dir = DIRECTION_down_right;
            else                 dir = DIRECTION_right;
        } else {
            if (top > down)      dir = DIRECTION_up;
            else if (top < down) dir = DIRECTION_down;
            else                 dir = DIRECTION_center;
        }
    }

    switch (world->world_map[index].type) {
        case BLOCK_wall: 
            if (corner_wall) 
                return get_wallvoid_uv_with_direciton(dir);
            return get_wall_uv_with_direction(dir);
        case BLOCK_floor: 
            return TILE_FLOOR;
        case BLOCK_empty: 
            return TILE_VOID;
        default:
            return TILE_VOID;
    }

    return TILE_VOID;
}

// @cleanup change MAP_WIDTH MAP_HEIGHT this to something that related to current map. like gen_conf struct
void draw_world_map(bool optimize) {
    Vector2 pixel_size = TILEMAP_PIXEL_SIZE;

    for (size_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
        int32_t x = i % MAP_WIDTH;
        int32_t y = i / MAP_WIDTH;

        Vector2 position = v2_sub(v2(x, y), v2(MAP_WIDTH / 2, MAP_HEIGHT / 2));

        if (optimize && v2_dist_to_greater_than(player_entity->position, position, MAX_DIST_TO_PLAYER + 0.1)) {
            continue;
        }

        Vector4 color = hex_to_rgba(0xffffffff);
        //color.g = (f32)world->flow_map[i].approach / (f32)MAX_FLOW_DIST;
        //color.r = (f32)world->flow_map[i].danger / (f32)MAX_FLOW_DIST;

        Draw_Quad* quad = draw_image(tiles, position, v2(1, 1), color);

        quad->uv = get_block_uv_with_direction(i);
    
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
    draw_text(font, tprintf("fps\t: %0.2f", frametime_buffer[FPS_METER_OUTPUT_ID]), 30, v2(0, -60), v2(1, 1), COLOR_WHITE);
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
    draw_text(font, tprintf("fps\t: %0.2f", frametime_buffer[FPS_METER_OUTPUT_ID]), 30, v2(0, -60), v2(1, 1), COLOR_WHITE);
}

// @gameupdate
void update_game_scene(void) {
    reset_draw_frame(&draw_frame);

    Vector2 target_pos = player_entity->position;
    animate_v2_to_target(&camera.position, target_pos, delta_time, 30.0f);

    float64 scale = camera.scale;
    draw_frame.view = m4_make_translation(v3(camera.position.x, camera.position.y, 0));
    draw_frame.view = m4_scale(draw_frame.view, v3(scale, scale, scale));

    update_player();
    update_enemies();
    update_items();
    update_bullets();

    clear_destroyed_entities();

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

    draw_text(font, tprintf("fps\t: %0.2f", frametime_buffer[FPS_METER_OUTPUT_ID]), 30, v2(0, -30), v2(1, 1), COLOR_WHITE);
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

    window.clear_color = hex_to_rgba(0x000000ff);

    camera.scale = CAMERA_SCALE;

    load_screen = load_image_from_disk(STR("res/graphics/loading.png"), get_heap_allocator());
}

// @resources
void game_late_init(void) {
    world = alloc(get_heap_allocator(), sizeof(world_t));

    //font = load_font_from_disk(STR("res/fonts/jacquard.ttf"), get_heap_allocator());
    font = load_font_from_disk(STR("res/fonts/arial.ttf"), get_heap_allocator());

    Audio_Source source = { 0 };

    {
        impact_player = audio_player_get_one();
        impact_player->release_when_done = false;
        impact_player->config.volume = 0.8;

        audio_open_source_stream(&source, STR("res/audio/sfx/bullet_impact_01.wav"), get_heap_allocator());
        audio_player_set_source(impact_player, source);
        audio_player_set_state(impact_player, AUDIO_PLAYER_STATE_PLAYING);
        audio_player_set_looping(impact_player, false);

    }

    {
        shoot_player = audio_player_get_one();
        shoot_player->release_when_done = false;
        impact_player->config.volume = 0.9;

        audio_open_source_stream(&source, STR("res/audio/sfx/bullet_shoot_01.wav"), get_heap_allocator());
        audio_player_set_source(shoot_player, source);
        audio_player_set_state(shoot_player, AUDIO_PLAYER_STATE_PLAYING);
        audio_player_set_looping(shoot_player, false);
    }

    {
        collect_player = audio_player_get_one();
        collect_player->release_when_done = false;
        collect_player->config.volume = 0.65;

        audio_open_source_stream(&source, STR("res/audio/sfx/collect_sound_01.wav"), get_heap_allocator());
        audio_player_set_source(collect_player, source);
        audio_player_set_state(collect_player, AUDIO_PLAYER_STATE_PLAYING);
        audio_player_set_looping(collect_player, false);
    }

    {
        menu_music_player = audio_player_get_one();
        menu_music_player->release_when_done = false;
        menu_music_player->config.volume = 0.5;

        audio_open_source_stream(&source, STR("res/audio/music/menu.wav"), get_heap_allocator());
        audio_player_set_source(menu_music_player, source);
        audio_player_set_state(menu_music_player, AUDIO_PLAYER_STATE_PLAYING);
        audio_player_set_looping(menu_music_player, true);
    }

    tiles = load_image_from_disk(STR("res/graphics/tiles/tilemap_world_01.png"), get_heap_allocator());

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

    size_t cntr = 0;
    while (!window.should_close) {
        reset_temporary_storage();

        now_time = os_get_current_time_in_seconds();
        delta_time = now_time - prew_time;
        if ((cntr % FPS_METER_OUTPUT_ID) == 0) {
            cntr = 0;

            float64 buf = 100000;
            for (size_t i = 0; i < FPS_METER_OUTPUT_ID; i++) {
                if (buf > frametime_buffer[i]) {
                    buf = frametime_buffer[i];
                }
            }

            frametime_buffer[FPS_METER_OUTPUT_ID] = buf;
        }

        frametime_buffer[cntr++] = 1.0 / delta_time;
        
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
