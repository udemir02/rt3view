#define TARGET_MS (1 / 60.f)

#define _UNICODE
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <gl/gl.h>

typedef struct {
	HDC           device;
	LARGE_INTEGER perf_hz;

	int wnd_width;
	int wnd_height;
} win32_global_var;

static win32_global_var win32_global;

#include <stdio.h>
#include <stdarg.h>
static void win32_print(char *format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	OutputDebugStringA(buf);
}

static double win32_get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart / (double)win32_global.perf_hz.QuadPart;
}

LRESULT wndproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_SIZE:
		win32_global.wnd_width  = LOWORD(lparam);
		win32_global.wnd_height = HIWORD(lparam);
		return 0;
	case WM_PAINT:
		BeginPaint(window, 0);
		SwapBuffers(win32_global.device);
		EndPaint(window, 0);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcA(window, msg, wparam, lparam);
}

#if DISABLE_CRT
void WinMainCRTStartup(void)
#else
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE pinst, LPSTR cmdline, int cmdshow)
#endif
{
	timeBeginPeriod(1);
	QueryPerformanceFrequency(&win32_global.perf_hz);

	WNDCLASSA wndclass = {0};
	wndclass.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = wndproc;
	wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszClassName = "The Window";

	RegisterClassA(&wndclass);

	HWND window = CreateWindowA(wndclass.lpszClassName, "RT3VIEW",
				    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    0, 0, 0, 0);

	win32_global.device = GetDC(window);

	PIXELFORMATDESCRIPTOR px_format_desired = {
		.nSize = sizeof(px_format_desired),
		.nVersion = 1,
		.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
		.cColorBits = 32,
		.cAlphaBits = 8,
		.iLayerType = PFD_MAIN_PLANE,
		.iPixelType = PFD_TYPE_RGBA
	};

	int px_format_ind = ChoosePixelFormat(win32_global.device, &px_format_desired);
	PIXELFORMATDESCRIPTOR px_format;
	DescribePixelFormat(win32_global.device, px_format_ind, sizeof(px_format), &px_format);
	SetPixelFormat(win32_global.device, px_format_ind, &px_format);

	HGLRC opengl_rc = wglCreateContext(win32_global.device);
	int result = wglMakeCurrent(win32_global.device, opengl_rc);

	double start_t = win32_get_time();
	UINT64 start_c = __rdtsc();

	MSG msg = {0};
	while (msg.message != WM_QUIT) {
		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			DispatchMessageA(&msg);
			TranslateMessage(&msg);
		}

		glViewport(0, 0, win32_global.wnd_width, win32_global.wnd_height);
		glClearColor(1.f, 0.f, 1.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(win32_global.device);

		float work_t = (float)(win32_get_time() - start_t);

		if (work_t < TARGET_MS) {
			DWORD sleep_ms = (DWORD)(1000 * (TARGET_MS - work_t));
			if (sleep_ms > 1)
				Sleep(sleep_ms - 1);

			while (work_t < TARGET_MS) {
				work_t = (float)(win32_get_time() - start_t);
			}
		}

		UINT64 end_c = __rdtsc();
		double end_t = win32_get_time();

#if 1
		double frame_t = end_t - start_t;
		UINT64 frame_c = end_c - start_c;

		double mspf = 1000. * frame_t;
		double fps  = 1. / frame_t;
		double mcpf = (double)frame_c / (1000. * 1000.);
		win32_print("%.2lfm/s %.2lffps %.2lfmc/f\n", mspf, fps, mcpf);
#endif

		start_c = end_c;
		start_t = end_t;
	}

#if DISABLE_CRT
	ExitProcess(0);
#else
	return (int)msg.wParam;
#endif
}

