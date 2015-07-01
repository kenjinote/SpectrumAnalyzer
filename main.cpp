#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "fmod")

#include <windows.h>
#include <shlwapi.h>
#include <gl\gl.h>
#include <fmod.h>
#include "resource.h"

#define SIZE 4
#define WINDOW_WIDTH (640/SIZE)
#define WINDOW_HEIGHT (480/SIZE)

FSOUND_STREAM* g_mp3_stream;
HDC hDC;
HGLRC hRC;
BOOL active = TRUE;
TCHAR szClassName[] = TEXT("{286FBACC-51F8-4324-B7B4-AA2047519AEF}");

void hsv2rgb(double h, double s, double v, double*r, double*g, double*b)
{
	while (h >= 360)h -= 360;
	const int i = (int)(h / 60.0);
	const double f = (h / 60.0) - i;
	const double p1 = v * (1 - s);
	const double p2 = v * (1 - s*f);
	const double p3 = v * (1 - s*(1 - f));
	switch (i)
	{
	case 0: *r = v; *g = p3; *b = p1; break;
	case 1: *r = p2; *g = v; *b = p1; break;
	case 2: *r = p1; *g = v; *b = p3; break;
	case 3: *r = p1; *g = p2; *b = v; break;
	case 4: *r = p3; *g = p1; *b = v; break;
	case 5: *r = v; *g = p1; *b = p2; break;
	}
}

BOOL InitGL(GLvoid)
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 532, 0, 10, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	return TRUE;
}

VOID DrawGLScene(GLvoid)
{
	static float temp[512];
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	const float* spectrum = FSOUND_DSP_GetSpectrum();
	if (!spectrum)
	{
		spectrum = temp;
	}
	glPointSize(16.0f / SIZE);
	glBegin(GL_POINTS);
	double r, g, b;
	for (unsigned int i = 0; i < 512; i++)
	{
		hsv2rgb(360.0 * i / 512.0, 1.0, 0.8, &r, &g, &b);
		glColor3f((GLfloat)r, (GLfloat)g, (GLfloat)b);
		glVertex2f(10.0f + i, 0.5f + 20.0f * spectrum[i]);
	}
	glEnd();
	glFlush();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	GLuint PixelFormat;
	static int nPosition;
	switch (msg)
	{
	case WM_CREATE:
		if (!(hDC = GetDC(hWnd)))
		{
			return -1;
		}
		if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
		{
			return -1;
		}
		if (!SetPixelFormat(hDC, PixelFormat, &pfd))
		{
			return -1;
		}
		if (!(hRC = wglCreateContext(hDC)))
		{
			return -1;
		}
		if (!wglMakeCurrent(hDC, hRC))
		{
			return -1;
		}
		if (!InitGL())
		{
			return -1;
		}
		if (FSOUND_Init(32000, 32, 0) == FALSE)
		{
			return -1;
		}
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_CHAR:
		switch (wParam)
		{
		case VK_ESCAPE:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case VK_SPACE:
			if (g_mp3_stream)
			{
				if (FSOUND_IsPlaying(0))
				{
					nPosition = FSOUND_Stream_GetPosition(g_mp3_stream);
					FSOUND_Stream_Stop(g_mp3_stream);
				}
				else
				{
					FSOUND_Stream_SetPosition(g_mp3_stream, nPosition);
					FSOUND_Stream_Play(0, g_mp3_stream);
				}
			}
			break;
		}
		break;
	case WM_APP:
		{
			CHAR szFileName[MAX_PATH];
			GetWindowTextA(hWnd, szFileName, MAX_PATH);
			if (PathFileExistsA(szFileName))
			{
				if (g_mp3_stream)
				{
					FSOUND_Stream_Stop(g_mp3_stream);
					FSOUND_Stream_Close(g_mp3_stream);
					g_mp3_stream = 0;
					nPosition = 0;
				}
				g_mp3_stream = FSOUND_Stream_Open(szFileName, FSOUND_2D, 0, 0);
				if (g_mp3_stream)
				{
					FSOUND_Stream_SetMode(g_mp3_stream, FSOUND_LOOP_NORMAL);
					FSOUND_Stream_Play(0, g_mp3_stream);
					FSOUND_DSP_SetActive(FSOUND_DSP_GetFFTUnit(), TRUE);
				}
			}
		}
		break;
	case WM_DROPFILES:
		{
			const HDROP hDrop = (HDROP)wParam;
			TCHAR szFileName[MAX_PATH];
			const UINT nFiles = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
			if (nFiles == 1)
			{
				DragQueryFile(hDrop, 0, szFileName, sizeof(szFileName));
				if ( PathMatchSpec(szFileName, TEXT("*.mp3")) ||
					 PathMatchSpec(szFileName, TEXT("*.wav")) )
				{
					SetWindowText(hWnd, szFileName);
					PostMessage(hWnd, WM_APP, 0, 0);
				}
			}
			DragFinish(hDrop);
		}
		break;
	case WM_NCHITTEST:
		wParam = DefWindowProc(hWnd, msg, wParam, lParam);
		if (wParam == HTCLIENT)
			return HTCAPTION;
		else
			return wParam;
	case WM_ACTIVATE:
		active = !HIWORD(wParam);
		break;
	case WM_DESTROY:
		FSOUND_Stream_Stop(g_mp3_stream);
		FSOUND_Stream_Close(g_mp3_stream);
		FSOUND_Close();
		if (hRC)
		{
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(hRC);
		}
		if (hDC)
		{
			ReleaseDC(hWnd, hDC);
		}
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	const HWND hWindow = FindWindow(szClassName, 0);
	if (hWindow)
	{
		SetForegroundWindow(hWindow);
		int n;
		LPTSTR* argv = CommandLineToArgvW(GetCommandLine(), &n);
		if (n == 2)
		{
			SetWindowText(hWindow, argv[1]);
			SendMessage(hWindow, WM_APP, 0, 0);
		}
		LocalFree(argv);
		return 0;
	}
	MSG msg;
	WNDCLASS wndclass = { 0, WndProc, 0, 0, hInstance, LoadIcon(hInstance, (LPCTSTR)IDI_ICON1), LoadCursor(0, IDC_ARROW), 0, 0, szClassName };
	RegisterClass(&wndclass);
	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&rect, WS_VISIBLE | WS_POPUP, FALSE);
	HWND hWnd = CreateWindowEx(WS_EX_TOPMOST, szClassName, 0, WS_VISIBLE | WS_POPUP, CW_USEDEFAULT, 0, rect.right - rect.left, rect.bottom - rect.top, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	BOOL done = FALSE;
	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				done = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (active)
		{
			DrawGLScene();
			SwapBuffers(hDC);
		}
	}
	return msg.wParam;
}
