#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.141592653589793238462
#define NUMLIGHTS 2
#define RADIUS 2000

struct LightPD
{
    vec4 position;		// vec3
    vec4 direction;		// vec3
};

struct LightProps
{
    int type;			// int   0: no light   1: directional   2: point   3: spot

    vec4 ambient;		// vec3
    vec4 diffuse;		// vec3
    vec4 specular;		// vec3

    vec4 degree;		// vec3	(constant, linear, quadratic)
    vec4 cutOff;		// vec2 (cuttOff, outerCutOff)
};

struct PreCalcValues
{
	vec3 halfwayDir[NUMLIGHTS];		// Bisector of the angle viewDir-lightDir
	vec3 lightDirFrag[NUMLIGHTS];	// Light direction from lightSource to fragment
	float attenuation[NUMLIGHTS];	// How light attenuates with distance
	float intensity[NUMLIGHTS];
	
} pre;

struct uvGradient	// Gradients for the X and Y texture coordinates can be used for fetching the textures when non-uniform flow control exists (https://www.khronos.org/opengl/wiki/Sampler_(GLSL)#Non-uniform_flow_control).  
{
	vec2 uv;		// xy texture coords
	vec2 dFdx;		// Gradient of x coords
	vec2 dFdy;		// Gradient of y coords
};

layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
    vec4 time;				// float
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[27];		// sampler1D, sampler2D, sampler3D

layout(location = 0)  		in vec3 	inPos;
layout(location = 1)  flat	in vec3 	inCamPos;
layout(location = 2)  		in vec3 	inNormal;
layout(location = 3)  		in float	inSlope;
layout(location = 4)  		in float	inDist;
layout(location = 5)  flat	in float	inCamSqrHeight;
layout(location = 6)		in float	inGroundHeight;
layout(location = 7)  		in vec3	 	inTanX;
layout(location = 8)  		in vec3	 	inBTanX;
layout(location = 9)  		in vec3	 	inTanY;
layout(location = 10)  		in vec3	 	inBTanY;
layout(location = 11) 		in vec3	 	inTanZ;
layout(location = 12) 		in vec3	 	inBTanZ;
layout(location = 13) flat	in LightPD	inLight[NUMLIGHTS];

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


// Declarations:

vec3  getFragColor	  (vec3 albedo, vec3 normal, vec3 specularity, float roughness);
void  getTex		  (inout vec3 result, int albedo, int normal, int specular, int roughness, float scale, vec2 UV);
vec4  triplanarTexture(sampler2D tex, float texFactor);
vec4  triplanarTexture(sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy);
vec3  triplanarNormal (sampler2D tex, float texFactor);		// https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
vec3  triplanarNormal (sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy);
vec3  toRGB			  (vec3 vec);							// Transforms non-linear sRGB color to linear RGB. Note: Usually, input is non-linear sRGB, but it's automatically converted to linear RGB in the shader, and output later in sRGB.
vec3  toSRGB		  (vec3 vec);							// Transforms linear RGB color to non-linear sRGB
void  precalculateValues();
vec2  unpackUV		  (vec2 UV, float texFactor);
vec3  unpackNormal    (vec3 normal);
uvGradient getGradients(vec2 uvs);
vec3  applyLinearFog  (vec3 fragColor, vec3 fogColor, float minDist, float maxDist);
float applyLinearFog  (float value, float fogValue, float minDist, float maxDist);
vec3  applyFog		  (vec3 fragColor, vec3 fogColor);
float applyFog		  (float value,   float fogValue);
float getTexScaling	  (float initialTexFactor, float baseDist, float mixRange, inout float texFactor1, inout float texFactor2);
vec3  getDryColor	  (vec3 color, float minHeight, float maxHeight);
float modulus		  (float dividend, float divider);		// modulus(%) = a - (b * floor(a/b))

void getTexture_Sand(inout vec3 result);
vec3 getTexture_GrassRock();


// Definitions:

void main()
{
	precalculateValues();
	vec3 color;
	
	//getTexture_Sand(color);
	color = getTexture_GrassRock();
	
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);
	outColor = vec4(color, 1.0);
}


void getTexture_Sand(inout vec3 result)
{
    float slopeThreshold = 0.04;          // sand-plainSand slope threshold
    float mixRange       = 0.02;          // threshold mixing range (slope range)
    float tf             = 50;            // texture factor
	
	float ratio = clamp((inSlope - slopeThreshold) / (2 * mixRange), 0.f, 1.f);
		
	vec3 dunes  = getFragColor(
						triplanarTexture(texSampler[17], tf).rgb,
						normalize(toSRGB(triplanarTexture(texSampler[18], tf).rgb) * 2.f - 1.f).rgb,
						triplanarTexture(texSampler[19], tf).rgb,
						triplanarTexture(texSampler[20], tf).r * 255 );
						
	vec3 plains = getFragColor(
						triplanarTexture(texSampler[21], tf).rgb,
						normalize(toSRGB(triplanarTexture(texSampler[22], tf).rgb) * 2.f - 1.f).rgb,
						triplanarTexture(texSampler[23], tf).rgb,
						triplanarTexture(texSampler[24], tf).r * 255 );

	result = (ratio) * plains + (1-ratio) * dunes;
}

