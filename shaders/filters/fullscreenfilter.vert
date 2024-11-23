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

out vec2 texCoord;

void main() 
{
   texCoord = uv[index];
   gl_Position = position[index];
}