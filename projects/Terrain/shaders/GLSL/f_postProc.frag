#version 450
#extension GL_ARB_separate_shader_objects : enable

#define CENTER vec3(0,0,0)
#define RADIUS 2100
/*
	Requirements:
		- Main framebuffer
		- Z map
		- camPos
*/
layout(set = 0, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2D inputColor;

layout(location = 0) in vec2 inUVcoord;
layout(location = 1) flat in float inAspRatio;
layout(location = 2) flat in vec3 inCamPos;
layout(location = 3) flat in vec3 inCamDir;
layout(location = 4) in vec2 inNDC;

layout(location = 0) out vec4 outColor;

void main()
{
	//vec2 NDCs = { gl_FragCoord.x / 960, gl_FragCoord.y / 540 };	// Get fragment Normalize Device Coordinates (NDC) [0,1] from its Window Coordinates (pixels)
	
	// https://www.reddit.com/r/vulkan/comments/mf1rb5/input_attachment_not_accessible_from_fragment/
	// https://stackoverflow.com/questions/45154213/is-there-a-way-to-address-subpassinput-in-vulkan
	// ChatGPT: The inputAttachment function is only available in GLSL shaders if the "GL_NV_shader_framebuffer_fetch" extension is enabled. This extension is not part of the core Vulkan specification and may not be available on all devices.
	//inputAttachment(0, vec2(NDCs.x, NDCs.y));
	//subpassLoad(inputAttachment, vec2(NDCs.x, NDCs.y));
	
	outColor = texture(inputColor, inUVcoord);
}


/*
float densityAtPoint(vec3 densitySamplePoint)
{
	float heightAboveSurface = length(densitySamplePoint - planetCentre) - planetRadius;
	float height01 = heightAboveSurface / (atmosphereRadius - planetRadius);
	float localDensity = exp(-height01 * densityFalloff) * (1 - height01);
	return localDensity;
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength)
{
	vec3 densitySamplePoint = rayOrigin;
	float setpSize = rayLength / (numOpticalDepthPoints - 1);
	float opticalDepth = 0;
	
	for(int i = 0; i < numOpticalDepthPoints; i++)
	{
		float localDensity = densityAtPoint(densitySamplePoint);
		opticalDepth += localDensity * stepSize;
		densitySamplePoint += rayDir * stepSize;
	}
	return opticalDepth;
}

float calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength)
{
	vec3 inScatterPoint = rayOrigin;
	float stepSize = rayLength / (numInScatteringPoints - 1);
	float inScatteredLight = 0;
	
	for(int i = 0; i < numInScatteringPoints; i++)
	{
		float sunRayLength = raySphere(dirToSun).y;
		float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);
		float viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
		float transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth));
		float localDensity = densityAtPoint(inScatterPoint);
		
		inScatteredLight += localDensity * transmittance * stepSize;
		inScatterPoint += rayDir * stepSize;
	}
	return inScatteredLight;
}

vec4 frag()	// v2f i
{
	vec4 originalCol = vec4(0,0,0,1);
	//vec4 originalCol = texture(_MainTex, uvCoords);
	//float sceneDepthNonLinear = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uvCoords);
	//float sceneDepth = LinearEyeDepth(sceneDepthNonLinear) * length(viewVec);
	
	vec3 rayOrigin = inCamPos;
	vec3 rayDir = normalize(viewVec);
	
		//float dstToOcean = raySphere(planetCentre, oceanRadius, rayOrigin, rayDir);
		//float dstToSurface = min(sceneDepth, dstToOcean);
	//float dstToSurface = sceneDepth;
	
	vec2 hitInfo = raySphere(planetCentre, atmosphereRadius, rayOrigin, rayDir);
	float dstToAtmosphere = hitInfo.x;
	//float dstThroughAtmosphere = min(hitInfo.y, dstToSurface - dstToAtmosphere);
	float dstThroughAtmosphere = hitInfo.y;
	
	return dstThroughAtmosphere / (2 * atmosphereRadius);
}
	

// Return vector (dstToSphere, dstThroughSphere)
// If ray origin is inside sphere, dstToSphere = 0
// If ray misses sphere, dstToSphere = maxValue; dstThroughSphere = 0
vec2 raySphere(vec3 sphereCentre, float sphereRadius, vec3 rayOrigin, vec3 rayDir)
{
	float offset = rayOrigin - sphereCentre;
	float a = 1; 						// Set to dot(rayDir, rayDir) if rayDir might not be normalized
	float b = 2 * dot(offset, rayDir);
	float c = dot(offset, offset) - sphereRadius * sphereRadius;
	float d = b * b - 4 * a * c;		// Discriminant from quadratic formula
	
	// Number of intersections (0 when d < 0) (1 when d = 0) (2 when d > 0)
	if(d > 0)
	{
		float s = sqrt(d);
		float dstToSphereNear = max(0, (-b - s) / (2 * a));
		float dstToSphereFar = (-b + s) / (2 * a);
		
		// Ignore intersections that occur behind the ray
		if(dstToSphereFar >= 0)
			return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
	}
	
	return vec2(maxFloat, 0);	// Ray didn't intersect sphere
}
*/

/*
	Sean O'Neil (2005) Accurate atmospheric scattering. GPU gems 2
		
	- Sun light comes into atmosphere, scatters at some point, and is reflected toward the camera.
	- Two rays that join: camera ray to some atmospheric point, and sun ray to that atmospheric point.
	- Take camera ray through the atmosphere > Get some points across the ray > Calculate scattering on each point.
	- Along both rays, the atmosphere scatters some light away.
	- Scattering types:
		- Rayleigh: Light scatters on small molecules more light from shorter wavelengths (blue < green < red)
		- Mie: Light scatters on larger particles and scatters all wavelengths of light equally.
	- Functions:
		- Phase function: How much light is scattered toward the direction of the camera based on the angle both rays.
		- Out-scattering equation: Inner integral. Scattering through the camera ray.
		- In- scattering equation: How much light is added to a ray due to light scattering from sun.
		- Surface-scattering equation: Scattered light reflected from a surface
*/