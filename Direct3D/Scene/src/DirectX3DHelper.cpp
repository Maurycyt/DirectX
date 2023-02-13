#include "DirectX3DHelper.h"

#include "DirectXMath.h"
#include "pixel_shader.h"
#include "vertex_shader.h"

#include <fstream>
#include <numbers>
#include <sstream>
#include <vector>

using namespace DirectX;

namespace {
	struct vertex_t {
		FLOAT position[3];
		FLOAT normal[3];
		FLOAT tex_coord[2];
	};

	size_t const VERTEX_SIZE = sizeof(vertex_t);

	vertex_t * triangleVertices;

	size_t VERTEX_BUFFER_SIZE{};
	size_t NUM_VERTICES{};

	struct vs_const_buffer_t {
		XMFLOAT4X4 matWorldViewProj;
		XMFLOAT4X4 matWorldView;
		XMFLOAT4X4 matView;
		XMFLOAT4 colLight;
		XMFLOAT4 dirLight;
		XMFLOAT4 padding[2];
	};

	vs_const_buffer_t vsConstBuffer;

	size_t const CONSTANT_BUFFER_SIZE = sizeof(vsConstBuffer);

	UINT const bmpPixelSize{4};
	UINT bmpWidth{}, bmpHeight{};
	BYTE * bmpBytes{};
} // namespace

DirectX3DHelper::DirectX3DHelper() = default;

DirectX3DHelper::DirectX3DHelper(HWND hwnd) : hwnd(hwnd) {
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}

	ThrowIfFailed(!GetClientRect(hwnd, &clientRect));
	scissorRect = clientRect;
	viewport = {
	    .TopLeftX = 0.0f,
	    .TopLeftY = 0.0f,
	    .Width = float(clientRect.right),
	    .Height = float(clientRect.bottom),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f};

	loadPipeline();
	loadAssets();
}

