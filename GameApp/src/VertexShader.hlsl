struct VertexInput
{
	float3 position : POSITION;
	float3 color : COLOR;
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
	output.rgb = input.color;
	return output;
}