// Get ratio of snow given a slope threshold mixing range (mixRange), and a snow height range at equator (minHeight, maxHeight).
float getSnowRatio(float mixRange, float minHeight, float maxHeight)
{
	float lat    = atan(abs(inPos.z) / sqrt(inPos.x * inPos.x + inPos.y * inPos.y));
	float height = inGroundHeight - RADIUS;
	float decrement = maxHeight * (lat / (PI/2.f));				// height range decreases with latitude
	minHeight -= decrement;
	maxHeight -= decrement;
	float slopeThreshold = (height - minHeight) / (maxHeight - minHeight);	// Height ratio == Slope threshold
	
	return 1 - clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
}

// Get ratio of rock given a slope threshold (slopeThreshold) and a slope mixing range (mixRange).
float getRockRatio(float slopeThreshold, float mixRange)
{
	return clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
}

vec3 getTexture_GrassRock()
{
	// Texture resolution and Ratios.
	float tf[2];												// texture factors
	float ratioMix  = getTexScaling(15, 40, 0.2, tf[0], tf[1]);	// params: initialTexFactor, baseDist, mixRange, minHeight, maxHeight
	float ratioRock = getRockRatio(0.22, 0.02);					// params: slopeThreshold, mixRange
	float ratioSnow = getSnowRatio(0.1, 100, 140);				// params: mixRange, minHeight, maxHeight

	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snow1Par[2];
	vec3 snow2Par[2];
	
	bool getGrass = (ratioRock < 1.f || ratioSnow < 1.f);
	bool getRock  = (ratioRock > 0.f);
	bool getSnow  = (ratioSnow > 0.f);

	float lowResDist = sqrt(inCamSqrHeight - RADIUS * RADIUS);	// Distance from where low resolution starts
	if(lowResDist < 700) lowResDist = 700;						// Minimum low res. distance
	
	vec3 dryColor = getDryColor(vec3(0.9, 0.6, 0), RADIUS + 15, RADIUS + 70);

	int i;
	if(inDist > lowResDist * 1.0)
	{
		for(i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor( 
				triplanarTexture(texSampler[0], tf[i]).rgb * dryColor,
				inNormal,
				vec3(0.06, 0.06, 0.06),
				200 );
				
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i]).rgb,
				inNormal,
				vec3(0.1, 0.1, 0.1),
				125 );
		
			snow1Par[i] = getFragColor(
				triplanarTexture(texSampler[15], tf[i]).rgb,
				inNormal,
				vec3(0.2, 0.2, 0.2),
				125 );
			
			snow2Par[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i]).rgb,
				inNormal,
				vec3(0.2, 0.2, 0.2),
				125 );
		
			if(ratioMix == 1.f)
			{
				grassPar[1] = grassPar[0];
				rockPar [1] = rockPar [0];
				snow1Par [1] = snow1Par [0];
				snow2Par [1] = snow2Par [0];
				break;
			}
		}
	}
	else
	{
		for(i = 0; i < 2; i++)
		{
				grassPar[i]  = getFragColor(
					triplanarTexture(texSampler[0], tf[i]).rgb * dryColor,
					triplanarNormal (texSampler[1], tf[i]),
					triplanarTexture(texSampler[2], tf[i]).rgb,
					triplanarTexture(texSampler[3], tf[i]).r * 255 );

				rockPar[i] = getFragColor(
					triplanarTexture(texSampler[5], tf[i]).rgb,
					triplanarNormal (texSampler[6], tf[i]),
					triplanarTexture(texSampler[7], tf[i]).rgb,
					triplanarTexture(texSampler[8], tf[i]).r * 255 );

				snow1Par[i] = getFragColor(
					triplanarTexture(texSampler[15], tf[i]).rgb,
					triplanarNormal (texSampler[16], tf[i]),
					triplanarTexture(texSampler[17], tf[i]).rgb,
					triplanarTexture(texSampler[18], tf[i]).r * 255 );
					
				snow2Par[i] = getFragColor(
					triplanarTexture(texSampler[10], tf[i]).rgb,
					triplanarNormal (texSampler[11], tf[i]),
					triplanarTexture(texSampler[12], tf[i]).rgb,
					triplanarTexture(texSampler[13], tf[i]).r * 255 );
					
			if(ratioMix == 1.f)
			{
				grassPar[1] = grassPar[0];
				rockPar [1] = rockPar [0];
				snow1Par [1] = snow1Par [0];
				snow2Par [1] = snow2Par [0];
				break;
			}
		}
	}
	
	// Get material colors depending upon distance mixing resolution.
	vec3 grass = grassPar[0] * ratioMix + grassPar[1] * (1 - ratioMix);
	vec3 rock  = rockPar [0] * ratioMix + rockPar [1] * (1 - ratioMix);
	vec3 snowP = snow1Par [0] * ratioMix + snow1Par [1] * (1 - ratioMix);	// plain snow
	vec3 snowR = snow2Par [0] * ratioMix + snow2Par [1] * (1 - ratioMix);	// rough snow

	// Grass + Rock:

	vec3 result;

	if(inDist < 5) 
	{
		float grassHeight  = triplanarTexture(texSampler[ 4], tf[0]).r;
		//float grass2Height = triplanarTexture(texSampler[ 4], tf[1]).r;
		float rockHeight   = triplanarTexture(texSampler[ 9], tf[0]).r;
		//float rock2Height  = triplanarTexture(texSampler[ 9], tf[1]).r;
		//float snowHeight   = triplanarTexture(texSampler[14], tf[0]).r;
		//float snow2Height  = triplanarTexture(texSampler[14], tf[1]).r;
		
		float ma = max(rockHeight  + ratioRock, grassHeight + (1-ratioRock)) - 0.1;		// 0.1 = depth
		float b1 = max(rockHeight  + ratioRock     - ma, 0);
		float b2 = max(grassHeight + (1-ratioRock) - ma, 0);
		result = (rock * b1 + grass * b2) / (b1 + b2);
	}
	else result = rock * (ratioRock) + grass * (1-ratioRock);
	
	// Snow:
	
	vec3 snow = snowR * (ratioRock) + snowP * (1-ratioRock);
	return result * (1 - ratioSnow) + snow * (ratioSnow);
	
	//return result * (1 - ratioSnow) + snowP * (ratioSnow);
}

