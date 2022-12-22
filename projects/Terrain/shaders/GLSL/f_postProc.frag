#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PLANET_CENTER vec3(0,0,0)
#define PLANET_RADIUS 1400
#define OCEAN_RADIUS 1			
#define ATM_RADIUS 2450
#define NUM_SCATT_POINTS 10			// number of scattering points
#define NUM_OPT_DEPTH_POINTS 10		// number of optical depth points
#define DENSITY_FALLOFF 10
#define SCATT_STRENGTH 10			// scattering strength
#define WAVELENGTHS vec3(700, 530, 440)
#define SCATT_COEFFICIENTS vec3(pow(400/WAVELENGTHS.x, 4)*SCATT_STRENGTH, pow(400/WAVELENGTHS.y, 4)*SCATT_STRENGTH, pow(400/WAVELENGTHS.z, 4)*SCATT_STRENGTH)

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define DBL_MAX 1.7976931348623158e+308
#define DBL_MIN 2.2250738585072014e-308

layout(set = 0, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2D inputAttachments[2];

layout(location = 0) in vec2 inUVs;
layout(location = 1) in vec3 inPixPos;
layout(location = 2) flat in vec3 inCamPos;
layout(location = 3) flat in float inDotLimit;
layout(location = 4) flat in vec3 inLightDir;
layout(location = 5) flat in vec2 inClipPlanes;

layout(location = 0) out vec4 outColor;

vec4 originalColor();
float depth();
float linearDepth();
float depthDist();
vec4 sphere();
vec4 sea();
vec4 atmosphere();

void main()
{
	//vec2 NDCs = { gl_FragCoord.x / 960, gl_FragCoord.y / 540 };	// Get fragment Normalize Device Coordinates (NDC) [0,1] from its Window Coordinates (pixels)
	
	// https://www.reddit.com/r/vulkan/comments/mf1rb5/input_attachment_not_accessible_from_fragment/
	// https://stackoverflow.com/questions/45154213/is-there-a-way-to-address-subpassinput-in-vulkan
	// ChatGPT: The inputAttachment function is only available in GLSL shaders if the "GL_NV_shader_framebuffer_fetch" extension is enabled. This extension is not part of the core Vulkan specification and may not be available on all devices.
	//inputAttachment(0, vec2(NDCs.x, NDCs.y));
	//subpassLoad(inputAttachment, vec2(NDCs.x, NDCs.y));
	
	//outColor = originalColor();
	//outColor = vec4(depth(), depth(), depth(), 1.f);
	//outColor = vec4(linearDepth(), linearDepth(), linearDepth(), 1.f);
	//outColor = sphere();
	//outColor = sea();
	outColor = atmosphere();	// <<< to optimize (less lookups)
	//if(depth() == 1) outColor = vec4(0,1,0,1);
}

// No post processing. Get the original color.
vec4 originalColor() { return texture(inputAttachments[0], inUVs); }
//vec4 originalColor() { return vec4(1,1,1,1); }

// Get non linear depth. Range: [0, 1]
float depth() {	return texture(inputAttachments[1], inUVs).x; }

// Get linear depth. Range: [0, 1]. Link: https://stackoverflow.com/questions/51108596/linearize-depth
float linearDepth()
{
	float zNear = inClipPlanes[0];
	float zFar = inClipPlanes[1];
	
	return (zNear * zFar / (zFar + depth() * (zNear - zFar))) / (zFar - zNear);
}

// Get dist from near plane to fragment.
float depthDist()
{
	float zNear = inClipPlanes[0];
	float zFar = inClipPlanes[1];
	
	return zNear * zFar / (zFar + depth() * (zNear - zFar));
}

// Draw sphere
vec4 sphere() 
{ 
	vec4 color   = vec4(0,0,1,1);
	vec3 nuclPos = vec3(0,0,0);
	vec3 pixDir  = normalize(inPixPos - inCamPos);
	vec3 nuclDir = normalize(nuclPos  - inCamPos); 
	
	if(dot(pixDir, nuclDir) > inDotLimit) return color;
	else return originalColor();
}

// Draw sea
vec4 sea()
{
	vec4 color   = vec4(0,0,1,1);
	vec3 nuclPos = vec3(0,0,0);
	vec3 pixDir  = normalize(inPixPos - inCamPos);
	vec3 nuclDir = normalize(nuclPos  - inCamPos); 
	
	if(dot(pixDir, nuclDir) > inDotLimit) 
	{
		if(depthDist() > 0.000000) return color;
	}
	
	return originalColor();
}

// Returns vector(distToSphere, distThroughSphere). 
//		If rayOrigin is inside sphere, distToSphere = 0. 
//		If ray misses sphere, distToSphere = maxValue; distThroughSphere = 0.
vec2 raySphere(vec3 centre, float radius, vec3 rayOrigin, vec3 rayDir)
{
	// Number of intersections
	vec3 offset = rayOrigin - centre;
	float a = 1;						// Set to dot(rayDir, rayDir) if rayDir might not be normalized
	float b = 2 * dot(offset, rayDir);
	float c = dot(offset, offset) - radius * radius;
	float d = b * b - 4 * a * c;		// Discriminant of quadratic formula (sqrt has 2 solutions/intersections when positive)
	
	// Two intersections (d > 0).
	if(d > 0)	
	{
		float s = sqrt(d);
		float distToSphereNear = max(0, (-b - s) / (2 * a));
		float distToSphereFar = (-b + s) / (2 * a);
		
		if(distToSphereFar >= 0)		// Ignore intersections that occur behind the ray
			return vec2(distToSphereNear, distToSphereFar - distToSphereNear);
	}
	
	// No intersection (d < 0) or one (d = 0)
	return vec2(FLT_MAX, 0);			// https://stackoverflow.com/questions/16069959/glsl-how-to-ensure-largest-possible-float-value-without-overflow
}

// Atmosphere: 
//		https://github.com/SebLague/Solar-System/blob/Development/Assets/Scripts/Celestial/Shaders/PostProcessing/Atmosphere.shader
//		https://www.youtube.com/watch?v=DxfEbulyFcY
//		https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering

// Atmosphere's density at one point. The closer to the surface, the denser it is.
float densityAtPoint(vec3 point)
{
	float heightAboveSurface = length(point - PLANET_CENTER) - PLANET_RADIUS;
	float height01 = heightAboveSurface / (ATM_RADIUS - PLANET_RADIUS);
	
	//return exp(-height01 * DENSITY_FALLOFF);					// There is always some density
	return exp(-height01 * DENSITY_FALLOFF) * (1 - height01);	// Density ends at some distance
}

// Average atmosphere density along a ray.
float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength)
{
	vec3 point = rayOrigin;
	float stepSize = rayLength / (NUM_OPT_DEPTH_POINTS - 1);
	float opticalDepth = 0;
	
	for(int i = 0; i < NUM_OPT_DEPTH_POINTS; i++)
	{
		opticalDepth += densityAtPoint(point) * stepSize;
		point += rayDir * stepSize;
	}
	
	return opticalDepth;
}

