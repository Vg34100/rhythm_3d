__VERSION__

#define Z 0.0

// Built-in vertices for the two fullscreen triangles
const vec4 vertices[6] = vec4[6](
	vec4(-1, -1, Z, 1), 
	vec4(1, -1, Z, 1), 
	vec4(1, 1, Z, 1),
	vec4(1, 1, Z, 1), 
	vec4(-1, 1, Z, 1), 
	vec4(-1, -1, Z, 1)
);

const vec2 uvs[6] = vec2[6](
	vec2(0, 0),
	vec2(1, 0),
	vec2(1, 1),
	vec2(1, 1),
	vec2(0, 1),
	vec2(0, 0)
);

out vec2 uv;

void main() {
	uv = uvs[gl_VertexID];
	gl_Position = vertices[gl_VertexID];
}