vec3 getTexture_GrassRock2()
{
	// Texture resolution and mixing ratio (mixes of same texture between distances).
	float tf[2];	// texture factors
	float ratio = getTexScaling(15, 40, 0.2, tf[0], tf[1]);	// initialTexFactor, baseDist, mixRange, minHeight, maxHeight
		
	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snowPar [2];

	float lowResDist = sqrt(inCamSqrHeight - RADIUS * RADIUS);	// Distance from where low resolution starts
	if(lowResDist < 750) lowResDist = 750;						// Minimum low res. distance

	if(inDist > lowResDist * 1.0)
		for(int i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor(
				triplanarTexture(texSampler[0], tf[i]).rgb,
				inNormal,
				vec3(0.06, 0.06, 0.06),
				200 );
		
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i]).rgb,
				inNormal,
				vec3(0.1, 0.1, 0.1),
				125 );
			
			snowPar[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i]).rgb,
				inNormal,
				vec3(0.2, 0.2, 0.2),
				125 );
		}
	else
		for(int i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor(
				triplanarTexture(texSampler[0], tf[i]).rgb,
				triplanarNormal (texSampler[1], tf[i]),
				triplanarTexture(texSampler[2], tf[i]).rgb,
				triplanarTexture(texSampler[3], tf[i]).r * 255 );
		
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i]).rgb,
				triplanarNormal (texSampler[6], tf[i]),
				triplanarTexture(texSampler[7], tf[i]).rgb,
				triplanarTexture(texSampler[8], tf[i]).r * 255 );
			
			snowPar[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i]).rgb,
				triplanarNormal (texSampler[11], tf[i]),
				triplanarTexture(texSampler[12], tf[i]).rgb,
				triplanarTexture(texSampler[13], tf[i]).r * 255 );
		}
	
	// Get material colors depending upon distance mixing resolution.
	vec3 grass = grassPar[0] * ratio + grassPar[1] * (1 - ratio);
	vec3 rock  = rockPar [0] * ratio + rockPar [1] * (1 - ratio);
	vec3 snow  = snowPar [0] * ratio + snowPar [1] * (1 - ratio);

	// Grass + Rock:
	ratio = getRockRatio(0.22, 0.02);	// params: slopeThreshold, mixRange
	vec3 result;

	if(inDist < 5) 
	{
		float grassHeight  = triplanarTexture(texSampler[ 4], tf[0]).r;
		float grass2Height = triplanarTexture(texSampler[ 4], tf[1]).r;
		float rockHeight   = triplanarTexture(texSampler[ 9], tf[0]).r;
		float rock2Height  = triplanarTexture(texSampler[ 9], tf[1]).r;
		float snowHeight   = triplanarTexture(texSampler[14], tf[0]).r;
		float snow2Height  = triplanarTexture(texSampler[14], tf[1]).r;
		
		float ma = max(rockHeight  + ratio, grassHeight + (1-ratio)) - 0.1;		// 0.1 = depth
		float b1 = max(rockHeight  + ratio     - ma, 0);
		float b2 = max(grassHeight + (1-ratio) - ma, 0);
		result = (rock * b1 + grass * b2) / (b1 + b2);
	}
	else result = rock * (ratio) + grass * (1-ratio);
	
	// Snow:
	ratio = getSnowRatio(0.1, 100, 140);		// params: mixRange, minHeight, maxHeight
	return result * (1 - ratio) + snow * (ratio);
}

