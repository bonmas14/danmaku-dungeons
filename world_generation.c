#define MAX_LIST_SIZE 1024
#define MIN_ROOM_COUNT (5 + 10)

typedef enum door_side_t {
    DOOR_NONE,
    DOOR_ON_LEFT,
    DOOR_ON_RIGHT,
    DOOR_ON_TOP,
    DOOR_ON_BOTTOM
} door_side_t;

typedef struct aabb_room_t {
    s32 center_x;    
    s32 center_y;    
    s32 size_x;    
    s32 size_y; 
    door_side_t door_on_side;
} aabb_room_t;

struct {
    s32 map_width;
    s32 map_height;

    s32 min_tunnel_count; 
    s32 max_tunnel_count; 

    s32 min_tunnel_size; 
    s32 max_tunnel_size; 

    s32 min_room_size; 
    s32 max_room_size; 

    s32 min_rooms_per_parent;
    s32 max_rooms_per_parent;

    s32 distance_between_tunnels;
} gen_conf;

struct {
    size_t attempts_per_room;
} gen_loc_conf;

struct {
    aabb_room_t global_bounds;
    size_t current_index;
    aabb_room_t rooms[MAX_LIST_SIZE];
} room_list;

bool check_collision(aabb_room_t a, aabb_room_t b) {
    bool collide_x = absi(a.center_x - b.center_x) < (a.size_x + b.size_x);
    bool collide_y = absi(a.center_y - b.center_y) < (a.size_y + b.size_y);

    return (collide_x && collide_y);
}

bool room_contains_coord(aabb_room_t container, s32 x, s32 y) {
    bool contains_x = absi(container.center_x - x) < container.size_x;
    bool contains_y = absi(container.center_y - y) < container.size_y;

    return (contains_x && contains_y);
}

void generator_init(void) {
    gen_conf.min_tunnel_count = 2;
    gen_conf.max_tunnel_count = 5;

    gen_conf.min_tunnel_size = 6;
    gen_conf.max_tunnel_size = 8;

    gen_conf.min_room_size = 10;
    gen_conf.max_room_size = 30;
    gen_conf.distance_between_tunnels = 15;

    gen_conf.min_rooms_per_parent = 0;
    gen_conf.max_rooms_per_parent = 10;

    gen_conf.map_width = MAP_WIDTH;
    gen_conf.map_height = MAP_HEIGHT;
}

void generate_base_tunnels(void) {
    int current_count = get_random_int_in_range(gen_conf.min_tunnel_count, gen_conf.max_tunnel_count);

    for (size_t i = 0; i < current_count; i++) {
        bool vertical = i % 2;

        s32 x = get_random_int_in_range(gen_conf.max_tunnel_size, gen_conf.map_width - gen_conf.max_tunnel_size);
        s32 y = get_random_int_in_range(gen_conf.max_tunnel_size, gen_conf.map_height - gen_conf.max_tunnel_size);

        s32 width = get_random_int_in_range(gen_conf.min_tunnel_size, gen_conf.max_tunnel_size) / 2;
        s32 height = get_random_int_in_range(gen_conf.min_tunnel_size, gen_conf.max_tunnel_size) / 2;

        aabb_room_t* current = &(room_list.rooms[room_list.current_index++]);

        if (vertical) {
            y = gen_conf.map_height / 2;
            height = gen_conf.map_height / 2 - 1;
        } else {
            x = gen_conf.map_width / 2;
            width = gen_conf.map_width / 2 - 1;
        }

        *current = (aabb_room_t) {
            .center_x = x,
            .center_y = y,
            .size_x = width, 
            .size_y = height 
        };

        for (size_t j = 0; j < i; j++) {
            if ((j % 2) != (i % 2))
                continue;

            aabb_room_t collision = {
                .center_x = x,
                .center_y = y,
                .size_x = width + gen_conf.distance_between_tunnels,
                .size_y = height + gen_conf.distance_between_tunnels
            };

            if (check_collision(collision, room_list.rooms[j])) {
                room_list.current_index--;
                i--;
                break;
            }
        }

        if (room_list.current_index >= MAX_LIST_SIZE) {
            break;
        }
    }
}

