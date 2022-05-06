#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <vector>
#include <string>
#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

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
		_T("DX12テスト"),      // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,   // タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,         // 表示x座標はOSにお任せ
		CW_USEDEFAULT,         // 表示y座標はOSにお任せ
		wrc.right - wrc.left,  // ウィンドウの幅
		wrc.bottom - wrc.top,  // ウィンドウの高さ
		nullptr,               // 親ウィンハンドル
		nullptr,               // メニューハンドル
		w.hInstance,           // 呼び出しアプリケーション
		nullptr);              // 追加パラメーター

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
	// タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.NodeMask = 0;
	// プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// キュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	// バックバッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	// フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// 特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// ウィンドウ⇔フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = dxgiFactory_->CreateSwapChainForHwnd(_cmdQueue,                           // コマンドキューオブジェクト
		hwnd,                                // ウィンドウハンドル
		&swapchainDesc,                      // スワップチェーン設定
		nullptr,                             // 
		nullptr,
		(IDXGISwapChain1**)&_swapchain);     // スワップチェーンオブジェクト取得用

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
	for (size_t i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		// レンダーターゲットビューを生成する
		_dev->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
		// ディスクリプタの種類に応じたサイズ分ポインターをずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	XMFLOAT3 vertices[] = 
	{
		{-0.4f,-0.7f,0.0f}, // インデックス : 0
		{-0.4f,0.7f,0.0f},  // インデックス : 1
		{0.4f,-0.7f,0.0f},  // インデックス : 2
		{0.4f,0.7f,0.0f},   // インデックス : 3
	};

	// 頂点バッファーの生成
	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);    // 頂点情報が入るだけのサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;

	result = _dev->CreateCommittedResource(&heapprop, 
	                                       D3D12_HEAP_FLAG_NONE, 
	                                       &resdesc, 
	                                       D3D12_RESOURCE_STATE_GENERIC_READ, 
	                                       nullptr, 
	                                       IID_PPV_ARGS(&vertBuff));

	XMFLOAT3* vertMap = nullptr;

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
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(&heapprop, 
		                                   D3D12_HEAP_FLAG_NONE, 
		                                   &resdesc, 
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
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
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

	gpipeline.pRootSignature = rootsignature;                                 // グラフィックスステートに設定

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