vec3 getTexture_GrassRock_Test()
{
	// Get parameters for texture resolution
	const int levels = 7;			// Number of resolution levels (4)
	float baseDist   = 40;			// Maximum distance. Higher distances don't change texture resolution.
	float texFactor  = 10;
	float mixRange = 0.2;			// Percent of mixing distance
	
	float levelDist[levels];		// Distances from where resolution changes (0, 5, 10, 20)
	float tf[levels];				// Texture factors for each level (1, 2, 4, 8)
	uvGradient texCoord[levels][3];	// UV coords and gradients for each texture 
	
	float sqr;
	int i;
	for(i = 0; i < levels; i++)
	{
		sqr = pow(2, i);
		levelDist[i] = baseDist * sqr;
		
		tf[i] = texFactor * sqr;
		
		texCoord[i][0] = getGradients(unpackUV(inPos.zy, tf[i]));
		texCoord[i][1] = getGradients(unpackUV(inPos.xz, tf[i]));
		texCoord[i][2] = getGradients(unpackUV(inPos.xy, tf[i]));
	}
	
	for(i = levels - 1; i > 0; i--) levelDist[i] = levelDist[i-1];
	levelDist[0] = 0;
	
	int lvl = 0;
	for(i = levels - 1; i >= 0; i--)
		if(inDist > levelDist[i]) 
		{
			lvl = i;
			break;
		}
	
	// Get textures
	vec3 grass[2];
	vec3 rock [2];
	vec3 snow [2];
			
	for(i = 0; i < 2; i++)
	{
		snow[i] = getFragColor(
			triplanarTexture(texSampler[10], texCoord[lvl+i][0], texCoord[lvl+i][1], texCoord[lvl+i][2]).rgb,
			triplanarNormal (texSampler[11], texCoord[lvl+i][0], texCoord[lvl+i][1], texCoord[lvl+i][2]),
			triplanarTexture(texSampler[12], texCoord[lvl+i][0], texCoord[lvl+i][1], texCoord[lvl+i][2]).rgb,
			triplanarTexture(texSampler[13], texCoord[lvl+i][0], texCoord[lvl+i][1], texCoord[lvl+i][2]).r * 255 );
			
		if((lvl+1) == levels) 
		{ 
			snow[1] = snow[0]; 
			break; 
		};
	}
	
	float ratio = 1;
	if(lvl != (levels-1))
		ratio = 1 - clamp((inDist - ((1 - mixRange) * levelDist[lvl + 1])) / (mixRange * levelDist[lvl + 1]), 0, 1); 

	return snow[0] * ratio + snow[1] * (1 - ratio);
}

