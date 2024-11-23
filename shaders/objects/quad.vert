__VERSION__

in uint index;

vec4 position[4] = {
	{-1.0, -1.0, 1.0, 1.0},
	{1.0, -1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{-1.0, 1.0, 1.0, 1.0}
};

vec2 uv[4] = {
	{0.0, 0.0},
	{1.0, 0.0},
	{1.0, 1.0},
	{0.0, 1.0}
};

uniform mat4 mvp;

out vec2 fragUV;

void main()
{
	gl_Position = mvp * position[index];
	fragUV = uv[index];
}
