Texture2D AlbedoTexture		: register(t0);	// "t" registers for textures
Texture2D RoughnessMap		: register(t1);
Texture2D NormalMap			: register(t2);
Texture2D MetalnessMap		: register(t3);
SamplerState BasicSampler	: register(s0);	// "s" registers for samplers

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
	float3 normal			: NORMAL;		// Surface normal
	float3 tangent			: TANGENT;		// Surface tangent
	float3 worldPosition	: POSITION;		// Position in world space
	float2 uv				: TEXCOORD;     // UV position
};

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
	float3 albedo = pow(AlbedoTexture.Sample(BasicSampler, input.uv).rgb, 2.2f);

	//return float4(albedo.r, albedo.g, albedo.b, 1);
	return float4(1, 1, 1, 1);
}