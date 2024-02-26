cbuffer WorldData : register(b0) {
	matrix world;
	matrix worldInvTranspose;
	matrix view;
	matrix projection;
}

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;     // XYZ position
	float3 normal			: NORMAL;       // RGBA color
	float3 tangent			: TANGENT;		// 
	float2 uv				: TEXCOORD;		// UV texture coordinate
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
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
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;

	// X and Y components must be between -1 and 1, Z component must be between 0 and 1.  
	// Each of these components is then automatically divided by the W component (1.0 for now)
	matrix wvp = mul(projection, mul(view, world));
	output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

	// Transform the normal in the same way this vertex was transformed
	//output.normal = mul((float3x3)worldInvTranspose, input.normal); // Why does this work again???
	output.normal = input.normal;

	output.tangent = mul((float3x3)world, input.tangent);

	// Transform local position and output as a float3
	output.worldPosition = mul(world, float4(input.localPosition, 1)).xyz;

	// Pass the uv coordinate through 
	output.uv = input.uv;

	// Send the output to the next stage (pixel shader)
	return output;
}