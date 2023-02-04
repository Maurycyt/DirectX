#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>
#include <wincodec.h>

using Microsoft::WRL::ComPtr;

class DirectX3DHelper {
	HWND hwnd{};

	D3D12_RECT clientRect{};

	static const unsigned int frameCount = 2;
	ComPtr<ID3D12Device> device{};
	ComPtr<IDXGIFactory7> factory{};
	ComPtr<IWICImagingFactory> WICFactory{};
	ComPtr<ID3D12CommandQueue> commandQueue{};
	ComPtr<IDXGISwapChain3> swapChain{};
	unsigned int frameIndex{};
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap{};
	unsigned int rtvDescriptorSize{};
	ComPtr<ID3D12Resource> renderTargets[frameCount]{};
	ComPtr<ID3D12CommandAllocator> commandAllocator{};
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	ComPtr<ID3D12RootSignature> rootSignature{};
	ComPtr<ID3D12PipelineState> pipelineState{};
	ComPtr<ID3D12GraphicsCommandList> commandList{};
	ComPtr<ID3D12Resource> vertexBuffer{};
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	ComPtr<ID3D12Resource> constantBuffer{};
	void * pcBufferDataBegin{};
	ComPtr<ID3D12Resource> textureBuffer{};
	ComPtr<ID3D12Resource> textureUploadBuffer{};
	ComPtr<ID3D12Resource> depthBuffer{};
	ComPtr<ID3D12DescriptorHeap> cbvDescriptorHeap{};
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap{};
	ComPtr<ID3D12Fence> fence{};
	HANDLE fenceEvent{};
	UINT64 fenceValue{};

	static inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr))
			throw std::exception(std::to_string(hr).data());
	}

	void loadPipeline();

	void loadAssets();

	void waitForPreviousFrame();

	void populateCommandList();

	void setWVPMatrix();

	void loadBitmapFromFile(PCWSTR uri, UINT &width, UINT &height, BYTE **ppBits);
public:

	DirectX3DHelper();

	explicit DirectX3DHelper(HWND hwnd);

	~DirectX3DHelper();

	void draw();
};