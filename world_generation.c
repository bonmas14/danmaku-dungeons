#define MAX_LIST_SIZE 256

struct aabb_room_t {
    int32_t center_x;    
    int32_t center_y;    
    int32_t size_x;    
    int32_t size_y;    
};

struct {
    size_t current_index;
    struct aabb_room_t rooms[MAX_LIST_SIZE];
} room_list;

bool check_collision(struct aabb_room_t a, struct aabb_room_t b) {
    bool collide_x = absi(a.center_x - b.center_x) > (a.size_x + b.size_x);
    bool collide_y = absi(a.center_y - b.center_y) > (a.size_y + b.size_y);

    return !(collide_x || collide_y);
}

void generator_init(void) {
    gen_conf.min_tunnel_count = 5;
    gen_conf.max_tunnel_count = 8;
    gen_conf.min_room_size = 6;
    gen_conf.max_room_size = 10;
}

void generate_base_tunnels(void) {
    int current_count = get_random_int_in_range(gen_conf.min_tunnel_count, gen_conf.max_tunnel_count);

    for (size_t i = 0; i < current_count; i++) {
        bool vertical = i % 2;

        int32_t x = get_random_int_in_range(gen_conf.max_room_size, MAP_WIDTH - gen_conf.max_room_size);
        int32_t y = get_random_int_in_range(gen_conf.max_room_size, MAP_HEIGHT - gen_conf.max_room_size);

        int32_t width = get_random_int_in_range(gen_conf.min_room_size, gen_conf.max_room_size) / 2;
        int32_t height = get_random_int_in_range(gen_conf.min_room_size, gen_conf.max_room_size) / 2;

        struct aabb_room_t* current = &(room_list.rooms[room_list.current_index++]);

        if (vertical) {
            y = MAP_HEIGHT / 2;
            height = MAP_HEIGHT / 2 - 1;
        } else {
            x = MAP_WIDTH / 2;
            width = MAP_WIDTH / 2 - 1;
        }


        *current = (struct aabb_room_t) {
            .center_x = x,
            .center_y = y,
            .size_x = width, 
            .size_y = height 
        };

        for (size_t j = 0; j < i; j++) {
            if ((j % 2) != (i % 2))
                continue;

            if (check_collision(*current, room_list.rooms[j])) {
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

void generator_clear(void) {
    for (int32_t i = 0; i < (MAP_WIDTH * MAP_HEIGHT); i++) {
        world->world_map[i] = (block_t) { .type = BLOCK_empty };
    }
    room_list.current_index = 0;
}

void generator_draw_room(struct aabb_room_t room) {
    int32_t start_x = room.center_x - room.size_x;
    int32_t start_y = room.center_y - room.size_y;
    int32_t end_x = room.center_x + room.size_x;
    int32_t end_y = room.center_y + room.size_y;

    //log("draw box dimensions (x,y,x,y): %d %d %d %d", start_x, start_y, end_x, end_y);

    for (int32_t y = start_y; y < end_y; y++) {
        for (int32_t x = start_x; x < end_x; x++) {

            block_t block = { .type = BLOCK_floor };

            if (x == start_x || x == (end_x - 1)) {
                if (world->world_map[x + y * MAP_WIDTH].type == BLOCK_empty) {
                    block.type = BLOCK_wall;
                }
            }

            if (y == start_y || y == (end_y - 1)) {
                if (world->world_map[x + y * MAP_WIDTH].type == BLOCK_empty) {
                    block.type = BLOCK_wall;
                }
            }

            world->world_map[x + y * MAP_WIDTH] = block;
        }
    }
}

void generate_map(void) {
    generator_clear();

    generate_base_tunnels();

    for (size_t i = room_list.current_index; i > 0; i--) {
        size_t index = i - 1;
        struct aabb_room_t room = room_list.rooms[index];

        generator_draw_room(room);
    }
}
