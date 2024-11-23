__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

uniform float period = 1.;

uniform sampler2D filterInput;

void main() 
{
	fragColor = texture(filterInput, texCoord);

	if (texCoord.y < 0.5) {
		float r = sin(iTime / period);
		float g = 0.;
		if (r < 0) {
			g = abs(r);
		}
		fragColor = vec4(r, g, 1.0 - sin((texCoord.x + texCoord.y) * 0.5), 1.);
	}
}