__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

vec4 edgeColor = vec4(1.0);
vec4 midColor = vec4(1.0, 0.32, .27, 1.);

uniform float threshold = 0.15;

void main() 
{
	vec4 result = vec4(0.);
	vec2 d = texCoord - vec2(0.5);
	float dl = length(d);

	if (dl > 0.5) {
		discard;
	}
	else {
		float t = dl / 0.5;
		t = sin(iTime + t);
		result = mix(midColor, edgeColor, t);
		result.a = 1.0 - t;
	}

	fragColor = result;
}