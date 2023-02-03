#include "DirectX3DHelper.h"

#include "DirectXMath.h"
#include "pixel_shader.h"
#include "vertex_shader.h"
#include <numbers>

using namespace DirectX;

namespace {
	struct vertex_t {
		FLOAT position[3];
		FLOAT normal[3];
		FLOAT color[4];
	};

	size_t const VERTEX_SIZE = sizeof(vertex_t);

	size_t const NUM_DIRECTIONS = 4;
	size_t const NUM_LEVELS = 3;
	float const levelBoundaries[NUM_LEVELS + 1] = {-1.5, 0.5, 1.5, 2.0};
	float const radii[NUM_LEVELS] = {1.0, 0.5, 0.25};

	vertex_t triangleVertices[2 * NUM_DIRECTIONS * NUM_LEVELS * 3] = {};

	void setTriangleVertices() {
		for (size_t level = 0; level < NUM_LEVELS; level++) {
			float bottom = levelBoundaries[level];
			float top = levelBoundaries[level + 1];
			float radius = radii[level];
			for (size_t direction = 0; direction < NUM_DIRECTIONS; direction++) {
				auto angle = float(direction * std::numbers::pi / 4);
				float s = radius * sinf(angle), c = radius * cosf(angle);
				size_t firstVertexIndex = level * NUM_DIRECTIONS * 2 * 3 + direction * 2 * 3;
				triangleVertices[firstVertexIndex + 0] = {-c, bottom, -s, s, 0.0, -c, 0.0, 1.0, 0.0, 1.0};
				triangleVertices[firstVertexIndex + 1] = {0.0, top, 0.0, s, 0.0, -c, 1.0, 1.0, 1.0, 1.0};
				triangleVertices[firstVertexIndex + 2] = {c, bottom, s, s, 0.0, -c, 0.0, 1.0, 0.0, 1.0};

				triangleVertices[firstVertexIndex + 3] = {c, bottom, s, -s, 0.0, c, 0.0, 1.0, 0.0, 1.0};
				triangleVertices[firstVertexIndex + 4] = {0.0, top, 0.0, -s, 0.0, c, 1.0, 1.0, 1.0, 1.0};
				triangleVertices[firstVertexIndex + 5] = {-c, bottom, -s, -s, 0.0, c, 0.0, 1.0, 0.0, 1.0};
			}
		}
	}

	size_t const VERTEX_BUFFER_SIZE = sizeof(triangleVertices);
	size_t const NUM_VERTICES = VERTEX_BUFFER_SIZE / VERTEX_SIZE;

	size_t const TREE_INSTANCES = 3;
	size_t const FLAKE_INSTANCES = 10'000;

	XMFLOAT4X4 instances[TREE_INSTANCES + FLAKE_INSTANCES];

	size_t const INSTANCE_BUFFER_SIZE = sizeof(instances);

	struct vs_const_buffer_t {
		XMFLOAT4X4 matWorldViewProj;
		XMFLOAT4X4 matWorldView;
		XMFLOAT4X4 matView;
		XMFLOAT4 colMaterial;
		XMFLOAT4 colLight;
		XMFLOAT4 dirLight;
		XMFLOAT4 padding;
	};

	vs_const_buffer_t vsConstBuffer;

	size_t const CONSTANT_BUFFER_SIZE = sizeof(vsConstBuffer);
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

