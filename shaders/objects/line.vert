__VERSION__

in uint index;

vec4 position[4] = vec4[](
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, -1.0, 0.0, 1.0),
	vec4(1.0, -1.0, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0)
);

uniform mat4 mvp;

void main()
{
	gl_Position = mvp * position[index];
}
