#include <array>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <d3d11.h>
//#include <d3dcompiler.h>
//#include "ReadData.h"
#include "VertexShader.h"
#include "PixelShader.h"

using Microsoft::WRL::ComPtr;

// Constants --------------------------------------------------------------------------------------------------------------------------------------------

constexpr int WIDTH = 600;
constexpr int HEIGHT = 300;

// Helpers ----------------------------------------------------------------------------------------------------------------------------------------------

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

#define ASSERT_SUCCEEDED(hr, what) \
do { \
	if (FAILED(hr)) \
	{ \
		OutputDebugString(L"\n !! Failed to " what L"\n\n"); \
		return hr; \
	} \
} while(0)

// A helper for grabbing a memory address for structs written in-line,
// to pass "keyword args structs" to those C-compatible DirectX functions that must receive pointers.
// https://stackoverflow.com/a/47460052
// Alternatively:
//   - Abuse the C++20 std::data() overload with std::initializer_list, like so:
//     std::data({ STRUCT_TYPENAME { ... } })
//   - template<typename T> T* as_ptr(const T&& t) { return &t; }
template<typename T>
T& as_lvalue(T&& t) { return t; }
// I like trains.

// Window procedure -------------------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK WinProc(HWND hWin, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE: // Escape key
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

// Entry point ------------------------------------------------------------------------------------------------------------------------------------------

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
		CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
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
	ShowWindow(hWnd, SC_DEFAULT /*nShowCmd*/);
	OutputDebugStringW(L"  should be showing up now.\n");
#pragma endregion


#pragma region Direct3D 11 initialization
	OutputDebugString(L"  .. Now on to D3D stuff...\n");

	// Create the device, device context and swap chain -------------------------------------------------------------------------------------------------

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGISwapChain> swapChain;

	UINT device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // We need a BGRA render target (TODO why?)
#if defined(DEBUG) || defined(_DEBUG)
	device_flags |= D3D11_CREATE_DEVICE_DEBUG; // Extra debugging stuff for debug builds
