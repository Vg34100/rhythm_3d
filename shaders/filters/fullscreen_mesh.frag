__VERSION__

uniform sampler2D DefaultSampler;
uniform sampler2D RotationSampler;
uniform sampler2D NormalSampler;
uniform sampler2D PositionSampler;
uniform isampler2D PrimitiveDataSampler;
uniform sampler2D EdgeColorSampler;
uniform isampler2D EdgeVerticesSampler;
uniform sampler2D DepthSampler;

in vec2 texCoord;

/* layout (location = 0) */ out vec4 DefaultOutput;
/* layout (location = 1) */ out vec4 RotationOutput;
/* layout (location = 2) */ out vec4 NormalOutput;
/* layout (location = 3) */ out vec4 PositionOutput;
/* layout (location = 4) */ out ivec4 PrimitiveDataOutput;
/* layout (location = 5) */ out vec4 EdgeColorOutput;
/* layout (location = 6) */ out ivec4 EdgeVerticesOutput;
/* layout (location = 7) */ out float DepthOutput;

void main() 
{
	DefaultOutput = texture2D(DefaultSampler, texCoord);
	RotationOutput = texture2D(RotationSampler, texCoord);
	NormalOutput = texture2D(NormalSampler, texCoord);
	PositionOutput = texture2D(PositionSampler, texCoord);
	PrimitiveDataOutput = texture(PrimitiveDataSampler, texCoord);
	EdgeColorOutput = texture2D(EdgeColorSampler, texCoord);
	EdgeVerticesOutput = texture(EdgeVerticesSampler, texCoord);
	DepthOutput = texture2D(DepthSampler, texCoord).x;

	gl_FragDepth = DepthOutput.x;
}