
// === Defines ===

#define PI 3.141592654f
#define MAX_RECURSION_DEPTH 10

// === Structs ===

// Layout of data in the vertex buffer
struct Vertex
{
    float3 localPosition	: POSITION;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
    float2 uv				: TEXCOORD;
};
static const uint VertexSizeInBytes = 11 * 4; // 11 floats total per vertex * 4 bytes each

struct MaterialData
{
	float4 color;
	uint albedoIndex;
	uint roughnessIndex;
	uint normalsIndex;
	uint metalIndex;
};


// Payload for rays (data that is "sent along" with each ray during raytrace)
// Note: This should be as small as possible
struct RayPayload
{
	float3 color;
	uint recursionDepth; // How many times this ray has bounced
	uint rayPerPixelIndex; // What is this ray's index within its pixel (used for PRNG)
};

// Note: We'll be using the built-in BuiltInTriangleIntersectionAttributes struct
// for triangle attributes, so no need to define our own.  It contains a single float2.



// === Constant buffers ===

cbuffer SceneData : register(b0)
{
	matrix inverseViewProjection;
	float3 cameraPosition;
	float pad0;
};


// Ensure this matches C++ buffer struct define!
#define MAX_INSTANCES_PER_BLAS 100
cbuffer ObjectData : register(b1)
{
	matrix worldInvTranspose[MAX_INSTANCES_PER_BLAS];
	MaterialData material[MAX_INSTANCES_PER_BLAS];
};


// === Resources ===

// Output UAV 
RWTexture2D<float4> OutputColor				: register(u0);

// The actual scene we want to trace through (a TLAS)
RaytracingAccelerationStructure SceneTLAS	: register(t0);

// Geometry buffers
ByteAddressBuffer IndexBuffer        		: register(t1);
ByteAddressBuffer VertexBuffer				: register(t2);
Texture2D textures[]						: register(t3);

SamplerState BasicSampler					: register(s0);

// === Pseudo-random Number Generators ===
float rand(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float2 rand2(float2 uv)
{
	float x = rand(uv);
	float y = sqrt(1 - x * x);
	return float2(x, y);
}

float3 rand3(float2 uv)
{
	return float3(
		rand2(uv),
		rand(uv.yx));
}

float3 RandomCosineWeightedHemisphere(float u0, float u1, float3 unitNormal)
{
	float a = u0 * 2 - 1;
	float b = sqrt(1 - a * a);
	float phi = 2.0f * PI * u1;

	float x = unitNormal.x + b * cos(phi);
	float y = unitNormal.y + b * sin(phi);
	float z = unitNormal.z + a;
	return float3(x, y, z);
}


// === Other Helpers ===

// Loads the indices of the specified triangle from the index buffer
uint3 LoadIndices(uint triangleIndex)
{
	// What is the start index of this triangle's indices?
	uint indicesStart = triangleIndex * 3;

	// Adjust by the byte size before loading
	return IndexBuffer.Load3(indicesStart * 4); // 4 bytes per index
}

// Barycentric interpolation of data from the triangle's vertices
Vertex InterpolateVertices(uint triangleIndex, float3 barycentricData)
{
	// Grab the indices
	uint3 indices = LoadIndices(triangleIndex);

	// Set up the final vertex
	Vertex vert;
	vert.localPosition = float3(0, 0, 0);
	vert.uv = float2(0, 0);
	vert.normal = float3(0, 0, 0);
	vert.tangent = float3(0, 0, 0);

	// Loop through the barycentric data and interpolate
	for (uint i = 0; i < 3; i++)
	{
		// Get the index of the first piece of data for this vertex
		uint dataIndex = indices[i] * VertexSizeInBytes;

		// Grab the position and offset
		vert.localPosition += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// Normal
		vert.normal += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// Tangent
		vert.tangent += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// UV (no offset at the end, since we start over after looping)
		vert.uv += asfloat(VertexBuffer.Load2(dataIndex)) * barycentricData[i];
		
	}

	// Final interpolated vertex data is ready
	return vert;
}

// Calculates an origin and direction from the camera fpr specific pixel indices
void CalcRayFromCamera(float2 rayIndices, out float3 origin, out float3 direction)
{
	// Offset to the middle of the pixel
	float2 pixel = rayIndices + 0.5f;
	float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
	screenPos.y = -screenPos.y;

	// Unproject the coords
	float4 worldPos = mul(inverseViewProjection, float4(screenPos, 0, 1));
	worldPos.xyz /= worldPos.w;

	// Set up the outputs
	origin = cameraPosition.xyz;
	direction = normalize(worldPos.xyz - origin);
}


// === Shaders ===

// Ray generation shader - Launched once for each ray we want to generate
// (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{
	// Get the ray indices
	uint2 rayIndices = DispatchRaysIndex().xy;

	float3 totalColor = float3(0, 0, 0);

	int raysPerPixel = 25;
	for (int r = 0; r < raysPerPixel; r++) {
		float2 adjustedRayIndices = (float2)rayIndices + rand((float)r / raysPerPixel);

		// Calculate the ray data
		float3 rayOrigin;
		float3 rayDirection;
		CalcRayFromCamera(adjustedRayIndices, rayOrigin, rayDirection);

		// Set up final ray description
		RayDesc ray;
		ray.Origin = rayOrigin;
		ray.Direction = rayDirection;
		ray.TMin = 0.0001f;
		ray.TMax = 1000.0f;

		// Set up the payload for the ray
		// This initializes the struct to all zeros
		RayPayload payload = (RayPayload)0;
		payload.color = float3(1, 1, 1);

		// Perform the ray trace for this ray
		TraceRay(
			SceneTLAS,
			RAY_FLAG_NONE,
			0xFF,
			0,
			0,
			0,
			ray,
			payload);

		totalColor += payload.color;
	}
	totalColor /= raysPerPixel;

	// Set the final color of the buffer (gamma corrected)
	OutputColor[rayIndices] = float4(pow(totalColor, 1.0f / 2.2f), 1);
}


// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
	// Hemispheric gradient
	float3 upColor = float3(0.3f, 0.5f, 0.95f);
	float3 downColor = float3(1, 1, 1);

	// Interpolate based on the direction of the ray
	float interpolation = dot(normalize(WorldRayDirection()), float3(0, 1, 0)) * 0.5f + 0.5f;
	payload.color *= lerp(downColor, upColor, interpolation);
}


// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	// Check for max recursion depth
	if (payload.recursionDepth > MAX_RECURSION_DEPTH) {
		payload.color = float3(0, 0, 0);
		return;
	}
	
	// Grab the index of the triangle we hit
	uint triangleIndex = PrimitiveIndex();

	// Calculate the barycentric data for vertex interpolation
	float3 barycentricData = float3(
		1.0f - hitAttributes.barycentrics.x - hitAttributes.barycentrics.y,
		hitAttributes.barycentrics.x,
		hitAttributes.barycentrics.y);

	// Get the interpolated vertex data
	Vertex interpolatedVert = InterpolateVertices(triangleIndex, barycentricData);
	// Get the data for this entity
	uint instanceID = InstanceID();
	// Adjust tint of payload
	payload.color *= material[instanceID].color.rgb;
	
	float3 normal_WS = mul((float3x3)worldInvTranspose[instanceID], interpolatedVert.normal);
	float3 refl = reflect(WorldRayDirection(), normal_WS); // A perfect reflection across the normal
	float2 pixelUV = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
	float2 rng = rand2(pixelUV * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());
	float3 diff = RandomCosineWeightedHemisphere(rand(rng), rand(rng.yx), normal_WS);

	// Set up new ray
	RayDesc ray;
	ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent(); // Intrinsic functions: the current ray's origin in world coords plus its direction times its T
	ray.Direction = normalize(lerp(refl, diff, material[InstanceID()].color.a)); // Working just with specular for now
	ray.TMin = 0.0001f;
	ray.TMax = 1000.0f;

	// Continue recursion
	payload.recursionDepth++;
	TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,
		ray,
		payload);
}
