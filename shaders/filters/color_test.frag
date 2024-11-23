__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

void main() 
{
    float b = texCoord.r * texCoord.g;

	fragColor = vec4(texCoord.r - b, texCoord.g - b, b, 1.0);
}