vec3 getTexture_GrassRock_Original(vec3 result)
{
	float tf[2];	// texture factors
	float ratio = getTexScaling(10, 40, 0.1, tf[0], tf[1]);	// initialTexFactor, baseDist, mixRange, resultingTFs[2]

	vec3 grass;		vec3 grass2;
	vec3 rock;		vec3 rock2;
	vec3 snow;		vec3 snow2;
	float grassHeight = 0;	float grass2Height = 0;
	float rockHeight  = 0;	float rock2Height  = 0;
	float snowHeight  = 0;	float snow2Height  = 0;

	float lowResDist = inCamSqrHeight * inCamSqrHeight * 0.000000000018;	// (h^4 + b) Distance from where low resolution starts
	
	if(inDist > lowResDist * 1.2)
	{
		grass  = getFragColor(
					triplanarTexture(texSampler[0], tf[0]).rgb,
					inNormal,
					vec3(0.06, 0.06, 0.06),
					200 );
		
		grass2  = getFragColor(
					triplanarTexture(texSampler[0], tf[1]).rgb,
					inNormal,
					vec3(0.06, 0.06, 0.06),
					200 );
			
		rock = getFragColor(
					triplanarTexture(texSampler[5], tf[0]).rgb,
					inNormal,
					vec3(0.7, 0.7, 0.7),
					125 );
				
		rock2 = getFragColor(
					triplanarTexture(texSampler[5], tf[1]).rgb,
					inNormal,
					vec3(0.7, 0.7, 0.7),
					125 );
		
		snow = getFragColor(
					triplanarTexture(texSampler[10], tf[0]).rgb,
					inNormal,
					vec3(1,1,1),
					125 );
						
		snow2 = getFragColor(
					triplanarTexture(texSampler[10], tf[1]).rgb,
					inNormal,
					vec3(1,1,1),
					125 );
	}
	else if(inDist > lowResDist)
	{
		float ratio = clamp((inDist - lowResDist) / (lowResDist * 0.2), 0, 1);
		
		grass  = 
			ratio * getFragColor(
							triplanarTexture(texSampler[0], tf[0]).rgb,
							inNormal,
							vec3(0.06, 0.06, 0.06),
							200 ) +
			(1 - ratio) * getFragColor(
							triplanarTexture(texSampler[0], tf[0]).rgb,
							triplanarNormal (texSampler[1], tf[0]),
							triplanarTexture(texSampler[2], tf[0]).rgb,
							triplanarTexture(texSampler[3], tf[0]).r * 255 );
		
		grass2  = 
			ratio * getFragColor(
							triplanarTexture(texSampler[0], tf[1]).rgb,
							inNormal,
							vec3(0.06, 0.06, 0.06),
							200 ) +
			(1 - ratio) * getFragColor(
							triplanarTexture(texSampler[0], tf[1]).rgb,
							triplanarNormal (texSampler[1], tf[1]),
							triplanarTexture(texSampler[2], tf[1]).rgb,
							triplanarTexture(texSampler[3], tf[1]).r * 255 );

		rock = 
			ratio * getFragColor(
							triplanarTexture(texSampler[5], tf[0]).rgb,
							inNormal,
							vec3(0.7, 0.7, 0.7),
							125 ) +
			(1 - ratio) * getFragColor(
							triplanarTexture(texSampler[5], tf[0]).rgb,
							triplanarNormal (texSampler[6], tf[0]),
							triplanarTexture(texSampler[7], tf[0]).rgb,
							triplanarTexture(texSampler[8], tf[0]).r * 255 );
		
		rock2 = 
			ratio * getFragColor(
							triplanarTexture(texSampler[5], tf[1]).rgb,
							inNormal,
							vec3(0.7, 0.7, 0.7),
							125 ) +
			(1 - ratio) * getFragColor(
							triplanarTexture(texSampler[5], tf[1]).rgb,
							triplanarNormal (texSampler[6], tf[1]),
							triplanarTexture(texSampler[7], tf[1]).rgb,
							triplanarTexture(texSampler[8], tf[1]).r * 255 );
							
		snow = 
			ratio * getFragColor(
							triplanarTexture(texSampler[10], tf[0]).rgb,
							inNormal,
							vec3(0.8, 0.8, 0.8),
							125 ) +		
			(1 - ratio) * getFragColor(
							triplanarTexture(texSampler[10], tf[0]).rgb,
							triplanarNormal (texSampler[11], tf[0]),
							triplanarTexture(texSampler[12], tf[0]).rgb,
							triplanarTexture(texSampler[13], tf[0]).r * 255 );
							
		snow2 = 
			ratio * getFragColor(
							triplanarTexture(texSampler[10], tf[1]).rgb,
							inNormal,
							vec3(0.8, 0.8, 0.8),
							125 ) +	
			(1 - ratio) * getFragColor(
							triplanarTexture(texSampler[10], tf[1]).rgb,
							triplanarNormal (texSampler[11], tf[1]),
							triplanarTexture(texSampler[12], tf[1]).rgb,
							triplanarTexture(texSampler[13], tf[1]).r * 255 );
	}
	else
	{
		grass  = getFragColor(
					triplanarTexture(texSampler[0], tf[0]).rgb,
					triplanarNormal (texSampler[1], tf[0]),
					triplanarTexture(texSampler[2], tf[0]).rgb,
					triplanarTexture(texSampler[3], tf[0]).r * 255 );
		
		grass2  = getFragColor(
					triplanarTexture(texSampler[0], tf[1]).rgb,
					triplanarNormal (texSampler[1], tf[1]),
					triplanarTexture(texSampler[2], tf[1]).rgb,
					triplanarTexture(texSampler[3], tf[1]).r * 255 );
	
		rock = getFragColor(
					triplanarTexture(texSampler[5], tf[0]).rgb,
					triplanarNormal (texSampler[6], tf[0]),
					triplanarTexture(texSampler[7], tf[0]).rgb,
					triplanarTexture(texSampler[8], tf[0]).r * 255 );
				
		rock2 = getFragColor(
					triplanarTexture(texSampler[5], tf[1]).rgb,
					triplanarNormal (texSampler[6], tf[1]),
					triplanarTexture(texSampler[7], tf[1]).rgb,
					triplanarTexture(texSampler[8], tf[1]).r * 255 );
		
		snow = getFragColor(
					triplanarTexture(texSampler[10], tf[0]).rgb,
					triplanarNormal (texSampler[11], tf[0]),
					triplanarTexture(texSampler[12], tf[0]).rgb,
					triplanarTexture(texSampler[13], tf[0]).r * 255 );
						
		snow2 = getFragColor(
					triplanarTexture(texSampler[10], tf[1]).rgb,
					triplanarNormal (texSampler[11], tf[1]),
					triplanarTexture(texSampler[12], tf[1]).rgb,
					triplanarTexture(texSampler[13], tf[1]).r * 255 );	
	
		if(inDist < 5)
		{
			grassHeight  = triplanarTexture(texSampler[ 4], tf[0]).r;
			grass2Height = triplanarTexture(texSampler[ 4], tf[1]).r;
			rockHeight   = triplanarTexture(texSampler[ 9], tf[0]).r;
			rock2Height  = triplanarTexture(texSampler[ 9], tf[1]).r;
			snowHeight   = triplanarTexture(texSampler[14], tf[0]).r;
			snow2Height  = triplanarTexture(texSampler[14], tf[1]).r;
		}
	}

	grass = (ratio) * grass + (1-ratio) * grass2;
	rock  = (ratio) * rock  + (1-ratio) * rock2;
	snow  = (ratio) * snow  + (1-ratio) * snow2;	// <<< BUG: Artifact lines between textures of different scale. Possible cause: Textures are get with non-constant tf values, which determine the texture scale. Possible solutions: (1) Not using mipmaps (and maybe AntiAliasing & Anisotropic filthering); (2) Getting all textures of all scales that are being used; (3) Maybe using dFdx() & dFdy() properly (https://www.khronos.org/opengl/wiki/Sampler_(GLSL)#Non-uniform_flow_control). See more in: https://community.khronos.org/t/artifact-in-the-limit-between-textures/109162

	// Grass + Rock:

	float slopeThreshold = 0.22;          // grass-rock slope threshold
    float mixRange       = 0.02;          // threshold mixing range (slope range)
	
	ratio = clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
	//result = rock * (ratio) + grass * (1-ratio);
	
	if(inDist < 5) 
	{
		float ma = max(rockHeight  + ratio, grassHeight + (1-ratio)) - 0.1;		// 0.1 = depth
		float b1 = max(rockHeight  + ratio     - ma, 0);
		float b2 = max(grassHeight + (1-ratio) - ma, 0);
		result = (rock * b1 + grass * b2) / (b1 + b2);
	}
	else result = rock * (ratio) + grass * (1-ratio);
	
	// Snow:

	mixRange = 0.1;	// slope threshold mixing range

	//		as function of slope
	float lat    = atan(abs(inPos.z) / sqrt(inPos.x * inPos.x + inPos.y * inPos.y));
	float minLat   = PI/3;	// Minimum latitude where snow appears
	slopeThreshold = (lat - minLat) / ((PI/2) - minLat);	// Latitude ratio == Slope threshold
	float ratio_1 = clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);

	//		as function of height
	float heightRange[2] = { 100, 150 };						// height range at equator
	float height = inGroundHeight - RADIUS;
	float decrement = heightRange[1] * (lat / (PI/2.f));	// height range decreases with latitude
	heightRange[0] -= decrement;
	heightRange[1] -= decrement;
	slopeThreshold = (height - heightRange[0]) / (heightRange[1] - heightRange[0]);	// Height ratio == Slope threshold
	float ratio_2 = clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);

	ratio = min(ratio_1, ratio_2);
	return result * (ratio) + snow * (1 - ratio);
}