void DirectX3DHelper::waitForPreviousFrame() {
	const UINT64 oldFenceValue = fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), oldFenceValue));
	fenceValue++;

	if (fence->GetCompletedValue() < oldFenceValue) {
		ThrowIfFailed(fence->SetEventOnCompletion(oldFenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void DirectX3DHelper::populateCommandList() {
	// Command list allocators can only be reset when the associated
	// command lists have finished execution on the GPU; apps should use
	// fences to determine GPU execution progress.
	ThrowIfFailed(commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command
	// list, that command list can then be reset at any time and must be before
	// re-recording.
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	// Set necessary state.
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap * descriptorHeaps[] = {cbvDescriptorHeap.Get()};
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle = cbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	commandList->SetGraphicsRootDescriptorTable(0, gpuDescHandle);
	gpuDescHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(1, gpuDescHandle);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// Indicate that the back buffer will be used as a render target.
	auto barrier1 = D3D12_RESOURCE_BARRIER({
	    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	    .Transition =
	        {
	            .pResource = renderTargets[frameIndex].Get(),
	            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
	            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
	            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
	        },
	});
	commandList->ResourceBarrier(1, &barrier1);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += frameIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record commands.
	float clearColor[] = {0.05f, 0.05f, 0.05f, 1.0f};
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(NUM_VERTICES, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	auto barrier2 = D3D12_RESOURCE_BARRIER({
	    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	    .Transition =
	        {
	            .pResource = renderTargets[frameIndex].Get(),
	            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
	            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
	            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
	        },
	});
	commandList->ResourceBarrier(1, &barrier2);

	ThrowIfFailed(commandList->Close());
}

Position DirectX3DHelper::getPosition(const POINT cursorPos) const {
	static const float pi = std::numbers::pi_v<float>;
	static const float baseSpeed = 0.05f;
	static Position position = {.x = 0.5, .y = 1.5, .z = -10.0, .lon = 0.0, .lat = 0.0};

	position.lon -= (float(cursorPos.x) - float(scissorRect.right) / 2) * 2 * pi / 2000; // One rotation per 2000 px
	position.lat -=
	    (float(cursorPos.y) - float(scissorRect.bottom) / 2) * pi / 2 / 500; // From horizontal to vertical in 500 px
	position.lat = min(pi / 2, max(-pi / 2, position.lat));                  // Keep in bounds

	float speed = baseSpeed;
	if (GetAsyncKeyState(VK_CONTROL)) {
		speed *= 5.0;
	}

	if (GetAsyncKeyState('W')) {
		position.z += speed * cosf(position.lon);
		position.x -= speed * sinf(position.lon);
	}
	if (GetAsyncKeyState('S')) {
		position.z -= speed * cosf(position.lon);
		position.x += speed * sinf(position.lon);
	}
	if (GetAsyncKeyState('A')) {
		position.z -= speed * sinf(position.lon);
		position.x -= speed * cosf(position.lon);
	}
	if (GetAsyncKeyState('D')) {
		position.z += speed * sinf(position.lon);
		position.x += speed * cosf(position.lon);
	}
	if (GetAsyncKeyState(' ')) {
		position.y += speed;
	}
	if (GetAsyncKeyState(VK_LSHIFT)) {
		position.y -= speed;
	}

	SetCursorPos(scissorRect.right / 2, scissorRect.bottom / 2);

	return position;
}

void DirectX3DHelper::setWVPMatrix(const POINT cursorPos) {
	Position position = getPosition(cursorPos);

	XMMATRIX wMatrix = XMMatrixIdentity(); // XMMatrixRotationY(angle);
	XMMATRIX vMatrix = XMMatrixMultiply(
	    XMMatrixTranslation(-position.x, -position.y, -position.z),
	    XMMatrixMultiply(XMMatrixRotationY(position.lon), XMMatrixRotationX(position.lat))
	);
	XMMATRIX pMatrix = XMMatrixPerspectiveFovLH(45.0f, viewport.Width / viewport.Height, 0.001f, 100.0f);
	XMMATRIX wvMatrix = XMMatrixMultiply(wMatrix, vMatrix);
	XMMATRIX wvpMatrix = XMMatrixMultiply(wvMatrix, pMatrix);
	XMStoreFloat4x4(&vsConstBuffer.matWorldViewProj, XMMatrixTranspose(wvpMatrix));
	XMStoreFloat4x4(&vsConstBuffer.matWorldView, XMMatrixTranspose(wvMatrix));
	XMStoreFloat4x4(&vsConstBuffer.matView, XMMatrixTranspose(vMatrix));
	vsConstBuffer.colLight = {1.0, 1.0, 1.0, 1.0}; // white light
	vsConstBuffer.dirLight = {
	    -sinf(position.lon) * cosf(position.lat), sinf(position.lat), cosf(position.lon) * cosf(position.lat), 0.0};
	memcpy(pcBufferDataBegin, &vsConstBuffer, CONSTANT_BUFFER_SIZE);
}

void DirectX3DHelper::draw(const POINT cursorPos) {
	if (!active) {
		return;
	}

	populateCommandList();
	setWVPMatrix(cursorPos);

	ID3D12CommandList * ppCommandLists[] = {commandList.Get()};
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(swapChain->Present(1, 0));

	waitForPreviousFrame();
}

void DirectX3DHelper::loadPipeline() {
	// Debug layer
	UINT dxgi_factory_flag = 0;
	ComPtr<ID3D12Debug> debug_controller = nullptr;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
	debug_controller->EnableDebugLayer();
	dxgi_factory_flag |= DXGI_CREATE_FACTORY_DEBUG;

	// Factory and device
	ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&factory)));
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

	// WIC Imaging Factory
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WICFactory)));

	// Command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc{.Type = D3D12_COMMAND_LIST_TYPE_DIRECT, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE};
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	// Swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{
	    .Width = 0,
	    .Height = 0,
	    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	    .SampleDesc = {.Count = 1},
	    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	    .BufferCount = frameCount,
	    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD};

	ComPtr<IDXGISwapChain1> babySwapChain;
	ThrowIfFailed(
	    factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &babySwapChain)
	);
	ThrowIfFailed(babySwapChain.As(&swapChain));

	ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Vertex buffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{
	    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV, .NumDescriptors = frameCount, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE};
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap)));
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Constant buffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC cbvDescriptorHeapDesc{
	    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	    .NumDescriptors = 2,
	    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	    .NodeMask = 0};
	device->CreateDescriptorHeap(&cbvDescriptorHeapDesc, IID_PPV_ARGS(&cbvDescriptorHeap));

	// Depth buffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc{
	    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
	    .NumDescriptors = 1,
	    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	    .NodeMask = 0};
	device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));

	// Frame resources
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (unsigned int i = 0; i < frameCount; i++) {
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, cpuDescriptorHandle);
		cpuDescriptorHandle.ptr = cpuDescriptorHandle.ptr + rtvDescriptorSize;
	}

	// Command allocator
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
}

