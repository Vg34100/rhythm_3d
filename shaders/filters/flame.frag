__VERSION__

__UNIFORMS__

#include "lib/classicnoise2D.glsl"

in vec2 texCoord;
out vec4 fragColor;

vec4 edgeColor = vec4(1.0);
vec4 midColor = vec4(1.0, 0.32, .27, 1.);

uniform float threshold = 0.15;

uniform	float xs = 3.14;
uniform	float ys = 1.0;
uniform float xint = 1.0;
uniform	float yint = 0.2;

/*
-----------------------
|   |             |   |           
|    |           |    |           
|    \           /    |           
|     \         /     |           
-------|       |-------
|       \     /       |           
|        |   |        |           
|         \ /         |           
|         | |         |
-----------V----------
*/
vec3 mask_layer(vec2 uv) {

	float right = pow( xs*uv.x-xint, 3) + yint;
	float left = -(pow(xs*uv.x+xint, 3) - yint);

	float yv = uv.y * ys;
	vec3 dists = vec3(left - yv, right - yv, 0.);
	dists.z = min(abs(dists.x), abs(dists.y));
	//bvec2 aboves = bvec2(dists.x < 0., dists.y < 0.);

	return dists;

	//if ((uv.x < 0. && above_left) || (uv.x > 0. && above_right)) return 1.0;
	//if (aboves.x && aboves.y) return 1.0;

	//if (uv.x > 0.5) return 1.0;

	//return 0.0;
}

vec3 noise_layer(vec2 uv, vec2 coord) {
	return vec3(cnoise(uv));
}

/*
const float[3][3] kernel = {
	{0.125, 0.125, 0.125},
	{0.125, -1.0, 0.125},
	{0.125, 0.125, 0.125}
};
*/

uniform float kernelWeight = 0.11111;
uniform float blurDist = 0.0078125;

uniform vec4 outerFlameColor = vec4(0.93, 0.07, 0.01, 1.0);
uniform vec4 middleFlameColor = vec4(0.8, 1.0, 0.01, 1.0);
uniform vec4 innerFlameColor = vec4(1.0, 1.0, 1.0, 1.0);

vec4 getImage(vec2 uv, vec2 ndc) {
	vec4 result = vec4(0.);
	
	for(int i = -1; i < 2; i++) {
		float x = float(i) * blurDist + ndc.x;
		float uvx = float(i) * blurDist + uv.x;
		for(int j = -1; j < 2; j++) {
			float y = float(j) * blurDist + ndc.y;
			float uvy = float(i) * blurDist + uv.y;

			vec2 cuv = vec2(uvx, uvy);
			vec2 cndc = vec2(x, y) + 0.05 * noise_layer(cuv + vec2(iTime), vec2(x, y)).xy;
			

			vec3 masked = mask_layer(cndc);

			if (masked.x < 0. && masked.y < 0.) {
				vec3 maskedColor = vec3(masked.z);
				maskedColor += noise_layer(cuv, cndc);

				vec3 maskAndFlame = vec3(0.);
				if (masked.z < 0.25) {
					maskAndFlame = mix(outerFlameColor.xyz, middleFlameColor.xyz, masked.z / 0.25);
					//maskAndFlame = mix(innerFlameColor.xyz, outerFlameColor.xyz, masked.z);
				}
				else if (masked.z < 1.0) {
					maskAndFlame = mix(middleFlameColor.xyz, innerFlameColor.xyz, (masked.z - 0.25) / 0.75);
				}

				else if (masked.z >= 1.0) {
					maskAndFlame = innerFlameColor.xyz * masked.z;
				}

				result += kernelWeight * vec4(maskAndFlame, 1.0);
			}
		}
	}

	return result;
}



void main() 
{
	vec2 ndc = (texCoord - vec2(0.5)) / 0.5;

	fragColor = getImage(texCoord, ndc);

	

}