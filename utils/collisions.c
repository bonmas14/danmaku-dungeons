
// ---- structs ---- //

typedef struct circle_t {
    Vector2 position;
    float32 radius;
} circle_t;

// ---- functions ---- //
inline circle_t circle(float32 radius, Vector2 position) { return (circle_t){ .radius = radius, .position = position }; }

inline float32 v2_magn(Vector2 a) {
    return sqrt(a.x * a.x + a.y * a.y);
}

inline float32 v2_sqr_magn(Vector2 a) {
    return (a.x * a.x + a.y * a.y);
}

inline float32 v2_dist(Vector2 a, Vector2 b) {
    return v2_magn(v2_sub(a, b));
}

inline float32 v2_sqr_dist(Vector2 a, Vector2 b) {
    return v2_sqr_magn(v2_sub(a, b));
}

inline bool v2_dist_to_greater_than(Vector2 a, Vector2 b, float32 value) {
    return v2_sqr_dist(a, b) > (value * value);
}

inline bool v2_magn_greater_than(Vector2 a, float32 value) {
    return v2_sqr_magn(a) > (value * value);
}

inline bool check_collision_circle_to_circle(circle_t a, circle_t b) {
    return v2_sqr_dist(a.position, b.position) < (a.radius * a.radius + b.radius * b.radius);
}
