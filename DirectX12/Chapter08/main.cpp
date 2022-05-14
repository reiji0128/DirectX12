#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>

#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>


#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;

// PMD�\����
struct PMD_VERTEX
{
	XMFLOAT3 pos;                // ���_���W       : 12�o�C�g
	XMFLOAT3 normal;             // �@���x�N�g��   : 12�o�C�g
	XMFLOAT2 uv;                 // uv���W         :  8�o�C�g
	unsigned short boneNo[2];    // �{�[���ԍ�     :  4�o�C�g
	unsigned char boneWeight;    // �{�[���e���x   :  1�o�C�g
	unsigned char edgeFlg;       // �֊s���t���O   :  1�o�C�g
};                               //              ���v38�o�C�g

// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// @param format �t�H�[�}�b�g(%d�Ƃ�%f�Ƃ���)
// @param �ϒ�����
// @remarks���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

// @brief �E�B���h�E�v���V�[�W��
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//�E�B���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);//OS�ɑ΂��āu�������̃A�v���͏I�����v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

#ifdef _DEBUG
int main() {
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif

	//�ȉ��������Ă����Ȃ���COM���|��������WIC������ɓ��삵�Ȃ����Ƃ�����܂��B
	//(�����Ȃ��Ă������Ƃ�������܂�)
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);

	DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//�E�B���h�E�N���X�������o�^
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DirectXTest");//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	w.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&w);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0,0, window_width, window_height };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	//�E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,//�N���X���w��
		_T("Chapter07"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		w.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^

#ifdef _DEBUG
	//�f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif
	//DirectX12�܂�菉����
	//�t�B�[�`�����x����
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = l;
			break;
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�v���C�I���e�B���Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�����̓R�}���h���X�g�ƍ��킹�Ă�������
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));//�R�}���h�L���[����

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//�����_�[�^�[�Q�b�g�r���[�Ȃ̂œ��RRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//���Ɏw��Ȃ�
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;


	for (size_t i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �[�x�o�b�t�@�[�֘A //
		// �[�x�o�b�t�@�̍쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;             // 2�����̃e�N�X�`���f�[�^
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;                                       // �e�N�X�`���z��ł��A3D�e�N�X�`���ł��Ȃ�
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;                             // �[�x�l�������ݗp�t�H�[�}�b�g
	depthResDesc.SampleDesc.Count = 1;                                       // �T���v����1�s�N�Z��������1��
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;            // �f�v�X�X�e���V���Ƃ��Ďg�p

	// �[�x�l�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;                            // DEFAULT�Ȃ̂�UNKWON�ł悢
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// ���̃N���A�r���[���d�v�ȈӖ�������
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;                               // �[��1.0f�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;                          // 32�r�b�gfloat�l�Ƃ��ăN���A

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,                    // �k�x�n�������ݗp�Ɏg�p
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));

	// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;                          // �[�x�r���[��1��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;       // �f�v�X�X�e���V���r���[�Ƃ��Ďg��
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;                  // �[�x�l��32�r�b�g���悤
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;   // 2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;                     // �t���O�͓��ɂȂ�
	_dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());


	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);//�E�B���h�E�\��

	struct Vertex
	{
		XMFLOAT3 pos;         //XYZ���W
		XMFLOAT2 uv;          //uv���W
	};

	// PMD�w�b�_�[�\����
	struct PMDHeader
	{
		float version;        // ��F00 00 80 3F == 1.00
		char model_name[20];  // ���f����
		char comment[256];    // ���f���R�����g
	};

	char signature[3] = {};   // �V�O�l�`��
	PMDHeader pmdheader = {};
	FILE* fp;
	auto err = fopen_s(&fp, "Model/�����~�N.pmd", "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);
	constexpr size_t pmdvertex_size = 38;                             // ���_1������̃T�C�Y

	unsigned int vertNum;                        // ���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);

	std::vector<unsigned char> vertices(vertNum * pmdvertex_size);     // �o�b�t�@�[�̊m��
	fread(vertices.data(), vertices.size(), 1, fp);                   // �ǂݍ���

	unsigned int indicesNum;                  // �C���f�b�N�X��
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	//UPLOAD(�m�ۂ͉\)
	ID3D12Resource* vertBuff = nullptr;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();   //�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = vertices.size();                       //�S�o�C�g��
	vbView.StrideInBytes = pmdvertex_size;                      //1���_������̃o�C�g��
	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);
	fclose(fp);

	ID3D12Resource* idxBuff = nullptr;
	//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
	//OK���Ǝv���܂��B
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);



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
		&_vsBlob, &_errorBlob);                           // �G���[����_errorBlob�Ƀ��b�Z�[�W������

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
		&_psBlob, &_errorBlob);							  // �G���[����_errorBlob�Ƀ��b�Z�[�W������

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


	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//�n���V�F�[�_�A�h���C���V�F�[�_�A�W�I���g���V�F�[�_�͐ݒ肵�Ȃ�
	gpipeline.HS.BytecodeLength = 0;
	gpipeline.HS.pShaderBytecode = nullptr;
	gpipeline.DS.BytecodeLength = 0;
	gpipeline.DS.pShaderBytecode = nullptr;
	gpipeline.GS.BytecodeLength = 0;
	gpipeline.GS.pShaderBytecode = nullptr;

	// ���X�^���C�Y(RS)
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//���g��h��Ԃ�
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthClipEnable = true;//�[�x�����̃N���b�s���O�͗L����
	gpipeline.RasterizerState.MultisampleEnable = false;//�܂��A���`�F���͎g��Ȃ�
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	// OutputMerger����
	// �����_�[�^�[�Q�b�g
	gpipeline.NumRenderTargets = 1;                         // ���̃^�[�Q�b�g���Ɛݒ肷��t�H�[�}�b�g����
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;   // ��v������

	// �[�x�X�e���V��
	gpipeline.DepthStencilState.DepthEnable = true;                            // �[�x�o�b�t�@�[���g��
	gpipeline.DepthStencilState.StencilEnable = false;
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;        // �[�x�l�������������̗p
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;   // �s�N�Z���`�掞�ɐ[�x�o�b�t�@�[�ɐ[�x�l����������
	// �u�����h�ݒ�
	//gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	gpipeline.BlendState.RenderTarget->BlendEnable = true;
	gpipeline.BlendState.RenderTarget->SrcBlend = D3D12_BLEND_SRC_ALPHA;
	gpipeline.BlendState.RenderTarget->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	gpipeline.BlendState.RenderTarget->BlendOp = D3D12_BLEND_OP_ADD;

	gpipeline.NodeMask = 0;
	gpipeline.SampleDesc.Count = 1;                    //�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;                  //�N�I���e�B�͍Œ�
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;  //���g��0xffffffff
	gpipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//�ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//�ЂƂ܂��_�����Z�͎g�p���Ȃ�
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.InputLayout.pInputElementDescs = inputLayout;      //���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);   //���C�A�E�g�z��


	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;  //�X�g���b�v���̃J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; //�O�p�`�ō\��

	gpipeline.NumRenderTargets = 1;//���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

	//renderTargetBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	//renderTargetBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	//renderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;


	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[1] = {};                 //�萔��1��
	descTblRange[0].NumDescriptors = 1;                          //�萔�ЂƂ�
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; //��ʂ͒萔
	descTblRange[0].BaseShaderRegister = 0;                      //0�ԃX���b�g����
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange[0]; //�f�X�N���v�^�����W�̃A�h���X
	rootparam.DescriptorTable.NumDescriptorRanges = 1;              //�f�X�N���v�^�����W��
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;       //�S�ẴV�F�[�_���猩����

	rootSignatureDesc.pParameters = &rootparam;                     //���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 1;                            //���[�g�p�����[�^��

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                //���J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                //�c�J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                //���s�J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; //�{�[�_�[�̎��͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;                   //��Ԃ��Ȃ�(�j�A���X�g�l�C�o�[)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                                //�~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;                                             //�~�b�v�}�b�v�ŏ��l
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;              //�I�[�o�[�T���v�����O�̍ۃ��T���v�����O���Ȃ��H
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;          //�s�N�Z���V�F�[�_����̂݉�

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &_errorBlob);
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;      //�o�͐�̕�(�s�N�Z����)
	viewport.Height = window_height;    //�o�͐�̍���(�s�N�Z����)
	viewport.TopLeftX = 0;              //�o�͐�̍�����WX
	viewport.TopLeftY = 0;              //�o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;           //�[�x�ő�l
	viewport.MinDepth = 0.0f;           //�[�x�ŏ��l


	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;                                   //�؂蔲������W
	scissorrect.left = 0;                                  //�؂蔲�������W
	scissorrect.right = scissorrect.left + window_width;   //�؂蔲���E���W
	scissorrect.bottom = scissorrect.top + window_height;  //�؂蔲�������W

	// �V�F�[�_�[���ɓn�����߂̊�{�I�ȍs��f�[�^
	struct MatricesData
	{
		XMMATRIX world;       // ���f���{�̂���]��������ړ��������邷��s��
		XMMATRIX viewproj;    // �r���[�ƃv���W�F�N�V���������s��
	};

	// �萔�o�b�t�@�쐬
	// ���[���h�s��
	auto worldMat = XMMatrixRotationY(XM_PIDIV4);

	// �r���[�s��
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// �v���W�F�N�V�����s��
	auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,//��p��90��
		static_cast<float>(window_width) / static_cast<float>(window_height),//�A�X��
		1.0f,//�߂���
		100.0f//������
	);
	ID3D12Resource* constBuff = nullptr;
	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);

	MatricesData* mapMatrix;                                      //�}�b�v��������|�C���^
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);  //�}�b�v
	// �s��̓��e���R�s�[
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;

	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
	descHeapDesc.NodeMask = 0;//�}�X�N��0
	descHeapDesc.NumDescriptors = 2;//SRV1��CBV1��
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//����

	//�f�X�N���v�^�̐擪�n���h�����擾���Ă���
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
	//�萔�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);




	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
	while (true) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�����A�v���P�[�V�������I�����Ď���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}

		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;
		angle += 0.005f;


		//DirectX����
		//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		_cmdList->SetPipelineState(_pipelinestate);

		//�����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		_cmdList->ResourceBarrier(1, &BarrierDesc);
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

		//��ʃN���A
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f }; // ���F
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		// �[�x�o�b�t�@�[�̃N���A
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//���߂̃N���[�Y
		_cmdList->Close();



		//�R�}���h���X�g�̎��s
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		////�҂�
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}


		//�t���b�v
		_swapchain->Present(1, 0);
		_cmdAllocator->Reset();//�L���[���N���A
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//�ĂуR�}���h���X�g�����߂鏀��

	}
	//�����N���X�g��񂩂�o�^�������Ă�
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}