void DirectX3DHelper::loadAssets() {
	// Scene
	parseObjectFile(TEXT("../assets/Scene64M.obj"));
	loadBitmapFromFile(TEXT("../assets/BakedTexture64M.png"), bmpWidth, bmpHeight, &bmpBytes);

	// Root signature with constant buffer
	D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {
	    {.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
	     .NumDescriptors = 1,
	     .BaseShaderRegister = 0,
	     .RegisterSpace = 0,
	     .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND},
	    {.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	     .NumDescriptors = 1,
	     .BaseShaderRegister = 0,
	     .RegisterSpace = 0,
	     .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}};

	D3D12_ROOT_PARAMETER rootParameters[] = {
	    {.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
	     .DescriptorTable = {1, &descriptorRanges[0]},
	     .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX},
	    {.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
	     .DescriptorTable = {1, &descriptorRanges[1]},
	     .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL}};

	D3D12_STATIC_SAMPLER_DESC textureSamplerDesc = {
	    .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_FILTER_ANISOTROPIC
	    .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP, //_MODE_MIRROR, _MODE_CLAMP, _MODE_BORDER
	    .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
	    .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
	    .MipLODBias = 0,
	    .MaxAnisotropy = 0,
	    .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
	    .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
	    .MinLOD = 0.0f,
	    .MaxLOD = D3D12_FLOAT32_MAX,
	    .ShaderRegister = 0,
	    .RegisterSpace = 0,
	    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL};

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{
	    .NumParameters = _countof(rootParameters),
	    .pParameters = rootParameters,
	    .NumStaticSamplers = 1,
	    .pStaticSamplers = &textureSamplerDesc,
	    .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};

	ComPtr<ID3DBlob> signature{};
	ComPtr<ID3DBlob> error{};
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(
	    0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)
	));

	// Pipeline state
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
	    {.SemanticName = "POSITION",
	     .SemanticIndex = 0,
	     .Format = DXGI_FORMAT_R32G32B32_FLOAT,
	     .InputSlot = 0,
	     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
	     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
	     .InstanceDataStepRate = 0},
	    {.SemanticName = "NORMAL",
	     .SemanticIndex = 0,
	     .Format = DXGI_FORMAT_R32G32B32_FLOAT,
	     .InputSlot = 0,
	     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
	     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
	     .InstanceDataStepRate = 0},
	    {.SemanticName = "TEXCOORD",
	     .SemanticIndex = 0,
	     .Format = DXGI_FORMAT_R32G32_FLOAT,
	     .InputSlot = 0,
	     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
	     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
	     .InstanceDataStepRate = 0}};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{
	    .pRootSignature = rootSignature.Get(),
	    .VS = {vs_main, sizeof(vs_main)},
	    .PS = {ps_main, sizeof(ps_main)},
	    .BlendState =
	        D3D12_BLEND_DESC{
	            .AlphaToCoverageEnable = FALSE,
	            .IndependentBlendEnable = FALSE,
	            .RenderTarget =
	                {{.BlendEnable = FALSE,
	                  .LogicOpEnable = FALSE,
	                  .SrcBlend = D3D12_BLEND_ONE,
	                  .DestBlend = D3D12_BLEND_ZERO,
	                  .BlendOp = D3D12_BLEND_OP_ADD,
	                  .SrcBlendAlpha = D3D12_BLEND_ONE,
	                  .DestBlendAlpha = D3D12_BLEND_ZERO,
	                  .BlendOpAlpha = D3D12_BLEND_OP_ADD,
	                  .LogicOp = D3D12_LOGIC_OP_NOOP,
	                  .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL}}},
	    .SampleMask = UINT_MAX,
	    .RasterizerState =
	        D3D12_RASTERIZER_DESC{
	            .FillMode = D3D12_FILL_MODE_SOLID,
	            .CullMode = D3D12_CULL_MODE_BACK,
	            .FrontCounterClockwise = FALSE,
	            .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
	            .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
	            .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
	            .DepthClipEnable = TRUE,
	            .MultisampleEnable = FALSE,
	            .AntialiasedLineEnable = FALSE,
	            .ForcedSampleCount = 0,
	            .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF},
	    .DepthStencilState =
	        {.DepthEnable = TRUE,
	         .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
	         .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
	         .StencilEnable = FALSE,
	         .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
	         .StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK,
	         .FrontFace =
	             {.StencilFailOp = D3D12_STENCIL_OP_KEEP,
	              .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
	              .StencilPassOp = D3D12_STENCIL_OP_KEEP,
	              .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS},
	         .BackFace =
	             {.StencilFailOp = D3D12_STENCIL_OP_KEEP,
	              .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
	              .StencilPassOp = D3D12_STENCIL_OP_KEEP,
	              .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS}},
	    .InputLayout = {inputElementDescs, _countof(inputElementDescs)},
	    .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
	    .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	    .NumRenderTargets = 1,
	    .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
	    .DSVFormat = DXGI_FORMAT_D32_FLOAT,
	    .SampleDesc = {.Count = 1, .Quality = 0}};

	ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));

	// Command list
	ThrowIfFailed(device->CreateCommandList(
	    0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)
	));

	ThrowIfFailed(commandList->Close());

	// Vertex buffer
	D3D12_HEAP_PROPERTIES rtvHeapProperties{
	    .Type = D3D12_HEAP_TYPE_UPLOAD,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1};

	D3D12_RESOURCE_DESC rtvResourceDesc{
	    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	    .Alignment = 0,
	    .Width = VERTEX_BUFFER_SIZE,
	    .Height = 1,
	    .DepthOrArraySize = 1,
	    .MipLevels = 1,
	    .Format = DXGI_FORMAT_UNKNOWN,
	    .SampleDesc = {.Count = 1, .Quality = 0},
	    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	    .Flags = D3D12_RESOURCE_FLAG_NONE};

	ThrowIfFailed(device->CreateCommittedResource(
	    &rtvHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &rtvResourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&vertexBuffer)
	));

	void * pVertexDataBegin;
	D3D12_RANGE vertexDataReadRange{0, 0};
	ThrowIfFailed(vertexBuffer->Map(0, &vertexDataReadRange, &pVertexDataBegin));
	memcpy(pVertexDataBegin, triangleVertices, VERTEX_BUFFER_SIZE);
	vertexBuffer->Unmap(0, nullptr);

	vertexBufferView = {
	    .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
	    .SizeInBytes = VERTEX_BUFFER_SIZE,
	    .StrideInBytes = VERTEX_SIZE,
	};

	// Constant buffer
	D3D12_HEAP_PROPERTIES cbvHeapProperties{
	    .Type = D3D12_HEAP_TYPE_UPLOAD,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1};

	D3D12_RESOURCE_DESC cbvResourceDesc{
	    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	    .Alignment = 0,
	    .Width = CONSTANT_BUFFER_SIZE,
	    .Height = 1,
	    .DepthOrArraySize = 1,
	    .MipLevels = 1,
	    .Format = DXGI_FORMAT_UNKNOWN,
	    .SampleDesc = {.Count = 1, .Quality = 0},
	    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	    .Flags = D3D12_RESOURCE_FLAG_NONE};

	ThrowIfFailed(device->CreateCommittedResource(
	    &cbvHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &cbvResourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&constantBuffer)
	));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
	    .BufferLocation = constantBuffer->GetGPUVirtualAddress(), .SizeInBytes = CONSTANT_BUFFER_SIZE};

	device->CreateConstantBufferView(&cbvDesc, cbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	XMStoreFloat4x4((XMFLOAT4X4 *)(&vsConstBuffer), XMMatrixIdentity());

	D3D12_RANGE cBufferReadRange{0, 0};
	ThrowIfFailed(constantBuffer->Map(0, &cBufferReadRange, &pcBufferDataBegin));
	memcpy(pcBufferDataBegin, &vsConstBuffer, CONSTANT_BUFFER_SIZE);

	// Texture buffer
	D3D12_HEAP_PROPERTIES texHeapProperties = {
	    .Type = D3D12_HEAP_TYPE_DEFAULT,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1};

	D3D12_RESOURCE_DESC texResourceDesc = {
	    .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	    .Alignment = 0,
	    .Width = bmpWidth,
	    .Height = bmpHeight,
	    .DepthOrArraySize = 1,
	    .MipLevels = 1,
	    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	    .SampleDesc = {.Count = 1, .Quality = 0},
	    .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
	    .Flags = D3D12_RESOURCE_FLAG_NONE};

	device->CreateCommittedResource(
	    &texHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &texResourceDesc,
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    nullptr,
	    IID_PPV_ARGS(&textureBuffer)
	);

	ComPtr<ID3D12Resource> texUploadBuffer{};
	UINT64 requiredSize = 0;
	auto Desc = textureBuffer->GetDesc();
	ComPtr<ID3D12Device> pDevice{};
	textureBuffer->GetDevice(__uuidof(*pDevice.Get()), (void **)(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, 0, 1, 0, nullptr, nullptr, nullptr, &requiredSize);
	pDevice->Release();
	pDevice = nullptr;

	D3D12_HEAP_PROPERTIES texUploadHeapProperties = {
	    .Type = D3D12_HEAP_TYPE_UPLOAD,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1};

	D3D12_RESOURCE_DESC texUploadResourceDesc = {
	    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	    .Alignment = 0,
	    .Width = requiredSize,
	    .Height = 1,
	    .DepthOrArraySize = 1,
	    .MipLevels = 1,
	    .Format = DXGI_FORMAT_UNKNOWN,
	    .SampleDesc = {.Count = 1, .Quality = 0},
	    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	    .Flags = D3D12_RESOURCE_FLAG_NONE};

	device->CreateCommittedResource(
	    &texUploadHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &texUploadResourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&textureUploadBuffer)
	);

	D3D12_SUBRESOURCE_DATA texture_data = {
	    .pData = bmpBytes,
	    .RowPitch = LONG_PTR(bmpWidth * bmpPixelSize),
	    .SlicePitch = LONG_PTR(bmpWidth * bmpHeight * bmpPixelSize)};

	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	UINT const MAX_SUBRESOURCES = 1;
	requiredSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MAX_SUBRESOURCES];
	UINT NumRows[MAX_SUBRESOURCES];
	UINT64 RowSizesInBytes[MAX_SUBRESOURCES];

	Desc = textureBuffer->GetDesc();
	textureBuffer->GetDevice(__uuidof(*pDevice.Get()), (void **)(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, 0, 1, 0, Layouts, NumRows, RowSizesInBytes, &requiredSize);
	pDevice->Release();
	pDevice = nullptr;

	BYTE * mapTexData = nullptr;
	textureUploadBuffer->Map(0, nullptr, (void **)(&mapTexData));

	D3D12_MEMCPY_DEST DestData = {
	    .pData = mapTexData + Layouts[0].Offset,
	    .RowPitch = Layouts[0].Footprint.RowPitch,
	    .SlicePitch = SIZE_T(Layouts[0].Footprint.RowPitch) * SIZE_T(NumRows[0])};

	for (UINT z = 0; z < Layouts[0].Footprint.Depth; ++z) {
		auto pDestSlice = (UINT8 *)(DestData.pData) + DestData.SlicePitch * z;
		auto pSrcSlice = (const UINT8 *)(texture_data.pData) + texture_data.SlicePitch * LONG_PTR(z);
		for (UINT y = 0; y < NumRows[0]; ++y) {
			memcpy(
			    pDestSlice + DestData.RowPitch * y,
			    pSrcSlice + texture_data.RowPitch * LONG_PTR(y),
			    SIZE_T(RowSizesInBytes[0])
			);
		}
	}

	textureUploadBuffer->Unmap(0, nullptr);

	D3D12_TEXTURE_COPY_LOCATION Dst = {
	    .pResource = textureBuffer.Get(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, .SubresourceIndex = 0};

	D3D12_TEXTURE_COPY_LOCATION Src = {
	    .pResource = textureUploadBuffer.Get(),
	    .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
	    .PlacedFootprint = Layouts[0]};

	commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	D3D12_RESOURCE_BARRIER texUploadResourceBarrier = {
	    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	    .Transition =
	        {.pResource = textureBuffer.Get(),
	         .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
	         .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
	         .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
	};
	commandList->ResourceBarrier(1, &texUploadResourceBarrier);
	commandList->Close();
	ID3D12CommandList * pCmdList = commandList.Get();
	commandQueue->ExecuteCommandLists(1, &pCmdList);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
	    .Format = texResourceDesc.Format,
	    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
	    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	    .Texture2D = {.MostDetailedMip = 0, .MipLevels = 1, .PlaneSlice = 0, .ResourceMinLODClamp = 0.0f},
	};

	D3D12_CPU_DESCRIPTOR_HANDLE texCPUDescHandle = cbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	texCPUDescHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CreateShaderResourceView(textureBuffer.Get(), &srvDesc, texCPUDescHandle);

	// Depth buffer
	D3D12_HEAP_PROPERTIES dsvHeapProperties{
	    .Type = D3D12_HEAP_TYPE_DEFAULT,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1};

	D3D12_RESOURCE_DESC dsvResourceDesc{
	    .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	    .Alignment = 0,
	    .Width = UINT64(scissorRect.right),
	    .Height = UINT(scissorRect.bottom),
	    .DepthOrArraySize = 1,
	    .MipLevels = 0,
	    .Format = DXGI_FORMAT_D32_FLOAT,
	    .SampleDesc = {.Count = 1, .Quality = 0},
	    .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
	    .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};

	D3D12_CLEAR_VALUE dsvClearValue{.Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = {.Depth = 1.0f, .Stencil = 0}};

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
	    .Format = DXGI_FORMAT_D32_FLOAT,
	    .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
	    .Flags = D3D12_DSV_FLAG_NONE,
	    .Texture2D = {}};

	ThrowIfFailed(device->CreateCommittedResource(
	    &dsvHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &dsvResourceDesc,
	    D3D12_RESOURCE_STATE_DEPTH_WRITE,
	    &dsvClearValue,
	    IID_PPV_ARGS(&depthBuffer)
	));

	device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Synchronization objects
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	waitForPreviousFrame();
}

