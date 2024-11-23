__VERSION__

uniform mat4 mvp;

uniform bool useVertexTransform = false;
uniform float iTime = 0.;
uniform float size = 1.0;

// Static attributes
in vec3 position;

void main()
{
	vec3 p = position * size;
	if (useVertexTransform) {
		//p = p + (p * sin(float(gl_VertexID) + iTime) * 0.1);
		p = p + sin(iTime * p.x + p.y + p.z) * 0.1;
	}
	gl_Position = mvp * vec4(p, 1.0);
}
