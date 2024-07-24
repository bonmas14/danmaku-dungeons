#define MAX_LIST_SIZE 64

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

    return collide_x || collide_y;
}

void generator_init(void) {
    gen_conf.min_tunnel_count = 3;
    gen_conf.max_tunnel_count = 5;

    gen_conf.min_room_size = 2;
    gen_conf.max_room_size = 4;
}

void generate_base_tunnels(void) {
    int current_count = get_random_int_in_range(gen_conf.min_tunnel_count, 
            gen_conf.max_tunnel_count);

    for (size_t i = 0; i < current_count; i++) {
        bool vertical = i % 2;

        int32_t x = get_random_int_in_range(gen_conf.max_room_size, MAP_WIDTH - gen_conf.max_room_size);
        int32_t y = get_random_int_in_range(gen_conf.max_room_size, MAP_HEIGHT - gen_conf.max_room_size);

        int32_t width = get_random_int_in_range(gen_conf.min_room_size, gen_conf.max_room_size);
        int32_t height = get_random_int_in_range(gen_conf.min_room_size, gen_conf.max_room_size);

        struct aabb_room_t* current = &(room_list.rooms[room_list.current_index++]);

        if (vertical) {
            y = MAP_HEIGHT / 2;
            height = MAP_HEIGHT / 2;
        } else {
            x = MAP_WIDTH / 2;
            width = MAP_WIDTH / 2;
        }

        *current = (struct aabb_room_t) {
            .center_x = x,
            .center_y = y,
            .size_x = width, 
            .size_y = height 
        };
    }
}

void generate_map(void) {
    generate_base_tunnels();
    struct aabb_room_t a = room_list.rooms[0];
}
