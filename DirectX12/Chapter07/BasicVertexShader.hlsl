#include "BasicShaderHeader.hlsli"

// �萔�o�b�t�@�[
cbuffer cvuff0 : register(b0)
{
	matrix mat;            // �ϊ��s��
}

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;         // �s�N�Z���V�F�[�_�֓n���l
	output.svpos = mul(mat, pos);
	output.uv = uv;
	return output;
}