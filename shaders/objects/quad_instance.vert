__VERSION__

in uint index;

vec4 position[4] = vec4[](
	vec4(-1.0, -1.0, 1.0, 1.0),
	vec4(1.0, -1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(-1.0, 1.0, 1.0, 1.0)
);

vec2 uv[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
);

in vec4 m0;
in vec4 m1;
in vec4 m2;
in vec4 m3;
in vec4 color;
in vec4 data;

uniform mat4 vp;

out vec2 fragUV;
out vec4 vertexColor;

void main()
{
	mat4 m;
	m[0] = m0;
	m[1] = m1;
	m[2] = m2;
	m[3] = m3;

	mat4 mvp = vp * m;

	uint usableIndex = index;
	if (data.x < 1.0) usableIndex = 0u;

	gl_Position = mvp * position[usableIndex];
	fragUV = uv[index];
	vertexColor = color;
}
