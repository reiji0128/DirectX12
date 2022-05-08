#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <vector>
#include <string>
#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

// @brief コンソール画面にフォーマット付き文字列を表示
// @param format フォーマット(%dとか%fとかの)
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOoutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

// @biref ウィンドウプロシージャ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

IDXGIFactory6* dxgiFactory_ = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();  // デバッグレイヤーの有効化
		debugLayer->Release();           // インターフェイスを解放
	}
}

#ifdef _DEBUG
int main()
{
#else
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	DebugOoutputFormatString("Show window test.");

	// ウィンドウクラスの生成・登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;  // コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");        // アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);    // ハンドルの取得
	RegisterClassEx(&w);                       // アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)

	RECT wrc = { 0,0,window_width,window_height };  // ウィンドウのサイズ
	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,       // クラス名指定
		_T("DX12テスト"),                           // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,                        // タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,                              // 表示x座標はOSにお任せ
		CW_USEDEFAULT,                              // 表示y座標はOSにお任せ
		wrc.right - wrc.left,                       // ウィンドウの幅
		wrc.bottom - wrc.top,                       // ウィンドウの高さ
		nullptr,                                    // 親ウィンハンドル
		nullptr,                                    // メニューハンドル
		w.hInstance,                                // 呼び出しアプリケーション
		nullptr);                                   // 追加パラメーター

#ifdef _DEBUG
	//デバッグレイヤーをオンに
	//デバイス生成時前にやっておかないと、デバイス生成後にやると
	//デバイスがロスとしてしまうので注意
	EnableDebugLayer();