void create_room_on_bounds_hor(aabb_room_t* result, aabb_room_t parent_room) {
    s32 side = get_random_int_in_range(0, 100);

    s32 y;
    if (side > 50) { 
        y = parent_room.center_y + parent_room.size_y + result->size_y + 1;
        result->door_on_side = DOOR_ON_BOTTOM;
    } else {
        y = parent_room.center_y - parent_room.size_y - result->size_y - 1;
        result->door_on_side = DOOR_ON_TOP;
    }

    result->center_x = get_random_int_in_range(parent_room.center_x - parent_room.size_x + result->size_x, parent_room.center_x + parent_room.size_x - result->size_x);
    result->center_y = y;
}
void create_room_on_bounds_vert(aabb_room_t* result, aabb_room_t parent_room) {
    s32 side = get_random_int_in_range(0, 100);
    
    s32 x;

    if (side > 50) { 
        x = parent_room.center_x + parent_room.size_x + result->size_x + 1;
        result->door_on_side = DOOR_ON_LEFT;
    } else {
        x = parent_room.center_x - parent_room.size_x - result->size_x - 1;
        result->door_on_side = DOOR_ON_RIGHT;
    }

    result->center_x = x;
    result->center_y = get_random_int_in_range(parent_room.center_y - parent_room.size_y + result->size_y, parent_room.center_y + parent_room.size_y - result->size_y);
}

void create_room_on_bounds(aabb_room_t parent_room) {
    s32 attempts = gen_loc_conf.attempts_per_room;

    aabb_room_t* current = &(room_list.rooms[room_list.current_index++]);

    while (attempts-- > 0) {
        bool vertical = get_random_int_in_range(0, 1);

        *current = (aabb_room_t) { 
            .size_x = get_random_int_in_range(gen_conf.min_room_size, gen_conf.max_room_size) / 2,
            .size_y = get_random_int_in_range(gen_conf.min_room_size, gen_conf.max_room_size) / 2
        };

        if (vertical) create_room_on_bounds_vert(current, parent_room);
        else          create_room_on_bounds_hor(current,  parent_room);

        if (!room_contains_coord(room_list.global_bounds, current->center_x, current->center_y)) {
            continue;
        }

        bool collided = false;
        for (size_t i = 0; i < (room_list.current_index - 1); i++) {
            if (check_collision(*current, room_list.rooms[i])) {
                collided = true;
                break;
            }
        }

        if (collided) continue;

        return;
    }

    room_list.current_index--;
}

void generate_child_rooms(void) {
    size_t child_start = room_list.current_index;

    if (room_list.current_index >= MAX_LIST_SIZE)
        return;

    for (size_t parent_id = 0; parent_id < child_start; parent_id++) {
        if (room_list.current_index >= MAX_LIST_SIZE)
            break;

        int child_count = get_random_int_in_range(gen_conf.min_rooms_per_parent, gen_conf.max_rooms_per_parent);

        for (size_t child_id = 0; child_id < child_count; child_id++) {
            create_room_on_bounds(room_list.rooms[parent_id]);
            if (room_list.current_index >= MAX_LIST_SIZE)
                break;
        }
    }

    // just connect to 2 different rooms. randomly
}

void generator_clear(void) {
    for (s32 i = 0; i < (gen_conf.map_width * gen_conf.map_height); i++) {
        world->world_map[i] = (block_t) { .type = BLOCK_empty };
    }
    room_list.current_index = 0;
}


