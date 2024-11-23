#include "datatypes.glsl"

uniform vec3 viewDirection;

layout(std140) uniform LightData {
	SceneLight sceneLight;
	//int numLights;
	Light lights[GPU_LIGHT_MAX_COUNT];
};

Material frontMaterial = Material(
	vec4(0.2, 0.2, 0.2, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	200.0
);

vec4 lighting(vec3 normal, vec4 position)
{
	vec3 normalDirection = normalize(normal);

	//vec3 viewDirection = normalize(vec3(v_inv * vec4(0.0, 0.0, 0.0, 1.0) - position.xyz));

	vec3 lightDirection;
	float attenuation;
 
	// initialize total lighting with ambient lighting
	vec3 totalLighting = vec3(sceneLight.scene_ambient) * vec3(frontMaterial.ambient);
 
	int numLights = lights.length();
	for (int index = 0; index < numLights; index++) // for all light sources
	{
		if (0.0 == lights[index].position.w) // directional light?
		{
			attenuation = 1.0; // no attenuation
			//lightDirection = normalize(vec3(lights[index].position.xyz));
			lightDirection = normalize(-vec3(lights[index].direction.xyz));
		} 
		else // point light or spotlight (or other kind of light) 
		{
			vec3 positionToLightSource = lights[index].position.xyz - position.xyz;
			float distance = length(positionToLightSource);
			lightDirection = normalize(positionToLightSource);
			attenuation = 1.0 / (lights[index].attenuation.x
						+ lights[index].attenuation.y * distance
						+ lights[index].attenuation.z * distance * distance);
 
			if (lights[index].spot.x <= 90.0) // spotlight?
				{
					float clampedCosine = max(0.0, dot(-lightDirection, normalize(lights[index].direction.xyz)));
					if (clampedCosine < cos(radians(lights[index].spot.x))) // outside of spotlight cone?
					{
						attenuation = 0.0;
					}
					else
					{
						attenuation = attenuation * pow(clampedCosine, lights[index].spot.y);   
					}
				}
		}
 
		vec3 diffuseReflection = attenuation 
			* lights[index].diffuse.rgb * frontMaterial.diffuse.rgb
			* max(0.0, dot(normalDirection, lightDirection));
 
		vec3 specularReflection;
		if (dot(normalDirection, lightDirection) < 0.0) // light source on the wrong side?
		{
			specularReflection = vec3(0.0, 0.0, 0.0); // no specular reflection
		}

		
		else // light source on the right side
		{
			specularReflection = attenuation * lights[index].specular.rgb * frontMaterial.specular.rgb
			* pow(max(0.0, dot(reflect(lightDirection, normalDirection), viewDirection)), frontMaterial.shininess);
		}
		
		
		
 
		totalLighting = totalLighting + diffuseReflection + specularReflection;
	}
 
	return vec4(totalLighting, 1.0);
}