__VERSION__

#include "lights.frag"

in vec4 v_position;
in vec3 v_normal;
in vec3 v_wnormal;
in vec2 v_uv;
//in vec4 v_color;
flat in ivec4 v_data;
in vec4 v_lightposition;

// First fragment out: color
out vec4 frag_color;

// Second fragment out: xyzw: mesh id, vertex id, prim id, unused
out vec4 frag_prim;

uniform vec4 baseColorFactor = vec4(1.0);

uniform bool useTexture = false;
uniform sampler2D inputTexture;

uniform bool useShadow = false;
uniform sampler2DShadow shadowTexture;

// Min and max for shadow bias
uniform vec2 shadowBias = vec2(0.005);
uniform vec3 lightPosition;
uniform vec3 lightDirection;

uniform bool isSelected = false;
uniform vec4 selectedColor = vec4(1., 1., 0., 1.);

uniform bool isLight = false;

uniform bool useLighting = true;

uniform bool showDepth = false;

uniform float c = 80.0;

uniform float iTime = 0.;
uniform bool useFragmentTransform = false;

float shadowCalcPCF() {
	if (!useShadow) return 0.0;

	float bias = shadowBias.x * tan(acos(dot(v_wnormal, lightDirection)));
	vec4 projCoordsW = v_lightposition;
	projCoordsW.z -= bias;


	vec3 projCoords = projCoordsW.xyz / projCoordsW.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0) {
        return 0.0;
	}

	float closestDepth = texture(shadowTexture, projCoords.xyz);
	float currentDepth = projCoords.z;

	/*
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowTexture, 0);
	vec3 texelSize3 = vec3(texelSize.x, texelSize.y, 0.);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowTexture, projCoords.xyz + vec3(x, y, 0) * texelSize3).r; 
			shadow += currentDepth - bias > pcfDepth ? 0.8 : 0.0;        
		}    
	}
	shadow /= 9.0;
	*/

	vec3 lightDirection2 = normalize(lightPosition - v_position.xyz);

	//float bias = max(shadowBias.y * (1.0 - dot(v_normal, lightDirection)), shadowBias.x);
	
	//bias = clamp(bias, shadowBias.x, shadowBias.y);

	float shadow = currentDepth - bias > closestDepth ? 0.8 : 0.0;

	if (closestDepth > projCoords.z - bias) {
		shadow = 0.0;
	}
	

	//return 0.5;

	return shadow;
}

float shadowCalc() {
	if (!useShadow) return 0.0;
	

	//float bias = shadowBias.x * tan(acos(dot(v_normal, lightDirection)));
	//float bias = max(shadowBias.y * (1.0 - dot(v_normal, lightDirection)), shadowBias.x);
	float bias = shadowBias.x;
	vec4 projCoordsW = v_lightposition ;//+ (vec4(v_normal, 1.0) * bias);
	


	vec3 projCoords = projCoordsW.xyz / projCoordsW.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0) {
        return 0.0;
	}

	float closestDepth = texture(shadowTexture, projCoords.xyz);
	float currentDepth = projCoords.z;

	vec3 lightDirection2 = normalize(lightPosition - v_position.xyz);

	
	
	//bias = clamp(bias, shadowBias.x, shadowBias.y);

	float shadow = currentDepth - bias > closestDepth ? 0.8 : 0.0;
	

	//return 0.5;

	return shadow;
}

float expShadowCalc() {
	if (!useShadow) return 0.0;

	vec3 projCoords = v_lightposition.xyz / v_lightposition.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0) {
        return 0.0;
	}

	float z = texture(shadowTexture, projCoords.xyz);
	//float d = length(lightPosition - v_position.xyz);
	float d = projCoords.z;

	z = exp(c * z);

	float shadow = clamp(exp(-c*d) * z, 0.0, 0.8);

	return 1.0 - shadow;
}

float linearize_depth2(float d,float zNear,float zFar)
{
    float z_n = 2.0 * d - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

void main()
{
	//vec4 result = v_color;
	vec4 result = baseColorFactor;

	if (useLighting) {
		result *= lighting(v_normal, v_position);
	}

	if (useTexture) {
		vec2 uv = v_uv;
		if (useFragmentTransform) {
			float period = 10.0;
			float m = mod(iTime, period);
			float t = m / period;

			uv -= vec2(0.5);

			if (t < 0.5) uv *= m;
			else {
				uv *= period - m;
			}

			uv += vec2(0.5);

		}
		vec4 texResult = texture(inputTexture, uv);
		if (texResult.a < 0.1) discard;
		result *= texResult;
	}

	if (useLighting && useShadow && !isLight) {
		result *= (1.0 - shadowCalc());
	}

	if (isSelected) {
		result += selectedColor * 0.4;
	}

	if (isLight) {
		frag_color = baseColorFactor;
	}
	else {
		frag_color = result;
	}

	frag_prim = vec4(v_data.x, v_data.y, gl_PrimitiveID, 1.0);

	/*
	if (useFragmentTransform && useTexture && result.x > 0.5 && result.y < 0.2) {
		discard;
	}
	*/

	if (showDepth) {
		frag_color = vec4( vec3( linearize_depth2(gl_FragCoord.z, 0.01, 100.0) / 100.0), 1.0);
	}

	//if (result.a == 0) discard;
}