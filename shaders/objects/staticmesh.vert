__VERSION__

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 lightmvp;
uniform mat3 normalMatrix;
uniform vec4 globalColor;
uniform int meshID;

uniform bool useVertexTransform = false;
uniform float iTime = 0.;

uniform float size = 1.0;

// Static attributes
in vec3 position;
in vec2 uv;
in vec3 normal;
in vec4 color;

out vec4 v_position;
out vec3 v_normal;
out vec3 v_wnormal;
out vec2 v_uv;
out vec4 v_color;
// xyzw: mesh id, vertex id, unused, unused
flat out ivec4 v_data;
out vec4 v_lightposition;

void main()
{
	vec3 p = position * size;
	if (useVertexTransform) {
		//p = p + (p * sin(float(gl_VertexID) + iTime) * 0.1);
		p = p + sin(iTime * p.x + p.y + p.z) * 0.1;
	}
	v_position =  model * vec4(p, 1.0);
	v_data = ivec4(meshID, gl_VertexID, 0, 0);
	gl_Position = mvp * vec4(p, 1.0);
	v_normal =  transpose(inverse(mat3(model))) * normal;
	v_wnormal = normal;
	v_uv = uv;
	v_color = color * globalColor;
	v_lightposition = lightmvp * v_position;
}
