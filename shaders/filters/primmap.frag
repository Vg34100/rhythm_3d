__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D primMap;


void main() 
{
    //float depthValue = texture(depthMap, vec3(texCoord, 0.)).r;
    vec4 primVal = texture(primMap, texCoord);

	fragColor = vec4(primVal.xyz, 1.0);

}