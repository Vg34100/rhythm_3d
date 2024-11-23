__VERSION__

__UNIFORMS__

in vec2 texCoord;
out vec4 fragColor;

int numRows = 20;
int numCols = 20;

uniform float timeOffset = 0.0;

uniform float period = 2.0;

#define halfperiod (period * 0.5)
#define dt (mod(iTime + timeOffset, period))
#define dth (mod(iTime + timeOffset, halfperiod))

#define moveFlip (int(mod(iTime + timeOffset, period * 2.0) > period))
#define moveRows (int(mod(iTime + timeOffset, period) > halfperiod))
#define moveEvens (int(mod(iTime + timeOffset, period) > halfperiod))

uniform float brightness = 1.0;

uniform vec2 testV2 = vec2(0.5, 0.5);

uniform mat2 testM2 = mat2(1.0, 0.0, 0.0, 1.0);

void main() 
{
    vec2 fragCoord = testM2 * gl_FragCoord.xy;

    int move_evens = moveEvens;
	if (moveFlip > 0)
	{
		move_evens = 1 - moveEvens;
	}

	vec2 iResolution = vec2(resolution);
	
	float dx = iResolution.x / float(numRows);
	float dy = iResolution.y / float(numCols);
	
	float x = fragCoord.x;
	float y = fragCoord.y;
	
	x *= iResolution.x/iResolution.y;
	
	
	int rowx = int(x / dx);
	int rowy = int(y / dy);
	
	int xeven = int(mod(float(rowx), 2.0));
	int yeven = int(mod(float(rowy), 2.0));
	
	float t = dth / halfperiod;
	
	float dtx = dx * t * 2.0;
	float dty = dy * t * 2.0;
	
	if (moveEvens == 1)
	{
		if (moveRows == 1 && xeven == 1)
		{
			y = y + dty;
			rowy = int(y / dy);
		}
		else if (moveRows == 0 && yeven == 0)
		{
			x = x + dtx;
			rowx = int(x / dx);
		}
	}
	else
	{
		if (moveRows == 1 && xeven == 0)
		{
			y = y + dty;
			rowy = int(y / dy);
		}
		else if (moveRows == 0 && yeven == 1)
		{
			x = x + dtx;
			rowx = int(x / dx);
		}
	}

	xeven = int(mod(float(rowx), 2.0));
	yeven = int(mod(float(rowy), 2.0));

	vec4 c = vec4(1.);
		
	if (c.x == 0.0 && c.y == 0.0 && c.z == 0.0)
	{
		c = vec4(1.0, 1.0, 1.0, 1.0);
	}

	c = c * brightness * (testV2.x + testV2.y);

	vec4 texColor = texture(filterInput, texCoord);
	if (length(texColor) > 0.) {
		c *= texColor;
	}

	if (xeven == yeven)
	{
		//vec4 c = texture(iChannel0, fragCoord.xy / resolution.xy);
		fragColor = c;
	}
	else
	{
		fragColor = vec4(vec3(1.) - c.rgb, 1.);
	}
	
}
