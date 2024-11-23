__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

uniform float period = 1.;

uniform sampler2D filterInput;
uniform sampler2D depthInput;

uniform ivec2 parameters = ivec2(3, 3);

uniform vec2 nearFar = vec2(0.01, 1000.0);
uniform vec2 depthRange = vec2(5.0, 10.0);

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main() 
{
	fragColor = vec4(0.);

	float depthVal = texture(depthInput, texCoord).x;
	depthVal = linearize_depth(depthVal, nearFar.x, nearFar.y);
	vec4 actualColor = texture(filterInput, texCoord);

	for (int i = -parameters.x; i <= parameters.x; ++i) {
		for (int j = -parameters.x; j <= parameters.x; ++j) {
			vec2 tc = texCoord + (vec2(i, j) * parameters.y) / resolution;
			fragColor += texture( filterInput, tc);
		}
	}


	fragColor /= pow(parameters.x * 2 + 1, 2);
	
	if (depthVal <= depthRange.y && depthVal >= depthRange.x) {
		float depthDiff = depthRange.y - depthRange.x;

		// Gives value from 0 to 1
		float depthCenter = depthRange.x + (0.5 * depthDiff);
		float depthOffset = abs(depthCenter - depthVal) / (0.5 * depthDiff);

		depthOffset = clamp(depthOffset, 0., 1.);

		fragColor = mix(actualColor, fragColor, depthOffset);
		//fragColor = actualColor;
	}
}