void generator_draw_room(aabb_room_t room) {
    s32 start_x = room.center_x - room.size_x;
    s32 start_y = room.center_y - room.size_y;
    s32 end_x = room.center_x + room.size_x;
    s32 end_y = room.center_y + room.size_y;

    //log("draw box dimensions (x,y,x,y): %d %d %d %d", start_x, start_y, end_x, end_y);

    for (s32 y = start_y; y < end_y; y++) {
        for (s32 x = start_x; x < end_x; x++) {
            if ((x < 0 || x > gen_conf.map_width) || (y < 0 || y > gen_conf.map_height))
                continue;

            block_t block = { .type = BLOCK_floor };
            block_t curr = world->world_map[x + y * gen_conf.map_width];

            if (x == start_x || x == (end_x - 1)) {
                if (curr.type == BLOCK_empty || curr.type == BLOCK_wall) {
                    block.type = BLOCK_wall;
                }
            }

            if (y == start_y || y == (end_y - 1)) {
                if (curr.type == BLOCK_empty || curr.type == BLOCK_wall) {
                    block.type = BLOCK_wall;
                }
            }

            world->world_map[x + y * gen_conf.map_width] = block;
        }
    }
}

void generator_draw_door(aabb_room_t to) {
    aabb_room_t door = { .size_x = 3, .size_y = 3 };

    switch (to.door_on_side) {
        case DOOR_ON_BOTTOM:
            door.center_x = to.center_x;
            door.center_y = to.center_y - to.size_y;
            break;
        case DOOR_ON_TOP:
            door.center_x = to.center_x;
            door.center_y = to.center_y + to.size_y;
            break;
        case DOOR_ON_RIGHT:
            door.center_x = to.center_x + to.size_x;
            door.center_y = to.center_y;
            break;
        case DOOR_ON_LEFT:
            door.center_x = to.center_x - to.size_x;
            door.center_y = to.center_y;
            break;
        case DOOR_NONE:
            return;
    }

    generator_draw_room(door);
}

// It freezed randomly one time...
// but it still works after all.
// So no fix for it right now.   | 27.07.2024
void generate_map(void) {
    room_list.global_bounds = (aabb_room_t) { 
        .center_x = gen_conf.map_width  / 2,
        .center_y = gen_conf.map_height / 2,
        .size_x   = gen_conf.map_width  / 2 - gen_conf.max_room_size / 2,
        .size_y   = gen_conf.map_height / 2 - gen_conf.max_room_size / 2
    };

    generator_clear();

    while (room_list.current_index < MIN_ROOM_COUNT) {
        generator_clear();
        generate_base_tunnels();
        gen_loc_conf.attempts_per_room = 1000;
        generate_child_rooms();
        gen_loc_conf.attempts_per_room = 100;
        generate_child_rooms();
        gen_loc_conf.attempts_per_room = 10;
        generate_child_rooms();
    }

    for (size_t i = room_list.current_index; i > 0; i--) {
        size_t index = i - 1;
        aabb_room_t room = room_list.rooms[index];

        generator_draw_room(room);
        generator_draw_door(room);

        if (index == 0) {
            size_t coord = room.center_x + room.center_y * gen_conf.map_width;
            world->world_map[coord].prototype.entity_type = ENTITY_player;
        } else {
            s32 x_min = room.center_x - room.size_x + 1;
            s32 x_max = room.center_x + room.size_x - 1;

            s32 y_min = room.center_y - room.size_y + 1;
            s32 y_max = room.center_y + room.size_y - 1;
            
            // big enemies in big rooms!
            s32 enemy_count = get_random_int_in_range(1, 10);
            for (s32 j = 0; j < enemy_count; j ++) {
                s32 x = get_random_int_in_range(x_min, x_max - 1);
                s32 y = get_random_int_in_range(y_min, y_max - 1);

                if (world->world_map[x + y * gen_conf.map_width].prototype.entity_type == ENTITY_null)
                    world->world_map[x + y * gen_conf.map_width].prototype.entity_type = ENTITY_enemy;
            }
        }
    }

    // @todo
    // cellular automata step to fill all emply cells and give proper directions for blocks!
    for (size_t i = 0; i < (gen_conf.map_width * gen_conf.map_height); i++) {
        break;
    }
}
