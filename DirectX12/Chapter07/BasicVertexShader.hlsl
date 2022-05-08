#include "BasicShaderHeader.hlsli"

// 定数バッファー
cbuffer cvuff0 : register(b0)
{
	matrix mat;            // 変換行列
}

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;         // ピクセルシェーダへ渡す値
	output.svpos = mul(mat, pos);
	output.uv = uv;
	return output;
}