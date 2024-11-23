__VERSION__

uniform sampler2D colorTex;
uniform sampler2D normalTex;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

uniform float Ia;
uniform vec3 Ka;

uniform sampler2D inputTexture;
uniform bool useTexture;

uniform sampler2D normalTexture;
uniform bool useNormalTexture;

// Diffuse
uniform float Id;
uniform vec3 Kd;

// Specular
uniform float shininess;
uniform vec3 Ks;

in vec2 uv;

out vec4 FragColor;

uniform vec2 offset[9] = vec2[9](vec2(-1.0, +1.0), vec2(+0.0, +1.0), vec2(+1.0, +1.0), 
                               vec2(-1.0, +0.0), vec2(+0.0, +0.0), vec2(+1.0, +0.0), 
                               vec2(-1.0, -1.0), vec2(+0.0, -1.0), vec2(+1.0, -1.0));

uniform float kernel[9] = float[9](0.0, -1.0, 0.0,
									-1.0, 4.0, -1.0,
									0.0, -1.0, 0.0);

vec2 resolution = vec2(2460.0, 1340.0);

void main() {

	/*
	vec3 sum = vec3(0.);

	for(int i = 0; i < 9; i++) {
		vec2 uv_offset = uv + (offset[i] / resolution.xy);
		vec4 c_offset = texture(colorTex, uv_offset);
		vec3 n_offset = texture(normalTex, uv_offset).xyz;

		vec3 final_offset = max(0.0, dot(n_offset, -lightDirection)) * c_offset.rgb;
		sum += final_offset * kernel[i];
	}

	float threshold = 0.0125;

	vec3 finalColor = vec3(0.);

	if (length(sum) > threshold) {
		
		//finalColor = texture(normalTex, uv).rgb;
		finalColor = vec3(1.0);
	}

	*/

	vec4 c = texture(colorTex, uv);
	vec3 n = texture(normalTex, uv).xyz;

	vec3 finalColor = max(0.0, dot(n, -lightDirection)) * c.rgb;

	FragColor = vec4(finalColor.rgb, 1.0);
	
}
