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

// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// @param format �t�H�[�}�b�g(%d�Ƃ�%f�Ƃ���)
// @param �ϒ�����
// @remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOoutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

// @biref �E�B���h�E�v���V�[�W��
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
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
		debugLayer->EnableDebugLayer();  // �f�o�b�O���C���[�̗L����
		debugLayer->Release();           // �C���^�[�t�F�C�X�����
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
	// �E�B���h�E�N���X�̐����E�o�^
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;  // �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");        // �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);    // �n���h���̎擾

	RegisterClassEx(&w);                       // �A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`����)

	RECT wrc = { 0,0,window_width,window_height };  // �E�B���h�E�̃T�C�Y
	// �֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,       // �N���X���w��
		_T("DX12�e�X�g"),      // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,   // �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,         // �\��x���W��OS�ɂ��C��
		CW_USEDEFAULT,         // �\��y���W��OS�ɂ��C��
		wrc.right - wrc.left,  // �E�B���h�E�̕�
		wrc.bottom - wrc.top,  // �E�B���h�E�̍���
		nullptr,               // �e�E�B���n���h��
		nullptr,               // ���j���[�n���h��
		w.hInstance,           // �Ăяo���A�v���P�[�V����
		nullptr);              // �ǉ��p�����[�^�[

#ifdef _DEBUG
	//�f�o�b�O���C���[���I����
	//�f�o�C�X�������O�ɂ���Ă����Ȃ��ƁA�f�o�C�X������ɂ���
	//�f�o�C�X�����X�Ƃ��Ă��܂��̂Œ���
	EnableDebugLayer();
#endif
	// DirectX12�܂��̏�����
	// �t�B�[�`���[���x���̗�
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

	// �A�_�v�^�[�̗񋓗p
	std::vector<IDXGIAdapter*> adapters;
	// �����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;

	for (int i = 0; dxgiFactory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);   // �A�_�v�^�[�̐����I�u�W�F�N�g�擾

		std::wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;  // �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	// �^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// �A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.NodeMask = 0;
	// �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// �R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// �L���[����
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
	// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	// �t���b�v��͑��₩�ɔj��
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// ���Ɏw��Ȃ�
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// �E�B���h�E�̃t���X�N���[���؂�ւ��\
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = dxgiFactory_->CreateSwapChainForHwnd(_cmdQueue,                           // �R�}���h�L���[�I�u�W�F�N�g
		hwnd,                                // �E�B���h�E�n���h��
		&swapchainDesc,                      // �X���b�v�`�F�[���ݒ�
		nullptr,                             // 
		nullptr,
		(IDXGISwapChain1**)&_swapchain);     // �X���b�v�`�F�[���I�u�W�F�N�g�擾�p

