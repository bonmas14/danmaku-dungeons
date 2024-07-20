
// ---- structs ---- //

typedef struct circle_t {
    Vector2 position;
    float32 radius;
} circle_t;

// ---- functions ---- //
inline circle_t circle(float32 radius, Vector2 position) { return (circle_t){ .radius = radius, .position = position }; }

float v2_magn(Vector2 a) {
    return sqrt(a.x * a.x + a.y * a.y);
}

float32 v2_dist(Vector2 a, Vector2 b) {
    return v2_magn(v2_sub(a, b));
}

bool check_collision_circle_to_circle(circle_t a, circle_t b) {
    float32 dist = v2_dist(a.position, b.position);
    return dist < (a.radius + b.radius);
}
