#include "DirectX3DHelper.h"

#include "pixel_shader.h"
#include "vertex_shader.h"

namespace {
	struct vertex_t {
		FLOAT position[3];
		FLOAT color[4];
	};

	size_t const VERTEX_SIZE = sizeof(vertex_t);

	vertex_t triangleVertices[] = {
	    {0.0f, 1.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
	    {1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f},
	    {-1.0f, -1.0f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}};

	size_t const VERTEX_BUFFER_SIZE = sizeof(triangleVertices);

	size_t const NUM_VERTICES = VERTEX_BUFFER_SIZE / sizeof(vertex_t);
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
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
	if (GetTickCount64() / 1000 % 2) {
		float newClearColor[] = {0.9f, 0.5f, 0.0f, 1.0f};
		std::copy(newClearColor, newClearColor + 4, clearColor);
	}
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(3, 1, 0, 0);

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

void DirectX3DHelper::draw() {
	populateCommandList();

	ID3D12CommandList * ppCommandLists[] = {commandList.Get()};
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(swapChain->Present(1, 0));

	waitForPreviousFrame();
}

void DirectX3DHelper::loadPipeline() {
	// Factory and device
	ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

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

	// Descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
	    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV, .NumDescriptors = frameCount, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE};
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap)));
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

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
	// Empty root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{
	    .NumParameters = 0,
	    .pParameters = nullptr,
	    .NumStaticSamplers = 0,
	    .pStaticSamplers = nullptr,
	    .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

	ComPtr<ID3DBlob> signature{};
	ComPtr<ID3DBlob> error{};
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(
	    0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)
	));

	// Pipeline state
	ComPtr<ID3DBlob> vertexShader{};
	ComPtr<ID3DBlob> pixelShader{};

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
	    {.SemanticName = "POSITION",
	     .SemanticIndex = 0,
	     .Format = DXGI_FORMAT_R32G32B32_FLOAT,
	     .InputSlot = 0,
	     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
	     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
	     .InstanceDataStepRate = 0},
	    {.SemanticName = "COLOR",
	     .SemanticIndex = 0,
	     .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
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
	    .InputLayout = {inputElementDescs, _countof(inputElementDescs)},
	    .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
	    .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	    .NumRenderTargets = 1,
	    .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
	    .SampleDesc = {.Count = 1, .Quality = 0},
	};

	ThrowIfFailed(device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));

	// Command list
	ThrowIfFailed(device->CreateCommandList(
	    0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)
	));

	ThrowIfFailed(commandList->Close());

	// Vertex buffer
	D3D12_HEAP_PROPERTIES heapProperties{
	    .Type = D3D12_HEAP_TYPE_UPLOAD,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1};

	D3D12_RESOURCE_DESC resourceDesc = {
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
	    &heapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&vertexBuffer)
	));

	UINT8 * pVertexDataBegin;
	D3D12_RANGE readRange{0, 0};
	ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	vertexBuffer->Unmap(0, nullptr);

	vertexBufferView = {
	    .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
	    .SizeInBytes = VERTEX_BUFFER_SIZE,
	    .StrideInBytes = VERTEX_SIZE,
	};

	// Synchronization objects
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	waitForPreviousFrame();
}

DirectX3DHelper::~DirectX3DHelper() {
	waitForPreviousFrame();
	CloseHandle(fenceEvent);
}