// �f�B�X�N���v�^�q�[�v�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;    // �����_�[�^�[�Q�b�g�r���[(RTV)
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                       // �f�B�X�N���v�^�[�̐�
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // ���Ɏw��Ȃ�
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	// �X���b�v�`�F�[���̃������ƂЂ��t����
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	// �f�B�X�N���v�^�q�[�v�̐擪�A�h���X���擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		// �����_�[�^�[�Q�b�g�r���[�𐶐�����
		_dev->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
		// �f�B�X�N���v�^�̎�ނɉ������T�C�Y���|�C���^�[�����炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// �E�B���h�E�̕\��
	ShowWindow(hwnd, SW_SHOW);

	XMFLOAT3 vertices[] = 
	{
		{-0.4f,-0.7f,0.0f}, // �C���f�b�N�X : 0
		{-0.4f,0.7f,0.0f},  // �C���f�b�N�X : 1
		{0.4f,-0.7f,0.0f},  // �C���f�b�N�X : 2
		{0.4f,0.7f,0.0f},   // �C���f�b�N�X : 3
	};

	// ���_�o�b�t�@�[�̐���
	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);    // ���_��񂪓��邾���̃T�C�Y
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

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();  // �o�b�t�@�[�̉��z�A�h���X
	vbView.SizeInBytes = sizeof(vertices);                     // �S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]);                // 1���_������̃o�C�g��

	unsigned short indices[] =
	{
		0,1,2,
		2,1,3
	};

	ID3D12Resource* idxBuff = nullptr;
	// �ݒ�̓o�b�t�@�[�̃T�C�Y�ȊO�A���_�o�b�t�@�[�̐ݒ���g���܂킷
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(&heapprop, 
		                                   D3D12_HEAP_FLAG_NONE, 
		                                   &resdesc, 
		                                   D3D12_RESOURCE_STATE_GENERIC_READ, 
		                                   nullptr, 
		                                   IID_PPV_ARGS(&idxBuff));

	// ������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappeIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappeIdx);
	std::copy(std::begin(indices), std::end(indices), mappeIdx);
	idxBuff->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�[�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* _errorBlob = nullptr;

	// ���_�V�F�[�_�[�̓ǂݍ���
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",                        // �V�F�[�_�[��
	                            nullptr,                                          // define�Ȃ�
		                        D3D_COMPILE_STANDARD_FILE_INCLUDE,                // �C���N���[�h�t�@�C���̓f�t�H���g
	                            "BasicVS", "vs_5_0",                              // �֐���BasicVS,�ΏۃV�F�[�_�[��vs_5.0
		                        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  // �f�o�b�O�p�y�эœK���Ȃ�
	                            0,
	                            &_vsBlob, &_errorBlob);                           // �G���[����errorBlob�Ƀ��b�Z�[�W������

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C����������܂���");
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

	// �s�N�Z���V�F�[�_�̓ǂݍ���
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",                         // �V�F�[�_�[��
	                            nullptr,										  // define�Ȃ�
	                            D3D_COMPILE_STANDARD_FILE_INCLUDE,				  // �C���N���[�h�t�@�C���̓f�t�H���g
	                            "BasicPS", "ps_5_0",							  // �֐���BasicVS,�ΏۃV�F�[�_�[��vs_5.0
	                            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  // �f�o�b�O�p�y�эœK���Ȃ�
	                            0,
	                            &_psBlob, &_errorBlob);							  // �G���[����errorBlob�Ƀ��b�Z�[�W������

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C����������܂���");
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

	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �V�F�[�_�[�̃Z�b�g
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;           // �f�t�H���g�̃T���v���}�X�N��\���萔(0xffffff)
	gpipeline.RasterizerState.MultisampleEnable = false;        // �A���`�G�C���A�X�͎g��Ȃ�����false
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // �J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;           // �[�x�����̃N���b�s���O�͗L��
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

	gpipeline.InputLayout.pInputElementDescs = inputLayout;      // ���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);   // ���C�A�E�g�z��̗v�f��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;   // �X�g���b�v�̃J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;  // �O�p�`�ō\��

	gpipeline.NumRenderTargets = 1;                              // ����1�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;        // 0�`1�ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;                              // �T���v�����O��1�s�N�Z���ɂ�1
	gpipeline.SampleDesc.Quality = 0;                            // �N�I���e�B�͍Œ�

	ID3D12RootSignature* rootsignature = nullptr;

	// ���[�g�V�O�l�`���̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// �o�C�i���R�[�h�̍쐬
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc,                 // ���[�g�V�O�l�`���ݒ�
	                                     D3D_ROOT_SIGNATURE_VERSION_1_0,     // ���[�g�V�O�l�`���o�[�W����
	                                     &rootSigBlob,                       // �V�F�[�_�[����������Ɠ���
	                                     &_errorBlob);                       // �G���[����������
	// ���[�g�V�O�l�`���I�u�W�F�N�g�̐���
	result = _dev->CreateRootSignature(0,                                    // nodemask
	                                   rootSigBlob->GetBufferPointer(),      // �V�F�[�_�[�̂Ƃ��Ɠ��l
	                                   rootSigBlob->GetBufferSize(),         // �V�F�[�_�[�̂Ƃ��Ɠ��l
	                                   IID_PPV_ARGS(&rootsignature));        // 

	rootSigBlob->Release();                                                  // �s�v�ɂȂ����̂ŊJ��

	gpipeline.pRootSignature = rootsignature;                                 // �O���t�B�b�N�X�X�e�[�g�ɐݒ�

	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	// �r���[�|�[�g�̐ݒ�
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;     // �o�͐�̕�
	viewport.Height = window_height;   // �o�͐�̍���
	viewport.TopLeftX = 0;             // �o�͐�ɍ�����WX
	viewport.TopLeftY = 0;             // �o�͐�ɍ�����WY
	viewport.MaxDepth = 1.0f;          // �[�x�ő�l
	viewport.MinDepth = 0.0f;          // �[�x�ŏ��l

	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;                                      // �؂蔲������W
	scissorrect.left = 0;                                     // �؂蔲�������W
	scissorrect.right = scissorrect.left + window_width;      // �؂蔲���E���W
	scissorrect.bottom = scissorrect.top + window_height;     // �؂蔲���������W


	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// �A�v���P�[�V�������I���Ƃ���message��WM_QUTI�ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}


		// DirectX����
		// �o�b�N�o�b�t�@�̃C���f�b�N�X���擾
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;    // �J��
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;         // ���Ɏw��Ȃ�
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];       // �o�b�N�o�b�t�@�[���\�[�X
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore
			= D3D12_RESOURCE_STATE_PRESENT;                           // ���O��PRESENT���
		BarrierDesc.Transition.StateAfter
			= D3D12_RESOURCE_STATE_RENDER_TARGET;                     // �����烌���_�[�^�[�Q�b�g���
		_cmdList->ResourceBarrier(1, &BarrierDesc);                   // �o���A�w����s
		_cmdList->SetPipelineState(_pipelinestate);                   // �p�C�v���C���X�e�[�g�̐ݒ�
		// �����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		_cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

		// ��ʃN���A
		float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };  // ���F
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->SetGraphicsRootSignature(rootsignature);             // ���[�g�V�O�l�`���̐ݒ�

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // �v���~�e�B�u�g�|���W�ݒ�
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);                           // ���_�o�b�t�@�[�̃Z�b�g

		// �`�施�߂̐ݒ�
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// ��Ԃ̑O������ւ���
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		// ���߂̃N���[�Y
		_cmdList->Close();

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			// �C�x���g����������܂ő҂�������
			WaitForSingleObject(event, INFINITE);

			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		// �L���[���N���A
		_cmdAllocator->Reset();
		// �ĂуR�}���h���X�g�����߂鏀��
		_cmdList->Reset(_cmdAllocator, nullptr);
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//�ĂуR�}���h���X�g�����߂鏀��

		// �t���b�v
		_swapchain->Present(1, 0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}