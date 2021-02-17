#pragma once

#include "MainPage.g.h"

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 modelViewProjection;
};

static const int frameCount = 3;

namespace winrt::SlipNFrag_Windows::implementation
{
	struct MainPage : MainPageT<MainPage>
    {
		std::vector<byte> vertexShaderBuffer;
		std::vector<byte> pixelShaderBuffer;
		winrt::com_ptr<IDXGIFactory4> dxgiFactory;
		winrt::com_ptr<ID3D12Device> d3dDevice;
		winrt::com_ptr<ID3D12CommandQueue> commandQueue;
		winrt::com_ptr<ID3D12DescriptorHeap> rtvHeap;
		UINT rtvDescriptorSize;
		winrt::com_ptr<ID3D12DescriptorHeap> dsvHeap;
		winrt::com_ptr<ID3D12DescriptorHeap> cbvSrvHeap;
		winrt::com_ptr<ID3D12CommandAllocator> commandAllocators[frameCount];
		winrt::com_ptr<ID3D12Fence> fence;
		UINT64 fenceValues[frameCount];
		HANDLE fenceEvent;
		winrt::com_ptr<IDXGISwapChain3> swapChain;
		UINT currentFrame;
		winrt::com_ptr<ID3D12Resource> renderTargets[frameCount];
		winrt::com_ptr<ID3D12Resource> depthStencil;
		winrt::com_ptr<ID3D12Resource> screen;
		winrt::com_ptr<ID3D12Resource> console;
		winrt::com_ptr<ID3D12Resource> palette;
		D3D12_VIEWPORT viewport;
		winrt::com_ptr<ID3D12RootSignature> rootSignature;
		winrt::com_ptr<ID3D12PipelineState> pipelineState;
		winrt::com_ptr<ID3D12GraphicsCommandList> commandList;
		winrt::com_ptr<ID3D12Resource> vertexBuffer;
		D3D12_HEAP_PROPERTIES defaultHeapProperties;
		D3D12_HEAP_PROPERTIES uploadHeapProperties;
		winrt::com_ptr<ID3D12Resource> constantBuffer;
		UINT8* mappedConstantBuffer = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_RECT scissorRect;
		ModelViewProjectionConstantBuffer constantBufferData;
		Windows::Foundation::IAsyncAction renderLoopWorker;
		std::mutex renderLoopMutex;
		bool deviceRemoved = true;
		LARGE_INTEGER previousTime = { };
		bool fileOperationRunning = false;
		bool firstResize = false;
		float newScreenWidth;
		float newScreenHeight;
		float newScreenRowbytes;
		float newConsoleWidth;
		float newConsoleHeight;
		float newConsoleRowbytes;
		com_ptr<ID3D12Resource> screenUpload;
		com_ptr<ID3D12Resource> consoleUpload;
		com_ptr<ID3D12Resource> paletteUpload;
		D3D12_RESOURCE_DESC screenTextureDesc;
		D3D12_RESOURCE_DESC consoleTextureDesc;
		D3D12_RESOURCE_DESC paletteTextureDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC screenSrvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC consoleSrvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc;
		D3D12_TEXTURE_COPY_LOCATION screenLocation;
		D3D12_TEXTURE_COPY_LOCATION screenUploadLocation;
		D3D12_TEXTURE_COPY_LOCATION consoleLocation;
		D3D12_TEXTURE_COPY_LOCATION consoleUploadLocation;
		D3D12_TEXTURE_COPY_LOCATION paletteLocation;
		D3D12_TEXTURE_COPY_LOCATION paletteUploadLocation;
		bool key_dest_was_game;
		event_token mouseMovedToken;
		winrt::weak_ref<winrt::Windows::Gaming::Input::RawGameController> joystick = nullptr;
		winrt::weak_ref<winrt::Windows::Gaming::Input::Gamepad> gamepad = nullptr;
		bool* previousJoystickButtons = nullptr;
		int previousJoystickButtonsLength = 0;
		winrt::Windows::Media::Audio::AudioGraph audioGraph = nullptr;
		winrt::Windows::Media::Audio::AudioDeviceOutputNode audioOutput = nullptr;
		winrt::Windows::Media::Audio::AudioFrameInputNode audioInput = nullptr;
		bool previousGamepadButtonsWereRead = false;
		unsigned int previousGamepadButtons;
		std::vector<std::string> joyCommands;
		std::vector<std::string> joyAxesAsButtons;
		std::vector<std::string> joyAxesAsButtonsOnRelease;
		std::vector<int> joyAxesAsKeys;
		std::vector<float> joyAxesInverted;
		bool previousJoyAxesAsButtonValuesWereRead = false;
		std::vector<float> previousJoyAxesAsButtonValues;
		std::vector<int> joyButtonsAsKeys;
		std::vector<std::string> joyButtonsAsCommands;
		bool mlook_enabled = false;

