#include "collisions.c"


// https://github.com/bigrando420/game1/commit/8162abee3bd5de0c183bd25b60c0989416647c74

Vector2 screen_to_world(Vector2 mouse, Vector2 window, Draw_Frame frame) {
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;

	// Normalize the mouse coordinates

	float ndc_x = (mouse.x / (window.x * 0.5f)) - 1.0f;
	float ndc_y = (mouse.y / (window.y * 0.5f)) - 1.0f;

	// Transform to world coordinates
    
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	// Return as 2D vector
	return (Vector2){ world_pos.x, world_pos.y };
}

// https://github.com/bigrando420/game1/commit/89bbc0fe4d1de7376443ba9e3fd1bf2ab84fc48e 
bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}
