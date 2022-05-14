#include "BasicShaderHeader.hlsli"


Output BasicVS(float4 pos : POSITION, 
	           float4 normal : NORMAL,
	           float2 uv : TEXCOORD,
	           min16uint2 boneno : BONE_NO,
	           min16uint weight : WEIGHT)
{
	Output output;                                 // �s�N�Z���V�F�[�_�֓n���l
	output.svpos = mul(mul(viewproj,world),pos);   // �V�F�[�_�[�ł͗�D��
	normal.w = 0;                                  // ���s�ړ������𖳌�
	output.normal = mul(world,normal);             // �@���Ƀ��[���h�ϊ����s��
	output.uv = uv;
	return output;
}