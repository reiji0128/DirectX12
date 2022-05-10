#include "BasicShaderHeader.hlsli"

// �萔�o�b�t�@�[
cbuffer cvuff0 : register(b0)
{
	matrix mat;            // �ϊ��s��
}

Output BasicVS(float4 pos : POSITION, 
	           float4 normal : NORMAL,
	           float2 uv : TEXCOORD,
	           min16uint2 boneno : BONE_NO,
	           min16uint weight : WEIGHT)
{
	Output output;         // �s�N�Z���V�F�[�_�֓n���l
	output.svpos = mul(mat, pos);
	output.uv = uv;
	return output;
}