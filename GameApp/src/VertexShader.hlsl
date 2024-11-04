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


VertexOutput main(in VertexInput input)
{
	VertexOutput output;
	output.position = float4 (input.position, 1.0f);
    output.rgb = float3(1, 1, 1);
	return output;
}