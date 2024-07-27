#define MAX_LIST_SIZE 1024
#define MIN_ROOM_COUNT (5 + 10)

typedef struct aabb_room_t {
    int32_t center_x;    
    int32_t center_y;    
    int32_t size_x;    
    int32_t size_y; 
} aabb_room_t;

struct {
    int32_t map_width;
    int32_t map_height;

    int32_t min_tunnel_count; 
    int32_t max_tunnel_count; 

    int32_t min_tunnel_size; 
    int32_t max_tunnel_size; 

    int32_t min_room_size; 
    int32_t max_room_size; 

    int32_t min_rooms_per_parent;
    int32_t max_rooms_per_parent;

    int32_t distance_between_tunnels;
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

bool room_contains_coord(aabb_room_t container, int32_t x, int32_t y) {
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

        int32_t x = get_random_int_in_range(gen_conf.max_tunnel_size, gen_conf.map_width - gen_conf.max_tunnel_size);
        int32_t y = get_random_int_in_range(gen_conf.max_tunnel_size, gen_conf.map_height - gen_conf.max_tunnel_size);

        int32_t width = get_random_int_in_range(gen_conf.min_tunnel_size, gen_conf.max_tunnel_size) / 2;
        int32_t height = get_random_int_in_range(gen_conf.min_tunnel_size, gen_conf.max_tunnel_size) / 2;

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
    int32_t side = get_random_int_in_range(0, 100);

    int32_t y;
    if (side > 50) { 
        y = parent_room.center_y + parent_room.size_y + result->size_y + 1;
    } else {
        y = parent_room.center_y - parent_room.size_y - result->size_y - 1;
    }

    result->center_x = get_random_int_in_range(parent_room.center_x - parent_room.size_x + result->size_x, parent_room.center_x + parent_room.size_x - result->size_x);
    result->center_y = y;
}
void create_room_on_bounds_vert(aabb_room_t* result, aabb_room_t parent_room) {
    int32_t side = get_random_int_in_range(0, 100);
    
    int32_t x;

    if (side > 50) { 
        x = parent_room.center_x + parent_room.size_x + result->size_x + 1;
    } else {
        x = parent_room.center_x - parent_room.size_x - result->size_x - 1;
    }

    result->center_x = x;
    result->center_y = get_random_int_in_range(parent_room.center_y - parent_room.size_y + result->size_y, parent_room.center_y + parent_room.size_y - result->size_y);
}

void create_room_on_bounds(aabb_room_t parent_room) {
    int32_t attempts = gen_loc_conf.attempts_per_room;

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
    for (int32_t i = 0; i < (gen_conf.map_width * gen_conf.map_height); i++) {
        world->world_map[i] = (block_t) { .type = BLOCK_empty };
    }
    room_list.current_index = 0;
}

void generator_draw_room(aabb_room_t room) {
    int32_t start_x = room.center_x - room.size_x;
    int32_t start_y = room.center_y - room.size_y;
    int32_t end_x = room.center_x + room.size_x;
    int32_t end_y = room.center_y + room.size_y;

    //log("draw box dimensions (x,y,x,y): %d %d %d %d", start_x, start_y, end_x, end_y);

    for (int32_t y = start_y; y < end_y; y++) {
        for (int32_t x = start_x; x < end_x; x++) {
            if ((x < 0 || x > gen_conf.map_width) || (y < 0 || y > gen_conf.map_height))
                continue;

            block_t block = { .type = BLOCK_floor };

            if (x == start_x || x == (end_x - 1)) {
                if (world->world_map[x + y * gen_conf.map_width].type == BLOCK_empty) {
                    block.type = BLOCK_wall;
                }
            }

            if (y == start_y || y == (end_y - 1)) {
                if (world->world_map[x + y * gen_conf.map_width].type == BLOCK_empty) {
                    block.type = BLOCK_wall;
                }
            }

            world->world_map[x + y * gen_conf.map_width] = block;
        }
    }
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

    // player spawn position is first room we render!
    for (size_t i = room_list.current_index; i > 0; i--) {
        size_t index = i - 1;
        aabb_room_t room = room_list.rooms[index];

        generator_draw_room(room);

        
        if (i == 0) {
            size_t coord = room.center_x + room.center_y * gen_conf.map_width;
            world->world_map[coord].prototype.entity_type = ENTITY_player;
        } else {
            //spawn some enemies
        }
    }
}
