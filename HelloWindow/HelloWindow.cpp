#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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

int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd ) {
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
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

	MSG msg;
	while (GetMessageW(&msg, hWnd, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	OutputDebugStringW(L"  -- End of message loop.");

	UnregisterClassW(wndclassname, hInstance);

	OutputDebugStringW(L"\n________________\n      End.      \n");
	return 0;
}