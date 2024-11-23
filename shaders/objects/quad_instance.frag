__VERSION__

uniform bool useTexture = false;
uniform vec4 mainColor = vec4(1.);

uniform sampler2D quadTexture;

// Offsets for texture sprite sheets
// xy: beginning of offset coordinates
// zw: scale of offset coordinates
uniform vec4 uvOffset = vec4(0, 0, 1, 1);

in vec2 fragUV;
in vec4 vertexColor;
out vec4 fragColor;

void main() 
{
	vec4 result = vertexColor * mainColor;

	if (useTexture) {
		float uvXOffset = uvOffset.x;
		float uvYOffset = uvOffset.y;

		float uvXScale = uvOffset.z;
		float uvYScale = uvOffset.w;

		vec2 uvOrigin = vec2(uvXOffset, uvYOffset);
		vec2 uvSize = vec2(uvXScale * fragUV.x, uvYScale * fragUV.y);

		vec2 uv = vec2(uvOrigin.x + uvSize.x, uvOrigin.y + uvSize.y);

		vec4 tc = texture(quadTexture, uv);

		if (tc.a < 0.5) discard;
		if (length(tc.rgb) < 0.25) discard;

		result *= tc;
	}
	fragColor = result;
}
