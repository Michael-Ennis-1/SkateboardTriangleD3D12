
struct VertexOutput
{
	float4 position : SV_Position;
	float3 rgb  : COLOR;
};


float4 main(in VertexOutput input) : SV_TARGET
{
	return float4(input.rgb, 1.0f);
}