// Describe the view ray of the camera through the atmosphere for the current pixel.
vec3 calculateLight(vec3 firstPoint, vec3 rayDir, float rayLength, vec3 originalCol)
{
	vec3 inScatterPoint = firstPoint;					// point to study
	vec3 inScatteredLight = vec3(0,0,0);				// light gathered
	float stepSize = rayLength / (NUM_SCATT_POINTS - 1);
	float viewRayOpticalDepth = 0;						// <<<
	
	for(int i = 0; i < NUM_SCATT_POINTS; i++)
	{
		vec3 dirToSun = -inLightDir;
		float sunRayLength = raySphere(PLANET_CENTER, ATM_RADIUS, inScatterPoint, dirToSun).y;		// length point-sunHit
		float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);			// OD: point-sunHit
		viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);					// OD: point-backHit
		vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * SCATT_COEFFICIENTS);	// Inversely proportional to optical depths
		float localDensity = densityAtPoint(inScatterPoint);										// atmosphere density at point
		
		inScatteredLight += localDensity * transmittance * SCATT_COEFFICIENTS * stepSize;			// light gathered for now <<< local density ?
		inScatterPoint   += rayDir * stepSize;														// next point
	}
	
	float originalColTransmittance = exp(-viewRayOpticalDepth);			// <<<
	return originalCol * originalColTransmittance + inScatteredLight;	// Final light color
}

// Final pixel color (atmosphere)
vec4 atmosphere()
{	
	// Camera ray
	vec3 rayOrigin = inCamPos;
	vec3 rayDir = normalize(inPixPos - inCamPos);	// normalize(inCamDir);
	
	// Distance to surface
	float distToOcean = raySphere(PLANET_CENTER, OCEAN_RADIUS, rayOrigin, rayDir).x;
	float distToSurface = min(depthDist(), distToOcean);
	
	// Distance to atmosphere and through it
	vec2 hitInfo = raySphere(PLANET_CENTER, ATM_RADIUS, rayOrigin, rayDir);
	float distToAtmosphere = hitInfo.x;
	float distThroughAtmosphere = min(hitInfo.y, distToSurface - distToAtmosphere);
	
	// Get fragment light
	vec4 originalCol = originalColor();
	if(distThroughAtmosphere > 0)
	{
		const float epsilon = 0.001;
		vec3 firstPoint = rayOrigin + rayDir * (distToAtmosphere + epsilon);
		vec3 light = calculateLight(firstPoint, rayDir, distThroughAtmosphere - epsilon * 2, originalCol.xyz);
		return vec4(light, 1);
	}
	
	return originalCol;
}

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

// https://stackoverflow.com/questions/38938498/how-do-i-convert-gl-fragcoord-to-a-world-space-point-in-a-fragment-shader
vec4 ndc2world2(vec2 ndc)
{
	// Window-screen space
	//	gl_FragCoord.xy: Window-space position of the fragment
	//	gl_FragCoord.z: Depth value of the fragment in window space used for depth testing. Range: [0, 1] (0 = near clipping plane; 1 = far clipping plane).
	//	gl_FragCoord.w: Window-space position of the fragment in the homogeneous coordinate system. Used for scaling elements in perspective projections depending upon distance to camera. Calculated by the graphics pipeline based on the projection matrix and the viewport transformation.
	
	// Normalized Device Coordinates
	vec2 winSize = vec2(1920/2, 1080/2);
	vec2 depthRange = vec2(100, 5000);		// { near, far}
		
	vec4 NDC;
	NDC.xy = 2.f * (gl_FragCoord.xy / winSize.xy) - 1.f;
	NDC.z = (2.f * gl_FragCoord.z - depthRange.x - depthRange.y) / (depthRange.y - depthRange.x);
	NDC.w = 1.f;
	
	// Clip space > Eye space > World space
	vec4 clipPos  = NDC / gl_FragCoord.w;
	mat4 inProj;
	mat4 inView;
	vec4 eyePos   = inverse(inProj) * clipPos;
	vec4 worldPos = inverse(inView) * eyePos;
 	return worldPos;
}