void DirectX3DHelper::activate() {
	active = true;
}

void DirectX3DHelper::deactivate() {
	active = false;
}

DirectX3DHelper::~DirectX3DHelper() {
	waitForPreviousFrame();
	CloseHandle(fenceEvent);
	delete[] bmpBytes;
}

void DirectX3DHelper::loadBitmapFromFile(PCWSTR uri, UINT & width, UINT & height, BYTE ** ppBits) {
	ComPtr<IWICBitmapDecoder> pDecoder{};
	ComPtr<IWICBitmapFrameDecode> pSource{};
	ComPtr<IWICFormatConverter> pConverter{};

	ThrowIfFailed(
	    WICFactory->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder)
	);
	ThrowIfFailed(pDecoder->GetFrame(0, &pSource));
	ThrowIfFailed(WICFactory->CreateFormatConverter(&pConverter));
	ThrowIfFailed(pConverter->Initialize(
	    pSource.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut
	));
	ThrowIfFailed(pConverter->GetSize(&width, &height));
	*ppBits = new BYTE[4 * width * height];
	ThrowIfFailed(pConverter->CopyPixels(nullptr, 4 * width, 4 * width * height, *ppBits));
}

void DirectX3DHelper::parseObjectFile(PCWSTR uri) {
	std::vector<XMFLOAT3> vertices{1};
	std::vector<XMFLOAT3> normals{1};
	std::vector<XMFLOAT2> texCoords{1};

	std::vector<vertex_t> result{};

	std::ifstream objFile{uri};

	std::string line;
	while (std::getline(objFile, line)) {
		std::stringstream linestream{line};
		std::string word;
		linestream >> word;
		if (word == "v") {
			XMFLOAT3 vertex{};
			linestream >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		} else if (word == "vn") {
			XMFLOAT3 normal{};
			linestream >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		} else if (word == "vt") {
			XMFLOAT2 texCoord{};
			linestream >> texCoord.x >> texCoord.y;
			texCoords.push_back(texCoord);
		} else if (word == "f") {
			size_t newVertexIndex = result.size();
			for (int i = 0; i < 3; i++) {
				int v, n, t;
				char slash;
				linestream >> v >> slash >> t >> slash >> n;
				result.push_back(
				    {-vertices[v].x,
				     vertices[v].y,
				     vertices[v].z,
				     -normals[n].x,
				     normals[n].y,
				     normals[n].z,
				     texCoords[t].x,
				     -texCoords[t].y}
				);
			}
			std::swap(result[newVertexIndex], result[newVertexIndex + 1]);
		}
	}

	NUM_VERTICES = result.size();
	VERTEX_BUFFER_SIZE = NUM_VERTICES * sizeof(vertex_t);
	triangleVertices = new vertex_t[NUM_VERTICES];
	memcpy(triangleVertices, result.data(), VERTEX_BUFFER_SIZE);
}
