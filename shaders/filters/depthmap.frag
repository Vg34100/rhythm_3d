__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

uniform bool isOrthographic = true;

uniform vec2 nearfar = vec2(0.01, 1000.0);

uniform float depthScale = 1.0;

//#define USE_SHADOW_SAMPLER

#if defined(USE_SHADOW_SAMPLER)
uniform sampler2DShadow depthMap;
#else
uniform sampler2D depthMap;
#endif


float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

float linearize_depth2(float d,float zNear,float zFar)
{
    float z_n = 2.0 * d - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

void main() 
{
#if defined(USE_SHADOW_SAMPLER)
    float depthValue = texture(depthMap, vec3(texCoord, 1.)).r;
#else
    float depthValue = texture(depthMap, texCoord).r;
#endif

    if (!isOrthographic) {
        depthValue = linearize_depth(depthValue, nearfar.x, nearfar.y);
    }

    depthValue *= depthScale;

	fragColor = vec4(vec3(depthValue), 1.0);

    /*
    if (depthValue < 0.01) {
        fragColor = vec4(1.0, 0.1, 0.5, 1.0);
    }
    */
    //fragColor = vec4(1.0, 0.1, 0.5, 1.0);
}