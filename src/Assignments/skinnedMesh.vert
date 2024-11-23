__VERSION__

uniform mat4 viewproj;

uniform mat4 joints[100];
uniform bool useSkinning;
uniform mat4 rigidParent;

// Static attributes
in vec3 position;
in vec2 uv;
in vec3 normal;
in ivec4 indices;
in vec4 weights;

out vec4 v_position;
out vec4 v_normal;
out vec2 v_uv;

void main()
{
	vec3 p = position;

	mat4 skinMat = mat4(0.0);
	if (useSkinning) {
		for(int i = 0; i < 4; i++) {
			float wi = weights[i];
			if (wi == 0.0) break;
			skinMat += joints[indices[i]] * wi;
		}
	}
	else {
		skinMat = rigidParent;
	}

	vec4 worldPos = skinMat * vec4(p, 1.0);
	v_position = worldPos;
	v_normal = skinMat * vec4(normal, 0.0);
	gl_Position = viewproj * worldPos;
	v_uv = uv;
}