	setTriangleVertices();
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
	commandList->SetGraphicsRootDescriptorTable(0, cbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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
	float clearColor[] = {0.7f, 0.6f, 1.0f, 1.0f};
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetVertexBuffers(1, 1, &instanceBufferView);
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

void DirectX3DHelper::setWVPMatrix() {
	static float angle = .0f;
	angle += 1.f / 64;

	XMMATRIX wMatrix = XMMatrixRotationY(angle);
	XMMATRIX vMatrix = XMMatrixTranslation(0.0f, 0.0f, 4.0f);
	XMMATRIX pMatrix = XMMatrixPerspectiveFovLH(
      45.0f, viewport.Width / viewport.Height, 1.0f, 100.0f
  );
	XMMATRIX wvMatrix = XMMatrixMultiply(wMatrix, vMatrix);
	XMMATRIX wvpMatrix = XMMatrixMultiply(wvMatrix, pMatrix);
	XMStoreFloat4x4(
	    &vsConstBuffer.matWorldViewProj,
	    XMMatrixTranspose(wvpMatrix)
	);
	XMStoreFloat4x4(
	    &vsConstBuffer.matWorldView,
	    XMMatrixTranspose(wvMatrix)
	);
	XMStoreFloat4x4(
	    &vsConstBuffer.matView,
	    XMMatrixTranspose(vMatrix)
	);
	vsConstBuffer.colMaterial = {1.0, 0.4, 1.0, 1.0}; // pink leaves
	vsConstBuffer.colLight = {1.0, 1.0, 1.0, 1.0}; // white light
	vsConstBuffer.dirLight = {0.0, 0.0, 1.0, 0.0};
	memcpy(pcBufferDataBegin,
	    &vsConstBuffer,
	    CONSTANT_BUFFER_SIZE
	);
}

void DirectX3DHelper::draw() {
	populateCommandList();
	setWVPMatrix();

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
	    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	    .NumDescriptors = frameCount,
	    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE};
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap)));
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Constant buffer descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC cbvDescriptorHeapDesc{
	    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	    .NumDescriptors = 1,
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
	// Root signature with constant buffer
	D3D12_DESCRIPTOR_RANGE descriptorRange{
	    .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
	    .NumDescriptors = 1,
	    .BaseShaderRegister = 0,
	    .RegisterSpace = 0,
	    .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};

	D3D12_ROOT_PARAMETER rootParameters[] = {D3D12_ROOT_PARAMETER{
	    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
	    .DescriptorTable = {1, &descriptorRange},
	    .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX}};

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{
	    .NumParameters = _countof(rootParameters),
	    .pParameters = rootParameters,
	    .NumStaticSamplers = 0,
	    .pStaticSamplers = nullptr,
	    .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS};

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
	    .DepthStencilState = {
	        .DepthEnable = TRUE,
	        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
	        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
	        .StencilEnable = FALSE,
	        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
	        .StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK,
	        .FrontFace = {
	            .StencilFailOp = D3D12_STENCIL_OP_KEEP,
	            .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
	            .StencilPassOp = D3D12_STENCIL_OP_KEEP,
	            .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS},
	        .BackFace = {
	            .StencilFailOp = D3D12_STENCIL_OP_KEEP,
	            .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
	            .StencilPassOp = D3D12_STENCIL_OP_KEEP,
	            .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
	        }},
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
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	vertexBuffer->Unmap(0, nullptr);

	vertexBufferView = {
	    .BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
	    .SizeInBytes = VERTEX_BUFFER_SIZE,
	    .StrideInBytes = VERTEX_SIZE,
	};

	// Instance buffer
	D3D12_HEAP_PROPERTIES instanceHeapProperties = {
	    .Type = D3D12_HEAP_TYPE_UPLOAD,
	    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	    .CreationNodeMask = 1,
	    .VisibleNodeMask = 1
	};

	D3D12_RESOURCE_DESC resource_desc = {
	    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	    .Alignment = 0,
	    .Width = INSTANCE_BUFFER_SIZE,
	    .Height = 1,
	    .DepthOrArraySize = 1,
	    .MipLevels = 1,
	    .Format = DXGI_FORMAT_UNKNOWN,
	    .SampleDesc = {.Count = 1, .Quality = 0 },
	    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	    .Flags = D3D12_RESOURCE_FLAG_NONE
	};

	device->CreateCommittedResource(
	    &instanceHeapProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resource_desc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&instanceBuffer)
	);

	void * pInstanceDataBegin;
	D3D12_RANGE readRange = { 0, 0 };
	instanceBuffer->Map(
	    0, &readRange, (void **)(&pInstanceDataBegin)
	);
	memcpy(pInstanceDataBegin, instances, INSTANCE_BUFFER_SIZE);
	instanceBuffer->Unmap(0, nullptr);

	instanceBufferView = {
		.BufferLocation = instanceBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = INSTANCE_BUFFER_SIZE,
		.StrideInBytes = sizeof(XMFLOAT4X4)
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
	    .SampleDesc = {.Count = 1, .Quality = 0 },
	    .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
	    .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};

	D3D12_CLEAR_VALUE dsvClearValue{
	    .Format = DXGI_FORMAT_D32_FLOAT,
	    .DepthStencil = { .Depth = 1.0f, .Stencil = 0 }};

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

DirectX3DHelper::~DirectX3DHelper() {
	waitForPreviousFrame();
	CloseHandle(fenceEvent);
}
