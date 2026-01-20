#define TARGET_FPS 60.f
#define TARGET_T (1 / TARGET_FPS)

#define _UNICODE
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <gl/gl.h>

#if DISABLE_CRT
int _fltused = 0;

#pragma function(memset)
void *memset(void *_Dst, int _Val, size_t _Size)
{
	char *p = (char *)_Dst;
	while (_Size--) *p++ = (char)_Val;
	return _Dst;
}

void WinMainCRTStartup(void)
{
	ExitProcess((unsigned int)WinMain(GetModuleHandle(0), 0, 0, 0));
}
#endif

typedef struct {
	HDC           device;
	LARGE_INTEGER perf_hz;

	int wnd_width;
	int wnd_height;
} win32_global;

static win32_global GLOBAL;

static double win32_get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart / (double)GLOBAL.perf_hz.QuadPart;
}

LRESULT wndproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_SIZE:
		GLOBAL.wnd_width  = LOWORD(lparam);
		GLOBAL.wnd_height = HIWORD(lparam);
		return 0;
	case WM_PAINT:
		BeginPaint(window, 0);
		SwapBuffers(GLOBAL.device);
		EndPaint(window, 0);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcA(window, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE pinst, LPSTR cmdline, int cmdshow)
{
	timeBeginPeriod(1);
	QueryPerformanceFrequency(&GLOBAL.perf_hz);

	WNDCLASSA wndclass = {
		.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc   = wndproc,
		.hCursor       = LoadCursor(0, IDC_ARROW),
		.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
		.lpszClassName = "W",
		.hInstance     = hinst,
	};

	RegisterClassA(&wndclass);

	HWND window = CreateWindowA(wndclass.lpszClassName, "RT3VIEW",
				    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    0, 0, hinst, 0);

	GLOBAL.device = GetDC(window);

	PIXELFORMATDESCRIPTOR px_format_desired = {
		.nSize = sizeof(px_format_desired),
		.nVersion = 1,
		.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
		.cColorBits = 32,
		.cAlphaBits = 8,
		.iLayerType = PFD_MAIN_PLANE,
		.iPixelType = PFD_TYPE_RGBA
	};

	int px_format_ind = ChoosePixelFormat(GLOBAL.device, &px_format_desired);
	PIXELFORMATDESCRIPTOR px_format;
	DescribePixelFormat(GLOBAL.device, px_format_ind, sizeof(px_format), &px_format);
	SetPixelFormat(GLOBAL.device, px_format_ind, &px_format);

	HGLRC opengl_rc = wglCreateContext(GLOBAL.device);
	wglMakeCurrent(GLOBAL.device, opengl_rc);

	double start_t = win32_get_time();
	UINT64 start_c = __rdtsc();

	MSG msg = {0};
	while (msg.message != WM_QUIT) {
		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		glViewport(0, 0, GLOBAL.wnd_width, GLOBAL.wnd_height);
		glClearColor(1.f, 0.f, 1.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(GLOBAL.device);

		float work_t = (float)(win32_get_time() - start_t);

		if (work_t < TARGET_T) {
			DWORD sleep_ms = (DWORD)(1000 * (TARGET_T - work_t));
			if (sleep_ms > 1)
				Sleep(sleep_ms - 1);

			while (work_t < TARGET_T) {
				work_t = (float)(win32_get_time() - start_t);
			}
		}

		UINT64 end_c = __rdtsc();
		double end_t = win32_get_time();

#if 1
		float frame_t = (float)(end_t - start_t);
	       	float frame_c = (float)(end_c - start_c);

		int mspf = (int)((1000.f * frame_t) + .5f);
		int fps  = (int)((1.f / frame_t) + .5f);
		int mcpf = (int)((frame_c / (1000.f * 1000.f)) + .5f);
		char buf[256];
		wsprintfA(buf, "%dm/s %dfps %dmc/f\n", mspf, fps, mcpf);
		OutputDebugStringA(buf);
#endif

		start_c = end_c;
		start_t = end_t;
	}

	return (int)msg.wParam;
}

