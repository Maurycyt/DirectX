#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>
#include <wincodec.h>

using Microsoft::WRL::ComPtr;

struct Position{
	float x, y, z, lon, lat;
};

inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr))
		throw std::exception(std::to_string(hr).data());
}

class DirectX3DHelper {
	HWND hwnd{};

	static const unsigned int frameCount = 2;
	unsigned int frameIndex{};
	D3D12_RECT clientRect{};

	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	ComPtr<IDXGISwapChain3> swapChain{};
	ComPtr<ID3D12Device> device{};
	ComPtr<ID3D12Resource> renderTargets[frameCount]{};
	ComPtr<ID3D12CommandAllocator> commandAllocator{};
	ComPtr<ID3D12CommandQueue> commandQueue{};
	ComPtr<ID3D12RootSignature> rootSignature{};

	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap{};
	ComPtr<ID3D12DescriptorHeap> cbvDescriptorHeap{};
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap{};
	unsigned int rtvDescriptorSize{};

	ComPtr<ID3D12PipelineState> pipelineState{};
	ComPtr<ID3D12GraphicsCommandList> commandList{};

	ComPtr<IWICImagingFactory> WICFactory{};

	ComPtr<ID3D12Resource> vertexBuffer{};
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	ComPtr<ID3D12Resource> constantBuffer{};
	void * pcBufferDataBegin{};
	ComPtr<ID3D12Resource> textureBuffer{};
	ComPtr<ID3D12Resource> textureUploadBuffer{};
	ComPtr<ID3D12Resource> depthBuffer{};

	ComPtr<ID3D12Fence> fence{};
	HANDLE fenceEvent{};
	UINT64 fenceValue{};

	bool active{};

	void loadPipeline();

	void loadAssets();

	void waitForPreviousFrame();

	void populateCommandList();

	[[nodiscard]] Position getPosition(POINT cursorPos) const;

	void setWVPMatrix(POINT cursorPos);

	void loadBitmapFromFile(PCWSTR uri, UINT &width, UINT &height, BYTE **ppBits);

	static void parseObjectFile(PCWSTR uri);
public:

	DirectX3DHelper();

	explicit DirectX3DHelper(HWND hwnd);

	~DirectX3DHelper();

	void draw(POINT cursorPos);

	// Focus management
	void activate();

	void deactivate();
};