#endif

	ASSERT_SUCCEEDED(
		D3D11CreateDeviceAndSwapChain(
			// --- Device config ---
			nullptr,                              // Use the first adapter enumerated by IDXGIFactory1::EnumAdapters
			D3D_DRIVER_TYPE_HARDWARE, nullptr,    // Use the GPU; So, don't specify a software rasterizer
			device_flags,
			nullptr, 0,                           // I like the default D3D_FEATURE_LEVELS (9.1 through 11.0); So, I'm not passing an array
			D3D11_SDK_VERSION,                    // We always pass D3D11_SDK_VERSION here
			// --- Swapchain desc ---
			&as_lvalue(DXGI_SWAP_CHAIN_DESC{
				.BufferDesc = {
					.Width = WIDTH, .Height = HEIGHT,                          // Dimensions and pixel format
					.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
				},
				.SampleDesc = {.Count = 1, .Quality = 0 },                     // No anti-aliasing
				.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,                // The buffer in this will receive the pixels from the graphics pipeline
				.BufferCount = 2,                                              // Double buffering
				.OutputWindow = hWnd, .Windowed = true,                        // Render to our window
				.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD, // or _FLIP_SEQUENTIAL
				}),
			// --- Outputs ---
			&swapChain,
			&device,
			nullptr,     // pFeatureLevel
			&context
		),
		L"D3D11CreateDeviceAndSwapChain(...)"
	);
	OutputDebugString(L"  -- Got swapChain, device and context!\n");

	// Get the back buffer from swap chain --------------------------------------------------------------------------------------------------------------

	ComPtr<ID3D11Texture2D> backBuffer;
	ASSERT_SUCCEEDED(
		swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)),
		L"GetBuffer(0, ...) (the back buffer) from swapChain"
	);

	ComPtr<ID3D11RenderTargetView> renderTargetView;
	ASSERT_SUCCEEDED(
		device->CreateRenderTargetView(backBuffer.Get(), /* pDesc */ nullptr, &renderTargetView),
		L"device->CreateRenderTargetView(backBuffer, ...)"
	);

	OutputDebugString(L"  -- Got back buffer and render target view!\n");

	// Stencil buffer -----------------------------------------------------------------------------------------------------------------------------------
	// https://learn.microsoft.com/en-us/windows/win32/direct3dgetstarted/work-with-dxgi#create-a-render-target-for-drawing

	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	ASSERT_SUCCEEDED(
		device->CreateTexture2D(
			&as_lvalue(CD3D11_TEXTURE2D_DESC {
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				WIDTH, HEIGHT,
				1, // One texture
				1, // One mipmap level
				D3D11_BIND_DEPTH_STENCIL
			}),
			/* pInitialData */ nullptr,
			&depthStencilBuffer
		),
		L"create depth stencil buffer"
	);

	ComPtr<ID3D11DepthStencilView> depthStencilView;
	ASSERT_SUCCEEDED(
		device->CreateDepthStencilView(
			depthStencilBuffer.Get(),
			&as_lvalue(CD3D11_DEPTH_STENCIL_VIEW_DESC { D3D11_DSV_DIMENSION_TEXTURE2D }),
			&depthStencilView
		),
		L"create depth stencil view"
	);

	// Create a simple triangle mesh --------------------------------------------------------------------------------------------------------------------

	float triangleVertices[] = {
		0.0f,  0.5f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f,
	   -0.5f, -0.5f, 0.0f, 1.0f,
	};

	ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
	ASSERT_SUCCEEDED(
		device->CreateBuffer(
			&as_lvalue(CD3D11_BUFFER_DESC      { sizeof(triangleVertices), D3D11_BIND_VERTEX_BUFFER }),
			&as_lvalue( D3D11_SUBRESOURCE_DATA { .pSysMem = triangleVertices }),
			&vertexBuffer
		),
		L"create vertex buffer"
	);

	D3D11_INPUT_ELEMENT_DESC inputLayoutElems[] = {
		{
			.SemanticName = "POSITION", .SemanticIndex = 0,
			.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = 0, // D3D11_APPEND_ALIGNED_ELEMENT for the following ones
			.InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0,
		},
	};

	ComPtr<ID3D11InputLayout> inputLayout;
	ASSERT_SUCCEEDED(
		device->CreateInputLayout(
			inputLayoutElems, std::size(inputLayoutElems),
			COMPILED_VertexShader_DATA, sizeof(COMPILED_VertexShader_DATA),
			&inputLayout
		),
		L"CreateInputLayout(...)"
	);

	// The shaders
	ComPtr<ID3D11VertexShader> vertexShader;
	ASSERT_SUCCEEDED(
		device->CreateVertexShader(COMPILED_VertexShader_DATA, sizeof(COMPILED_VertexShader_DATA), nullptr, &vertexShader),
		L"create vertex shader"
	);

	ComPtr<ID3D11PixelShader> pixelShader;
	ASSERT_SUCCEEDED(
		device->CreatePixelShader(COMPILED_PixelShader_DATA, sizeof(COMPILED_PixelShader_DATA), nullptr, &pixelShader),
		L"create pixel shader"
	);

	// Viewport

	D3D11_VIEWPORT viewports[] = {
		{
			.TopLeftX = 0, .TopLeftY = 0,
			.Width = WIDTH, .Height = HEIGHT,
			.MinDepth = 0.0f, .MaxDepth = 1.0f
		},
	};
	context->RSSetViewports(std::size(viewports), viewports);
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
			const float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
			context->ClearRenderTargetView(renderTargetView.Get(), clearColor);
			context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			ID3D11RenderTargetView *const renderTargetViews[] = { renderTargetView.Get() }; // or .GetAddressOf()
			context->OMSetRenderTargets(1, renderTargetViews, depthStencilView.Get());

			const UINT stride = sizeof(float[4]);
			const UINT offset = 0;
			context->IASetInputLayout(inputLayout.Get());
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

			context->VSSetShader(vertexShader.Get(), /* don't use any class instances */nullptr, 0);
			context->PSSetShader(pixelShader.Get(),  /* don't use any class instances */nullptr, 0);

			context->Draw(/* Three vertices */ 3, 0);
			swapChain->Present(1 /* sync. at least 1 vblank */, 0);
		}
	}

	OutputDebugString(L"  -- End of message loop.\n");
#pragma endregion


#pragma region Cleanup
	OutputDebugString(L"  .. Cleaning up...\n");

	UnregisterClassW(wndclassname, hInstance);

	OutputDebugString(L"\n\n______The_end._______\n\n");
#pragma endregion

	return 0;
}