#endif
	// DirectX12まわりの初期化
	// フィーチャーレベルの列挙
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory_))))
	{
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory_))))
		{
			return -1;
		}
	}

	// アダプターの列挙用
	std::vector<IDXGIAdapter*> adapters;
	// ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;

	for (int i = 0; dxgiFactory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);   // アダプターの説明オブジェクト取得

		std::wstring strDesc = adesc.Description;

		// 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;  // 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;                           // タイムアウトなし
	cmdQueueDesc.NodeMask = 0;                                                    // アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;                  // プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;                           // コマンドリストと合わせる
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));   // コマンドキュー生成

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;                      // バックバッファーは伸び縮み可能
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;          // フリップ後は速やかに破棄
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;             // 特に指定なし
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;      // ウィンドウ⇔フルスクリーン切り替え可能

	result = dxgiFactory_->CreateSwapChainForHwnd(_cmdQueue,           // コマンドキューオブジェクト
		hwnd,                                                          // ウィンドウハンドル
		&swapchainDesc,                                                // スワップチェーン設定
		nullptr,                                                       // 
		nullptr,							                           
		(IDXGISwapChain1**)&_swapchain);                               // スワップチェーンオブジェクト取得用

    // ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;    // レンダーターゲットビュー(RTV)
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                       // ディスクリプターの数
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // 特に指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	// スワップチェーンのメモリとひも付ける
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	// ディスクリプタヒープの先頭アドレスを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	// SRGBレンダーターゲット設定
	D3D12_RENDER_TARGET_VIEW_DESC  rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;   // ガンマ補正あり(sRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (size_t i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle);                        // レンダーターゲットビューを生成する
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);   // ディスクリプタの種類に応じたサイズ分ポインターをずらす
	}

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	// 頂点データ構造体
	struct Vertex
	{
		XMFLOAT3 pos;  // XYZ座標
		XMFLOAT2 uv;   // uv座標
	};

	Vertex vertices[] =
	{
		{{ -1.0f,  -1.0f,  0.0f},{ 0.0f, 1.0f}},  // 左下 インデックス : 0
		{{ -1.0f,   1.0f,  0.0f},{ 0.0f, 0.0f}},  // 左上 インデックス : 1
		{{  1.0f,  -1.0f,  0.0f},{ 1.0f, 1.0f}},  // 右下 インデックス : 2
		{{  1.0f,   1.0f,  0.0f},{ 1.0f, 0.0f}},  // 右上 インデックス : 3
	};

	// 頂点バッファーの生成
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resDesc = {};

	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeof(vertices);    // 頂点情報が入るだけのサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// UPLOAD(確保は可能)
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	Vertex* vertMap = nullptr;

	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();  // バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices);                     // 全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]);                // 1頂点当たりのバイト数

	unsigned short indices[] =
	{
		0,1,2,
		2,1,3
	};

	ID3D12Resource* idxBuff = nullptr;
	// 設定はバッファーのサイズ以外、頂点バッファーの設定を使いまわす
	resDesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	// 作ったバッファーにインデックスデータをコピー
	unsigned short* mappeIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappeIdx);
	std::copy(std::begin(indices), std::end(indices), mappeIdx);
	idxBuff->Unmap(0, nullptr);

	// インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* _errorBlob = nullptr;

	// 頂点シェーダーの読み込み
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",                        // シェーダー名
		nullptr,                                          // defineなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,                // インクルードファイルはデフォルト
		"BasicVS", "vs_5_0",                              // 関数はBasicVS,対象シェーダーはvs_5.0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  // デバッグ用及び最適化なし
		0,
		&_vsBlob, &_errorBlob);                           // エラー時はerrorBlobにメッセージを入れる

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");
			exit(1);
		}
		else
		{
			std::string errstr;
			errstr.resize(_errorBlob->GetBufferSize());

			std::copy_n((char*)_errorBlob->GetBufferPointer(),
				_errorBlob->GetBufferSize(),
				errstr.begin());

			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
	}

	// ピクセルシェーダの読み込み
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",                         // シェーダー名
		nullptr,										  // defineなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				  // インクルードファイルはデフォルト
		"BasicPS", "ps_5_0",							  // 関数はBasicVS,対象シェーダーはvs_5.0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  // デバッグ用及び最適化なし
		0,
		&_psBlob, &_errorBlob);							  // エラー時はerrorBlobにメッセージを入れる

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");
			exit(1);
		}
		else
		{
			std::string errstr;
			errstr.resize(_errorBlob->GetBufferSize());

			std::copy_n((char*)_errorBlob->GetBufferPointer(),
				_errorBlob->GetBufferSize(),
				errstr.begin());

			errstr += "\n";
			::OutputDebugStringA(errstr.c_str());
		}
	}

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			// 座標情報
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{
			// uv座標情報
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		}
	};

	// シェーダーのセット
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;           // デフォルトのサンプルマスクを表す定数(0xffffff)
	gpipeline.RasterizerState.MultisampleEnable = false;        // アンチエイリアスは使わないためfalse
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;           // 深度方向のクリッピングは有効
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;      // レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);   // レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;   // ストリップのカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;  // 三角形で構成

	gpipeline.NumRenderTargets = 1;                              // 今は1つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;        // 0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;                              // サンプリングは1ピクセルにつき1
	gpipeline.SampleDesc.Quality = 0;                            // クオリティは最低

	ID3D12RootSignature* rootsignature = nullptr;

	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};                  // テクスチャと定数の２つ
	// テクスチャ用レジスター0番
	descTblRange[0].NumDescriptors = 1;                           // テクスチャ1つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  // 種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0;                       // 0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数用レジスター0番
	descTblRange[1].NumDescriptors = 1;                           // 定数1つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;  // 種別は定数
	descTblRange[1].BaseShaderRegister = 0;                       // 0番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメーターの作成
	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam.DescriptorTable.pDescriptorRanges = descTblRange;          // 配列先頭アドレス
	rootparam.DescriptorTable.NumDescriptorRanges = 2;                   // ディスクリプタレンジ数
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;            // すべてのシェーダから見える

	//D3D12_ROOT_PARAMETER rootparam[2] = {};
	//rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];        // ディスクリプタレンジのアドレス
	//rootparam[0].DescriptorTable.NumDescriptorRanges = 1;                     // ディスクリプタレンジ数
	//rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;            // ピクセルシェーダから見える

	//rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	//rootparam[1].DescriptorTable.NumDescriptorRanges = 1;                     // ディスクリプタレンジ数
	//rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;           // 頂点シェーダから見える

	rootSignatureDesc.pParameters = &rootparam;                                 // ルートパラメーターの先頭アドレス
	rootSignatureDesc.NumParameters = 1;                                        // ルートパラメーター数

	// サンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                  // 横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                  // 縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                  // 奥行の繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	 // ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;                    // 線形補完
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                                  // ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;                                               // ミップマップ最小値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;            // ピクセルシェーダから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;                // リサンプリングしない

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	// バイナリコードの作成
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc,                 // ルートシグネチャ設定
		                                 D3D_ROOT_SIGNATURE_VERSION_1_0,     // ルートシグネチャバージョン
		                                 &rootSigBlob,                       // シェーダーを作った時と同じ
		                                 &_errorBlob);                       // エラー処理も同じ
    // ルートシグネチャオブジェクトの生成
	result = _dev->CreateRootSignature(0,                                    // nodemask
		                               rootSigBlob->GetBufferPointer(),      // シェーダーのときと同様
		                               rootSigBlob->GetBufferSize(),         // シェーダーのときと同様
		                               IID_PPV_ARGS(&rootsignature));        // 

	rootSigBlob->Release();                                                  // 不要になったので開放

	gpipeline.pRootSignature = rootsignature;                                // グラフィックスステートに設定
	// グラフィックスパイプラインステートオブジェクトの生成
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	// ビューポートの設定
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;     // 出力先の幅
	viewport.Height = window_height;   // 出力先の高さ
	viewport.TopLeftX = 0;             // 出力先に左上座標X
	viewport.TopLeftY = 0;             // 出力先に左上座標Y
	viewport.MaxDepth = 1.0f;          // 深度最大値
	viewport.MinDepth = 0.0f;          // 深度最小値

	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;                                      // 切り抜き上座標
	scissorrect.left = 0;                                     // 切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;      // 切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;     // 切り抜きした座標

	// WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(L"img/textest.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	auto img = scratchImg.GetImage(0, 0, 0);                   // 生データ抽出

	//// まずは中間バッファとしてのアップロードヒープ設定
	//D3D12_HEAP_PROPERTIES uploadheapProp = {};
	//uploadheapProp.Type = D3D12_HEAP_TYPE_UPLOAD;                        // マップ可能にするため,UPLOADにする
	//uploadheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;    // アップロードに使用すること前提
	//uploadheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;     // アップロードに使用すること前提
	//uploadheapProp.CreationNodeMask = 0;                                 // 単一アダプターのための0
	//uploadheapProp.VisibleNodeMask = 0;                                  // 単一アダプターのための0

	//D3D12_RESOURCE_DESC resDesc = {};
	//resDesc.Format = DXGI_FORMAT_UNKNOWN;                                // 単なるデータの塊なのでUNKNOWN
	//resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;                 // 単なるバッファーとして設定
	//resDesc.Width = img->slicePitch;                                     // データサイズ
	//resDesc.Height = 1;
	//resDesc.DepthOrArraySize = 1;
	//resDesc.MipLevels = 1;
	//resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;                     // 連続したデータ
	//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;                            // 特にフラグなし
	//resDesc.SampleDesc.Count = 1;                                        // 通常テクスチャなのでアンチエイリアシングしない
	//resDesc.SampleDesc.Quality = 0;

	//// 中間バッファー作成
	//ID3D12Resource* uploadbuff = nullptr;
	//result = _dev->CreateCommittedResource(
	//	&uploadheapProp,
	//	D3D12_HEAP_FLAG_NONE,              // 特に指定なし
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(&uploadbuff)
	//);

	// テクスチャのためのヒープ設定(WriteToSubresource用)
	D3D12_HEAP_PROPERTIES texheapProp = {};
	texheapProp.Type = D3D12_HEAP_TYPE_CUSTOM;                                       // テクスチャ用
	texheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;                // ライトバック
	texheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;                         // 転送がL0つまりCPU側から直で
	texheapProp.CreationNodeMask = 0;                                                // 単一アダプターのため0
	texheapProp.VisibleNodeMask = 0;                                                 // 単一アダプターのため0
																		             
	// リソース設定(変数は使いまわし)										           
	resDesc.Format = metadata.format;                                                // DXGI_FORMAT_R8G8B8A8_UNORM;//RGBAフォーマット
	resDesc.Width = static_cast<UINT>(metadata.width);                               // 幅
	resDesc.Height = static_cast<UINT>(metadata.height);                             // 高さ
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);              // 2Dで配列でもないので 1
	resDesc.SampleDesc.Count = 1;                                                    // 通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0;										             
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);                     // ミップマップしないのでミップ数は1つ
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);   // 2Dテクスチャ用
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                                   // レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;                                        // 特にフラグなし
	
	// テクスチャバッファーの作成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texheapProp,
		D3D12_HEAP_FLAG_NONE,            // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,  // コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	//// テクスチャデータの作成
	//struct TexRGBA
	//{
	//	unsigned char R, G, B, A;
	//};
	//std::vector<TexRGBA> texturedata(256 * 256);

	//for (auto& rgba : texturedata)
	//{
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255;            // aは1.0とする
	//}

	texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texheapProp,
		D3D12_HEAP_FLAG_NONE,                              // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,        // テクスチャ用(ピクセルシェーダから見る用)
		nullptr, 
		IID_PPV_ARGS(&texbuff));
	
	// テクスチャバッファーに転送
	result = texbuff->WriteToSubresource(
		0,
		nullptr,                               // 全領域へコピー
		img->pixels,                           // 元データアドレス
		img->rowPitch,                         // 1ラインサイズ
		img->slicePitch                        // 1枚サイズ
	);

	// ワールド行列
	auto worldMat = XMMatrixRotationY(XM_PIDIV4);     // 45°

	// ビュー行列
	XMFLOAT3 eye(0, 0, -5);
	XMFLOAT3 target(0, 0, 0);
	XMFLOAT3 up(0, 1, 0);
	auto viewMat =  XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// プロジェクション行列
	auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,                                    // 画角は90°
		           static_cast<float>(window_width) / static_cast<float>(window_height),  // アスペクト比
		           1.0f,                                                                  // 近いほう
		           10.0f                                                                  // 遠いほう
	);

	XMMATRIX matrix = XMMatrixIdentity();
	matrix.r[0].m128_f32[0] = 2.0f / window_width;
	matrix.r[1].m128_f32[1] = -2.0f / window_height;
	matrix.r[3].m128_f32[0] = -1.0f;
	matrix.r[3].m128_f32[1] =  1.0f;

	// 定数バッファの作成
	ID3D12Resource* constBuff = nullptr;
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(matrix) + 0xff) & ~0xff);
	_dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff));

	XMMATRIX* mapMatrix;                                       // マップ先を示すポインター
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);   // マップ
	*mapMatrix = worldMat * viewMat * projMat;

	// ディスクリプタヒープの作成
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;                    // シェーダーから見えるように
	descHeapDesc.NodeMask = 0;                                                         // マスクは0
	descHeapDesc.NumDescriptors = 2;                                                   // SRV1つとCBV1つ
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;                        // シェーダーリソースビュー用
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));  // 生成

	// シェーダーリソースビューの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;  // 指定されたフォーマットにデータ通りの順序で割り当てられている
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                       // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;                                             // ミップマップは使用しないので1
	_dev->CreateShaderResourceView(                            // 生成
		texbuff,                                               // ビューと関連付けるバッファー
		&srvDesc,                                              // 先ほど設定したテクスチャ設定情報
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()    // ヒープのどこに割り当てるか
	);

	// 定数バッファービューの作成
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();                                  // ディスクリプターヒープの先頭アドレスを取得
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);       // ハンドルのポインタメンバーを大きさだけインクリメント
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle); // 生成


	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// アプリケーションが終わるときにmessageがWM_QUTIになる
		if (msg.message == WM_QUIT)
		{
			break;
		}


		// DirectX処理
		// バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;    // 遷移
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;         // 特に指定なし
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];       // バックバッファーリソース
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore
			= D3D12_RESOURCE_STATE_PRESENT;                           // 直前はPRESENT状態
		BarrierDesc.Transition.StateAfter
			= D3D12_RESOURCE_STATE_RENDER_TARGET;                     // 今からレンダーターゲット状態
		_cmdList->ResourceBarrier(1, &BarrierDesc);                   // バリア指定実行
		_cmdList->SetPipelineState(_pipelinestate);                   // パイプラインステートの設定
		// レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		_cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

		// 画面クリア
		float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };  // 黄色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->SetGraphicsRootSignature(rootsignature);             // ルートシグネチャの設定

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // プリミティブトポロジ設定
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);                           // 頂点バッファーのセット

		_cmdList->SetGraphicsRootSignature(rootsignature);             // ルートシグネチャの設定
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);               // ディスクリプタヒープの指定
		_cmdList->SetGraphicsRootDescriptorTable(                      // ルートパラメーターとディスクリプタヒープの関連付け
			0,                                                         // ルートパラメーターインデックス
			basicDescHeap->GetGPUDescriptorHandleForHeapStart());      // ヒープアドレス

		// ルートパラメーター番号1に対してディスクリプタヒープの次の場所を対応付け
		/*auto heapHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_cmdList->SetGraphicsRootDescriptorTable(1, heapHandle);*/
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		// 描画命令の設定
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// 状態の前後を入れ替える
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		// 命令のクローズ
		_cmdList->Close();

		// コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			// イベントが発生するまで待ち続ける
			WaitForSingleObject(event, INFINITE);

			// イベントハンドルを閉じる
			CloseHandle(event);
		}

		// キューをクリア
		_cmdAllocator->Reset();
		// 再びコマンドリストをためる準備
		_cmdList->Reset(_cmdAllocator, nullptr);
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//再びコマンドリストをためる準備

		// フリップ
		_swapchain->Present(1, 0);
	}

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}