		MainPage();
		void UpdateTitleBarLayout(Windows::ApplicationModel::Core::CoreApplicationViewTitleBar const& titleBar);
		void OpenVertexShaderFile(Windows::Storage::StorageFolder const& folder);
		void ReadVertexShaderFile(Windows::Storage::StorageFile const& file, Windows::Storage::StorageFolder const& folder);
		void OpenPixelShaderFile(Windows::Storage::StorageFolder const& folder);
		void ReadPixelShaderFile(Windows::Storage::StorageFile const& file);
		void Start();
		void CreateDevice();
		void RemoveDevice();
		void swapChainPanel_SizeChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::SizeChangedEventArgs const&);
		void swapChainPanel_CompositionScaleChanged(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&, winrt::Windows::Foundation::IInspectable const&);
		void DpiChanged(Windows::Graphics::Display::DisplayInformation const&, IInspectable const&);
		void OrientationChanged(Windows::Graphics::Display::DisplayInformation const&, IInspectable const&);
		void DisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation const&, IInspectable const&);
		void RenderLoop(Windows::Foundation::IAsyncAction const&);
		bool Update();
		void Render();
		void WaitForGPU(int frame, const char* timeoutError);
		void UploadBufferToGPU(D3D12_SUBRESOURCE_DATA data, winrt::com_ptr<ID3D12Resource> const& buffer, winrt::com_ptr<ID3D12Resource> const& bufferUpload, D3D12_RESOURCE_DESC const& descriptor);
		void CleanupFrameData();
		void CreateOutputSize(Windows::Graphics::Display::DisplayInformation const& displayInformation, Windows::Foundation::Size& outputSize);
		void CreateRotation(Windows::Graphics::Display::DisplayInformation const& displayInformation, DXGI_MODE_ROTATION& rotation);
		void CreateRenderTargetSize(DXGI_MODE_ROTATION& rotation, Windows::Foundation::Size const& size, Windows::Foundation::Size& renderTargetSize);
		void CreateOrientationTransform3D(DXGI_MODE_ROTATION rotation, DirectX::XMFLOAT4X4& orientationTransform3D);
		void UpdateSwapChainTransform(DXGI_MODE_ROTATION rotation, float compositionScaleX, float compositionScaleY);
		void CreateRenderTargetViews();
		void CreateDepthStencilBuffer(UINT width, UINT height);
		void CreateScreen();
		void UpdateModelViewProjectionMatrix(DirectX::XMFLOAT4X4 const& orientationTransform3D);
		void DisplaySysError();
		void ProcessFiles();
		void SignalFileOperationError(std::string errorMessage);
		void OpenFileForRead();
		void OpenFileForWrite();
		void OpenFileForAppend();
		void ReadFile();
		void WriteToFile();
		void GetFileTime();
		void FullscreenButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		void RegisterMouseMoved();
		void UnregisterMouseMoved();
		void CreateAudioGraph();
		void CreateAudioOutput();
		void CreateAudioInput();
		void AddJoystickAxis(winrt::Windows::Foundation::Collections::IPropertySet const& values, winrt::hstring const& stickName, std::string const& axisName, std::vector<std::string>& arguments);
		void CalculateConsoleDimensions();
		void HandleKey(int key, bool pressed);
	};
}

namespace winrt::SlipNFrag_Windows::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
