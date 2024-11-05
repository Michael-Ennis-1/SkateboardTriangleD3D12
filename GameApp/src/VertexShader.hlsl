struct VertexInput
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BINORMAL;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float3 rgb  : COLOR;
};

// Create the pass buffer
struct PassBuffer
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrixInverse;
    float4x4 ProjectionMatrixInverse;
    float3 CameraPosition;
    float Padding;
};

// Forward declare the pass buffer, accessing memory register b0
ConstantBuffer<PassBuffer> gPassBuffer : register(b0);

VertexOutput main(in VertexInput input)
{
	VertexOutput output;
	
	output.position = float4 (input.position, 1.0f);
    output.rgb = float3(0, 0, 0);
	
	return output;
}