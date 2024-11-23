__VERSION__

uniform vec4 color;

uniform bool useTexture = false;

uniform sampler2D quadTexture;

in vec2 fragUV;
out vec4 fragColor;

void main() 
{
	vec4 result = color;

	if (useTexture) {
		vec4 tc = texture(quadTexture, fragUV);

		if (tc.a < 0.05) discard;

		result *= tc;
	}
	fragColor = result;
}
