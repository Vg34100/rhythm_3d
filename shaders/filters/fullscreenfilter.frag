__VERSION__

#define _GL_RGBA32F 0x8814
#define _GL_RGBA32I 0x8D82
#define _GL_R32I 0x8235

uniform bool displayDepth = false;
uniform int format = _GL_RGBA32F;
layout(binding = 0) uniform sampler2D texA;
layout(binding = 1) uniform sampler2D depthTexA;
layout(binding = 2) uniform isampler2D itexA;

in vec2 texCoord;

out vec4 fragColor;

uniform float zNear = 0.001;
uniform float zFar = 1000000.0;
uniform bool isCameraOrtho = false;

float getLinearDepth(float z)
{
	if (isCameraOrtho)
	{
		return z;
	}

	else
	{	
		//float z_n = 2.0 * z - 1.0;
		//float z_n = z;
		//return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));

		//return (0.1 * (gl_FragCoord.z / gl_FragCoord.w) / (zFar - zNear));

		return (2.0 * zNear) / (zFar + zNear - z * (zFar - zNear));
	}
}

void main() 
{
	if (displayDepth)
	{
		float depth = texture2D(depthTexA, texCoord).x;
		//depth = getLinearDepth(depth);
		fragColor = texture2D(texA, texCoord);
		//fragColor = vec4(vec3(depth), 1.0);
		fragColor.rgb += vec3(depth);
		//fragColor = vec4(vec3(fragColor.x), 1.0);
	}

	else if (format == _GL_RGBA32F)
	{
		fragColor = texture2D(texA, texCoord);
	}
	else
	{
		if (format == _GL_RGBA32I)
		{
			ivec4 v = texture(itexA, texCoord);

			if (v.yz != ivec2(0))
			{
				float col = float(v.x) / float(v.w);

				fragColor = vec4(vec3(col), 1.0);
			}
			else
			{
				fragColor = vec4(0);
			}
		}
		else
		{
			int v = texture(itexA, texCoord).r;

			if (v != 0)
			{
				fragColor = vec4(v);
			}
			else
			{
				fragColor = vec4(0.0);
			}
		}
	}
}