// Tools ---------------------------------------------------------------------------------------------

vec3 directionalLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
	// ----- Ambient lighting -----
	vec3 ambient = ubo.light[i].ambient.xyz * albedo;
	if(dot(inLight[i].direction.xyz, normal) > 0) return ambient;			// If light comes from below the tangent plane
	
	// ----- Diffuse lighting -----
	float diff   = max(dot(normal, -inLight[i].direction.xyz), 0.f);
	vec3 diffuse = ubo.light[i].diffuse.xyz * albedo * diff;		
	
	// ----- Specular lighting -----
	float spec		= pow(max(dot(normal, pre.halfwayDir[i]), 0.0), roughness * 4);
	vec3 specular	= ubo.light[i].specular.xyz * specularity * spec;
	
	// ----- Result -----
	return vec3(ambient + diffuse + specular);
}


vec3 PointLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    // ----- Ambient lighting -----
    vec3 ambient = ubo.light[i].ambient.xyz * albedo * pre.attenuation[i];
	if(dot(pre.lightDirFrag[i], normal) > 0) return ambient;				// If light comes from below the tangent plane

    // ----- Diffuse lighting -----
    float diff   = max(dot(normal, -pre.lightDirFrag[i]), 0.f);
    vec3 diffuse = ubo.light[i].diffuse.xyz * albedo * diff * pre.attenuation[i];
	
    // ----- Specular lighting -----
	float spec        = pow(max(dot(normal, pre.halfwayDir[i]), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec * pre.attenuation[i];
	
    // ----- Result -----
    return vec3(ambient + diffuse + specular);	
}


vec3 SpotLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    // ----- Ambient lighting -----
    vec3 ambient = ubo.light[i].ambient.xyz * albedo * pre.attenuation[i];
	if(dot(pre.lightDirFrag[i], normal) > 0) return ambient;				// If light comes from below the tangent plane

    // ----- Diffuse lighting -----
	float diff      = max(dot(normal, -pre.lightDirFrag[i]), 0.f);
    vec3 diffuse    = ubo.light[i].diffuse.xyz * albedo * diff * pre.attenuation[i] * pre.intensity[i];

    // ----- Specular lighting -----
	float spec        = pow(max(dot(normal, pre.halfwayDir[i]), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec * pre.attenuation[i] * pre.intensity[i];
	
    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}

// Apply the lighting type you want to a fragment
vec3 getFragColor(vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
	//albedo      = applyLinearFog(albedo, vec3(.1,.1,.1), 100, 500);
	//specularity = applyLinearFog(specularity, vec3(0,0,0), 100, 500);
	//roughness   = applyLinearFog(roughness, 0, 100, 500);

	vec3 result = vec3(0,0,0);

	for(int i = 0; i < NUMLIGHTS; i++)		// for each light source
	{
		if(ubo.light[i].type == 1)
			result += directionalLightColor	(i, albedo, normal, specularity, roughness);
		else if(ubo.light[i].type == 2)
			result += PointLightColor		(i, albedo, normal, specularity, roughness);
		else if(ubo.light[i].type == 3)
			result += SpotLightColor		(i, albedo, normal, specularity, roughness);
	}
	
	return result;
}

void getTex(inout vec3 result, int albedo, int normal, int specular, int roughness, float scale, vec2 UV)
{
	result = getFragColor(
				texture(texSampler[albedo], UV/scale).rgb,
				normalize(toSRGB(texture(texSampler[normal], UV/scale).rgb) * 2.f - 1.f).rgb,
				texture(texSampler[specular], UV/scale).rgb, 
				texture(texSampler[roughness], UV/scale).r * 255 );
}

void precalculateValues()
{
	vec3 viewDir = normalize(inCamPos - inPos);	// Camera view direction
	float distFragLight;						// Distance fragment-lightSource
	float theta;								// The closer to 1, the more direct the light gets to fragment.
	float epsilon;								// Cutoff range
	int type;									// Light type
	
	for(int i = 0; i < NUMLIGHTS; i++)
	{
		type = ubo.light[i].type;
		if(type == 1)
		{
			pre.halfwayDir[i]	= normalize(-inLight[i].direction.xyz + viewDir);
		}
		else if(type == 2)
		{
			distFragLight		= length(inLight[i].position.xyz - inPos);
			pre.lightDirFrag[i]	= normalize(inPos - inLight[i].position.xyz);
			pre.halfwayDir[i]   = normalize(-pre.lightDirFrag[i] + viewDir);
			pre.attenuation[i]  = 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distFragLight + ubo.light[i].degree[2] * distFragLight * distFragLight);
		}
		else if(type == 3)
		{
			distFragLight		= length(inLight[i].position.xyz - inPos);
			pre.lightDirFrag[i]	= normalize(inPos - inLight[i].position.xyz);
			pre.halfwayDir[i]   = normalize(-pre.lightDirFrag[i] + viewDir);
			pre.attenuation[i]	= 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distFragLight + ubo.light[i].degree[2] * distFragLight * distFragLight);
			theta				= dot(pre.lightDirFrag[i], inLight[i].direction.xyz);
			epsilon				= ubo.light[i].cutOff[0] - ubo.light[i].cutOff[1];
			pre.intensity[i]	= clamp((theta - ubo.light[i].cutOff[1]) / epsilon, 0.0, 1.0);
		}
	}
}

