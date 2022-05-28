#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex : register(t0);     // 0番スロットに設定されたテクスチャ
SamplerState smp :register(s0);           // 0番スロットに設定されたサンプラー

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1,-1,1));
	float brightness = dot(-light, input.normal);
	return float4(brightness, brightness, brightness, 1) * diffuse * tex.Sample(smp,input.uv);

}