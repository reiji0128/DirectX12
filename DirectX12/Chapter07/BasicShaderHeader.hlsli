// 頂点シェーダからピクセルシェーダへのやり取りに使用する構造体
struct Output
{
	float4 svpos  : SV_POSITION;   // システム頂点座標
	float4 normal : NORMAL;        // 法線ベクトル   
	float2 uv     : TEXCOORD;      // uv値
};

// 定数バッファー
cbuffer cbuff0 : register(b0)
{
	matrix world;                  // ワールド変換行列
	matrix viewproj;               // ビュープロジェクション行列
}