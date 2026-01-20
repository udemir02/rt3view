#define TARGET_FPS 60.f
#define TARGET_T (1 / TARGET_FPS)

#define _UNICODE
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <gl/gl.h>

typedef struct {
	HDC           windowdc;
	LARGE_INTEGER perf_hz;

	int wnd_width;
	int wnd_height;
} win32_global;

static win32_global GLOBAL;

typedef void   (WINAPI *glgenbuffers)    (GLsizei n, GLuint *buffers);
typedef void   (WINAPI *glbindbuffer)    (GLenum target, GLuint buffer);
typedef void   (WINAPI *glbufferdata)    (GLenum target, GLsizei size, const void *data, GLenum usage);
typedef GLuint (WINAPI *glcreateshader)  (GLenum type);
typedef void   (WINAPI *glshadersource)  (GLuint shader, GLsizei count, const char *str, const GLint *len);
typedef void   (WINAPI *glcompileshader) (GLuint shader);
typedef GLuint (WINAPI *glcreateprogram) (void);
typedef void   (WINAPI *glattachshader)  (GLuint program, GLuint shader);
typedef void   (WINAPI *gllinkprogram)   (GLuint program);
typedef void   (WINAPI *gluseprogram)    (GLuint program);


#define GLEXT \
	X(glgenbuffers,    glGenBuffers)    \
	X(glbindbuffer,    glBindBuffer)    \
	X(glbufferdata,    glBufferData)    \
	X(glcreateshader,  glCreateShader)  \
	X(glshadersource,  glShaderSource)  \
	X(glcompileshader, glCompileShader) \
	X(glcreateprogram, glCreateProgram) \
	X(glattachshader,  glAttachShader)  \
	X(gllinkprogram,   glLinkProgram)   \
	X(gluseprogram,    glUseProgram)    \

#define X(type, name) static type name;
GLEXT
#undef X

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
		SwapBuffers(GLOBAL.windowdc);
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

	GLOBAL.windowdc = GetDC(window);

	PIXELFORMATDESCRIPTOR px_format = {
		.nSize        = sizeof(px_format),
		.nVersion     = 1,
		.dwFlags      = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
		.iPixelType   = PFD_TYPE_RGBA,
		.cColorBits   = 32,
		.cDepthBits   = 24,
		.cStencilBits = 8,
		.iLayerType   = PFD_MAIN_PLANE,
	};

	SetPixelFormat(GLOBAL.windowdc, ChoosePixelFormat(GLOBAL.windowdc, &px_format), &px_format);

	HGLRC openglrc = wglCreateContext(GLOBAL.windowdc);

	if (wglMakeCurrent(GLOBAL.windowdc, openglrc)) {
	}

#define X(type, name) name = (type)wglGetProcAddress(#name);
	GLEXT
#undef X

	double start_t = win32_get_time();
	UINT64 start_c = __rdtsc();

	MSG msg;
	while (1) {
		while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT)
				return (int)msg.wParam;
		}

		glViewport(0, 0, GLOBAL.wnd_width, GLOBAL.wnd_height);
		glClearColor(1.f, 0.f, 1.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(GLOBAL.windowdc);

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