vec2 unpackUV(vec2 UV, float texFactor)
{
	return UV.xy * vec2(1, -1) / texFactor;
}

vec4 triplanarTexture(sampler2D tex, float texFactor)
{
	vec4 dx = texture(tex, unpackUV(inPos.zy, texFactor));
	vec4 dy = texture(tex, unpackUV(inPos.xz, texFactor));
	vec4 dz = texture(tex, unpackUV(inPos.xy, texFactor));
	
	vec3 weights = abs(normalize(inNormal));
	weights *= weights;
	weights = weights / (weights.x + weights.y + weights.z);

	return dx * weights.x + dy * weights.y + dz * weights.z;
}

vec4 triplanarTexture(sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy)
{
	vec4 dx = textureGrad(tex, dzy.uv, dzy.dFdx, dzy.dFdy);
	vec4 dy = textureGrad(tex, dxz.uv, dxz.dFdx, dxz.dFdy);
	vec4 dz = textureGrad(tex, dxy.uv, dxy.dFdx, dxy.dFdy);
	
	vec3 weights = abs(normalize(inNormal));
	weights *= weights;
	weights = weights / (weights.x + weights.y + weights.z);

	return dx * weights.x + dy * weights.y + dz * weights.z;
}

vec3 unpackNormal(vec3 normal)
{
	return normalize(toSRGB(normal) * 2.f - 1.f);
}

vec3 triplanarNormal(sampler2D tex, float texFactor)
{	
	// Tangent space normal maps (retrieved using triplanar UVs; i.e., 3 facing planes)
	vec3 tnormalX = unpackNormal(texture(tex, unpackUV(inPos.zy, texFactor)).xyz);
	vec3 tnormalY = unpackNormal(texture(tex, unpackUV(inPos.xz, texFactor)).xyz);
	vec3 tnormalZ = unpackNormal(texture(tex, unpackUV(inPos.xy, texFactor)).xyz);
	
	// Fix X plane projection over positive X axis
	vec3 axis = sign(inNormal);
	tnormalX.x *= -axis.x;	
	
	// World space normals
	tnormalX = mat3(inTanX, inBTanX, inNormal) * tnormalX;	// TBN_X * tnormalX
	tnormalY = mat3(inTanY, inBTanY, inNormal) * tnormalY;	// TBN_Y * tnormalY
	tnormalZ = mat3(inTanZ, inBTanZ, inNormal) * tnormalZ;	// TBN_Z * tnormalZ
	
	// Weighted average
	vec3 weights = abs(inNormal);
	weights *= weights;
	weights /= weights.x + weights.y + weights.z;
		
	return normalize(tnormalX * weights.x  +  tnormalY * weights.y  +  tnormalZ * weights.z);
}

