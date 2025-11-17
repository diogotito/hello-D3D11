#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>

void DebugLastError() {
	DWORD errorCode = GetLastError();
	LPVOID lpMsgBuf = nullptr;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorCode, 0,
		(LPWSTR)&lpMsgBuf, 0,
		nullptr);
	OutputDebugStringW((LPCWSTR)lpMsgBuf);
	OutputDebugStringW(L"\n----------------\n");
	LocalFree(lpMsgBuf);
	DebugBreak();
}

LRESULT CALLBACK WinProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case 0x51: // Q key
				{
					DestroyWindow(hWin);
				} break;
			}
		} break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		} break;
	}
	return DefWindowProcW(hWin, uMsg, wParam, lParam);
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /* hPrevInstance */,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
) {

#pragma region Window creation
	OutputDebugStringW(L"\n================\nHello, debugger!\n================\n");

	const wchar_t* wndclassname = L"HelloWindow";
	WNDCLASSW wndclass = { 0 };
	wndclass.lpfnWndProc = WinProc;
	wndclass.hInstance = hInstance;
	wndclass.lpszClassName = wndclassname;
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH) /*(HBRUSH)(COLOR_WINDOW + 1)*/;
	wndclass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

	if ( !RegisterClassW(&wndclass) )
	{
		OutputDebugStringW(L"  !! RegisterClassExW failed\n  :: ");
		DebugLastError();
		return 1;
	}

	HWND hWnd = CreateWindowExW(
		WS_EX_APPWINDOW,
		wndclassname,
		L"My first window",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 300,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	if ( !hWnd )
	{
		OutputDebugStringW(L"  !! CreateWindowExW failed:\n  :: ");
		DebugLastError();
		return 2;
	}
	
	OutputDebugStringW(L"  -- Showing window...");
	ShowWindow(hWnd, nShowCmd);
	OutputDebugStringW(L"  should be showing up now.\n");
#pragma endregion


#pragma region Direct3D 11 initialization
	OutputDebugString(L"  .. Now on to D3D stuff...\n");

	HRESULT hr;

	// Create the device, device context and swap chain

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain* swapChain;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {
		.BufferDesc = {
			.Width = 600, .Height = 300,                    // Dimensions and pixel format
			.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
		},
		.SampleDesc = { .Count = 1, .Quality = 0 },         // No anti-aliasing
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,     // The buffer in this will receive the pixels from the graphics pipeline
		.BufferCount = 2,                                   // Double buffering
		.OutputWindow = hWnd, .Windowed = true,             // Render to our window
		.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD, // or _FLIP_SEQUENTIAL
	};

	hr = D3D11CreateDeviceAndSwapChain(
		nullptr,                              // Use the first adapter enumerated by IDXGIFactory1::EnumAdapters
		D3D_DRIVER_TYPE_HARDWARE, nullptr,    // Use the GPU; So, don't specify a software rasterizer
		D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT, // I want Debug Layer
		nullptr, 0,                           // I like the default D3D_FEATURE_LEVELS (9.1 through 11.0); So, I'm not passing an array
		D3D11_SDK_VERSION,                    // We always pass D3D11_SDK_VERSION here
		// --- Outputs ---
		&swapChainDesc, &swapChain, &device, /* pFeatureLevel */ nullptr, &context
	);
	if (FAILED(hr))
	{
		OutputDebugString(L"\n !! Failed to D3D11CreateDeviceAndSwapChain()\n\n");
		return hr;
	}
	OutputDebugString(L"  -- Got swapChain, device and context!\n");

	// Get the back buffer from swap chain

	ID3D11Texture2D *backBuffer;
	ID3D11RenderTargetView *renderTargetView;

	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (FAILED(hr))
	{
		OutputDebugString(L"\n !! Failed to GetBuffer(0, ...) (the back buffer) from swapChain\n\n");
		return hr;
	}

	hr = device->CreateRenderTargetView(backBuffer, /* pDesc */ nullptr, &renderTargetView);
	if (FAILED(hr))
	{
		OutputDebugString(L"\n !! Failed to device->CreateRenderTargetView(backBuffer, ...)\n\n");
		return hr;
	}

	OutputDebugString(L"  -- Got back buffer and render target view!\n");

	// TODO Stencil buffer
	// https://learn.microsoft.com/en-us/windows/win32/direct3dgetstarted/work-with-dxgi#create-a-render-target-for-drawing

	// Viewport

	D3D11_VIEWPORT viewports[] = {
		{
			.TopLeftX = 0, .TopLeftY = 0,
			.Width = 600, .Height = 300,
			.MinDepth = 0.0f, .MaxDepth = 1.0f
		},
	};
	context->RSSetViewports(ARRAYSIZE(viewports), viewports);
#pragma endregion

#pragma region Message loop
	OutputDebugString(L"  .. On to the message loop...\n");

	MSG msg = { .message = WM_NULL };

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if(0) {
				wchar_t debugStr[256];
				wsprintf(debugStr, L"  >> Dispatching message <%4X>\n", msg.message);
				OutputDebugString(debugStr);
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			// Render!
			static const float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
			context->ClearRenderTargetView(renderTargetView, clearColor);

			swapChain->Present(1 /* sync. at least 1 vblank */, 0);
		}
	}

	OutputDebugString(L"  -- End of message loop.\n");
#pragma endregion


#pragma region Cleanup
	OutputDebugString(L"  .. Cleaning up...\n");

	backBuffer->Release();
	renderTargetView->Release();
	context->Release();
	device->Release();
	swapChain->Release();

	UnregisterClassW(wndclassname, hInstance);

	OutputDebugString(L"\n\n______The_end._______\n\n");
#pragma endregion

	return 0;
}