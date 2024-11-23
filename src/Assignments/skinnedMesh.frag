__VERSION__

#include "lights.frag"

// Input attributes from vertex shader
in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

// Renderer expects two outputs of fragment shader: color and primitive data
out vec4 frag_color;
out vec4 frag_prim;

// Global variables shared by all fragments
uniform bool useTexture;
uniform sampler2D inputTexture;
uniform vec4 baseColorFactor = vec4(1.0);
uniform float alphaCutoff;
uniform float timeOfDay = 0.;

void main()
{
	vec4 result = baseColorFactor;
	vec2 uv = v_uv;

	vec4 texResult = texture(inputTexture, uv);
	if (texResult.a < alphaCutoff) discard;
	if (useTexture) result *= texResult;

	result *= lighting(v_normal.xyz, v_position);

	frag_color = result;

	frag_prim = vec4(0.0, 0.0, gl_PrimitiveID, 1.0);
}