vec3 triplanarNormal(sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy)
{	
	// Tangent space normal maps (retrieved using triplanar UVs; i.e., 3 facing planes)
	vec3 tnormalX = unpackNormal(texture(tex, dzy.uv).xyz);
	vec3 tnormalY = unpackNormal(texture(tex, dxz.uv).xyz);
	vec3 tnormalZ = unpackNormal(texture(tex, dxy.uv).xyz);
	
	// Fix X plane projection over positive X axis
	vec3 axis = sign(inNormal);
	tnormalX.x *= -axis.x;	
	
	// World space normals
	tnormalX = mat3(inTanX, inBTanX, inNormal) * tnormalX;	// TBN_X * tnormalX
	tnormalY = mat3(inTanY, inBTanY, inNormal) * tnormalY;	// TBN_Y * tnormalY
	tnormalZ = mat3(inTanZ, inBTanZ, inNormal) * tnormalZ;	// TBN_Z * tnormalZ
	
	// Weighted average
	vec3 weights = abs(inNormal);
	weights *= weights;
	weights /= weights.x + weights.y + weights.z;
		
	return normalize(tnormalX * weights.x  +  tnormalY * weights.y  +  tnormalZ * weights.z);
}

vec3 toRGB(vec3 vec)
{
	vec3 linear;
	
	if (vec.x <= 0.04045) linear.x = vec.x / 12.92;
	else linear.x = pow((vec.x + 0.055) / 1.055, 2.4);
	
	if (vec.y <= 0.04045) linear.y = vec.y / 12.92;
	else linear.y = pow((vec.y + 0.055) / 1.055, 2.4);
	
	if (vec.z <= 0.04045) linear.z = vec.z / 12.92;
	else linear.z = pow((vec.z + 0.055) / 1.055, 2.4);
	
	return linear;
}

vec3 toSRGB(vec3 vec)
{
	vec3 nonLinear;
	
	if (vec.x <= 0.0031308) nonLinear.x = vec.x * 12.92;
	else nonLinear.x = 1.055 * pow(vec.x, 1.0/2.4) - 0.055;
	
	if (vec.y <= 0.0031308) nonLinear.y = vec.y * 12.92;
	else nonLinear.y = 1.055 * pow(vec.y, 1.0/2.4) - 0.055;
	
	if (vec.z <= 0.0031308) nonLinear.z = vec.z * 12.92;
	else nonLinear.z = 1.055 * pow(vec.z, 1.0/2.4) - 0.055;
	
	return nonLinear;
}

uvGradient getGradients(vec2 uvs)
{
	uvGradient result;
	result.uv = uvs;
	result.dFdx = dFdx(uvs);
	result.dFdy = dFdy(uvs);
	return result;
}

vec3 applyLinearFog(vec3 fragColor, vec3 fogColor, float minDist, float maxDist)
{
	float minSqrRadius = minDist * minDist;
	float maxSqrRadius = maxDist * maxDist;
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    if(sqrDist > maxSqrRadius) return fogColor;
    else
    {
        float ratio  = (sqrDist - minSqrRadius) / (maxSqrRadius - minSqrRadius);
        return fragColor * (1-ratio) + fogColor * ratio;
    }
}

float applyLinearFog(float value, float fogValue, float minDist, float maxDist)
{
	float minSqrRadius = minDist * minDist;
	float maxSqrRadius = maxDist * maxDist;
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    if(sqrDist > maxSqrRadius) return fogValue;
    else
    {
        float ratio  = (sqrDist - minSqrRadius) / (maxSqrRadius - minSqrRadius);
        return value * (1-ratio) + fogValue * ratio;
    }
}

vec3 applyFog(vec3 fragColor, vec3 fogColor)
{
	float coeff[3] = { 1, 0.000000000001, 0.000000000001 };		// coefficients  ->  a + b*dist + c*dist^2
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	
	float attenuation = 1.0 / (coeff[0] + coeff[1] * sqrDist + coeff[2] * sqrDist * sqrDist);
	return fragColor * attenuation + fogColor * (1. - attenuation);
}

float applyFog(float value, float fogValue)
{
	float coeff[3] = { 1, 0.000000000001, 0.000000000001 };		// coefficients  ->  a + b*dist + c*dist^2
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	
	float attenuation = 1.0 / (coeff[0] + coeff[1] * sqrDist + coeff[2] * sqrDist * sqrDist);
	return value * attenuation + fogValue * (1. - attenuation);
}

float getTexScaling(float initialTexFactor, float baseDist, float mixRange, inout float texFactor1, inout float texFactor2)
{
	// Compute current and next step
	float linearStep = 1 + floor(inDist / baseDist);	// Linear step [1, inf)
	float quadraticStep = ceil(log(linearStep) / log(2));
	float step[2];
	step[0] = pow (2, quadraticStep);					// Exponential step [0, inf)
	step[1] = pow(2, quadraticStep + 1);				// Next exponential step
	
	// Get texture resolution for each section
	texFactor1 = step[0] * initialTexFactor;
	texFactor2 = step[1] * initialTexFactor;
	
	// Get mixing ratio
	float maxDist = baseDist * step[0];
	mixRange = mixRange * maxDist;						// mixRange is now an absolute value (not percentage)
	return clamp((maxDist - inDist) / mixRange, 0.f, 1.f);
}

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((inGroundHeight - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}

float modulus(float dividend, float divider) { return dividend - (divider * floor(dividend/divider)); }
