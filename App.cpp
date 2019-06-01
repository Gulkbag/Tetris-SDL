#include "App.h"
#include "Debugger.h"
#include "Game.h"
#include "Render.h"
#include <SDL.h>
#include <SDL_ttf.h>
#ifdef __VCCOREVER__ //raspberry Pi
#include <GLES2/gl2.h>
#endif // __VCCOREVER__
#include <stdio.h>
#include <chrono>

//=====================================================================================

static void print_SDL_version(const char* preamble, const SDL_version& v)
{
	printf("%s %u.%u.%u\n", preamble, v.major, v.minor, v.patch);
}

static bool operator==(SDL_version& a, SDL_version& b)
{
	return (a.major == b.major) && (a.minor == b.minor) && (a.patch == b.patch);
}

#ifdef GL_ES_VERSION_2_0

static void SetGLAttribute(SDL_GLattr attr, int value)
{
	if (SDL_GL_SetAttribute(attr, value) != 0)
	{
		fprintf(stderr, "SDL_GL_SETatrr failed: %s\n", SDL_GetError());
		HP_FATAL_ERROR("SDL_GL_SetAttr failed");
	}
}

static void PrintGLString(GLenum name)
{
	const GLubyte* ret = glGetString(name);
	if (ret == 0)
	{
		fprintf(stderr, "Failed to get GL string: %d\n", name);
	}
	else
	{
		printf("%s\n", ret);
	}
}

#endif // GL_ES_VERSION_2_0

//=================================================================================

App::App()
	: m_Window(0)
	, m_Renderer(0)
	, m_Game(0)
{

}

bool App::Init(bool FullScreen, unsigned int Width, unsigned int Height)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "SDL failed to initalize: %s\n", SDL_GetError());
		return false;
	}

	printf("SDL initalized\n");

	SDL_version compiledVersion;
	SDL_version linkedVersion;
	SDL_VERSION(&compiledVersion);
	SDL_GetVersion(&linkedVersion);
	print_SDL_version("Compiled against SDL version", compiledVersion);
	print_SDL_version("Linking against SDL version", linkedVersion);
	SDL_assert_release((compiledVersion == linkedVersion));

	int numDisplays = SDL_GetNumVideoDisplays();
	printf("%d video displays\n", numDisplays);
	for (int i = 0; i < numDisplays; ++i)
	{
		SDL_DisplayMode displayMode;
		if (SDL_GetCurrentDisplayMode(i, &displayMode) != 0)
		{
			fprintf(stderr, "Failed to get display mode for video display %d: %s", i, SDL_GetError());
			continue;
		}

		printf("Display %d: w=%d, h=%d refresh_rate=%d\n", i, displayMode.w, displayMode.h, displayMode.refresh_rate);

	}

#ifdef GL_ES_VERSION_2_0
	SetGLAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SetGLAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SetGLAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif // GL_ES_VERSION_2_0

	const char* title = "Tetris";
	if (FullScreen)
	{
		HP_FATAL_ERROR("Just checking");
		m_Window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else
	{
		m_Window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, SDL_WINDOW_SHOWN);
	}

	if (!m_Window)
	{
		printf("Failed to create SDL window: %s\n", SDL_GetError());
		return false;
	}

	SDL_Surface* surface;

	Uint16 pixels[16 * 16] = {  // ...or with raw pixel data:
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0aab, 0x0789, 0x0bcc, 0x0eee, 0x09aa, 0x099a, 0x0ddd,
  0x0fff, 0x0eee, 0x0899, 0x0fff, 0x0fff, 0x1fff, 0x0dde, 0x0dee,
  0x0fff, 0xabbc, 0xf779, 0x8cdd, 0x3fff, 0x9bbc, 0xaaab, 0x6fff,
  0x0fff, 0x3fff, 0xbaab, 0x0fff, 0x0fff, 0x6689, 0x6fff, 0x0dee,
  0xe678, 0xf134, 0x8abb, 0xf235, 0xf678, 0xf013, 0xf568, 0xf001,
  0xd889, 0x7abc, 0xf001, 0x0fff, 0x0fff, 0x0bcc, 0x9124, 0x5fff,
  0xf124, 0xf356, 0x3eee, 0x0fff, 0x7bbc, 0xf124, 0x0789, 0x2fff,
  0xf002, 0xd789, 0xf024, 0x0fff, 0x0fff, 0x0002, 0x0134, 0xd79a,
  0x1fff, 0xf023, 0xf000, 0xf124, 0xc99a, 0xf024, 0x0567, 0x0fff,
  0xf002, 0xe678, 0xf013, 0x0fff, 0x0ddd, 0x0fff, 0x0fff, 0xb689,
  0x8abb, 0x0fff, 0x0fff, 0xf001, 0xf235, 0xf013, 0x0fff, 0xd789,
  0xf002, 0x9899, 0xf001, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0xe789,
  0xf023, 0xf000, 0xf001, 0xe456, 0x8bcc, 0xf013, 0xf002, 0xf012,
  0x1767, 0x5aaa, 0xf013, 0xf001, 0xf000, 0x0fff, 0x7fff, 0xf124,
  0x0fff, 0x089a, 0x0578, 0x0fff, 0x089a, 0x0013, 0x0245, 0x0eff,
  0x0223, 0x0dde, 0x0135, 0x0789, 0x0ddd, 0xbbbc, 0xf346, 0x0467,
  0x0fff, 0x4eee, 0x3ddd, 0x0edd, 0x0dee, 0x0fff, 0x0fff, 0x0dee,
  0x0def, 0x08ab, 0x0fff, 0x7fff, 0xfabc, 0xf356, 0x0457, 0x0467,
  0x0fff, 0x0bcd, 0x4bde, 0x9bcc, 0x8dee, 0x8eff, 0x8fff, 0x9fff,
  0xadee, 0xeccd, 0xf689, 0xc357, 0x2356, 0x0356, 0x0467, 0x0467,
  0x0fff, 0x0ccd, 0x0bdd, 0x0cdd, 0x0aaa, 0x2234, 0x4135, 0x4346,
  0x5356, 0x2246, 0x0346, 0x0356, 0x0467, 0x0356, 0x0467, 0x0467,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
  0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff
};


	surface = SDL_CreateRGBSurfaceFrom(pixels, 16, 16, 16, 16 * 2, 0x0f00, 0x0f0, 0x000f, 0xf000);

	SDL_SetWindowIcon(m_Window, surface);

	SDL_FreeSurface(surface);

#ifdef GL_ES_VERSION_2_0
	SDL_GLContext gl_context = SDL_GL_CreateContext(m_Window);
	printf("GL_VERSION: ");
	PrintGLString(GL_VERSION);
	printf("GL_RENDERER: ");
	PrintGLString(GL_RENDERER);
	printf("GL_SHADING_LANGUAGE_VERSION: ");
	PrintGLString(GL_SHADING_LANGUAGE_VERSION);
	printf("GL_EXTENSIONS: ");
	PrintGLString(GL_EXTENSIONS);
	SDL_GL_DeleteContext(gl_context);
#endif // GL_ES_VERSION_2_0

	if (TTF_Init() == -1)
	{
		fprintf(stderr, "Failed to initalize ttf: %s\n", TTF_GetError());
		return false;
	}

	printf("SDL_ttf initalized\n");

	SDL_TTF_VERSION(&compiledVersion);
	const SDL_version* pLinkedVersion = TTF_Linked_Version();
	print_SDL_version("Compiled against SDL_ttf version", compiledVersion);
	print_SDL_version("Linking against SDL_ttf version", *pLinkedVersion);

	unsigned int logicalWidth = 1280;
	unsigned int logicalHeight = 720;
	m_Renderer = new Renderer(*m_Window, logicalWidth, logicalHeight);

	m_Game = new Game();

	if (!m_Game->Init())
	{
		fprintf(stderr, "ERROR - Game failed to initialise\n");
		return false;
	}

	return true;
}

void App::ShutDown()
{
	if (m_Game)
	{
		m_Game->Shutdown();
		delete m_Game;
		m_Game = 0;
	}

	delete m_Renderer;
	m_Renderer = 0;

	TTF_Quit();

	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void App::Run()
{
	Uint32 lastTimeMs = SDL_GetTicks();
	auto lastTime = std::chrono::high_resolution_clock::now();

	bool Done = false;
	while (!Done)
	{
		GameInput input = { 0 };

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				Done = true;
			}

			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					Done = true;
				}
				else if (event.key.keysym.sym == SDLK_SPACE)
				{
					input.start = true;
				}
				else if (event.key.keysym.sym == SDLK_LEFT)
				{
					input.moveLeft = true;
				}
				else if (event.key.keysym.sym == SDLK_RIGHT)
				{
					input.moveRight = true;
				}
				else if (event.key.keysym.sym == SDLK_z)
				{
					input.rotClockwise = true;
				}
				else if (event.key.keysym.sym == SDLK_x)
				{
					input.rotAnticlockwise = true;
				}
				else if (event.key.keysym.sym == SDLK_UP)
				{
					input.hardDrop = true;
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					input.softDrop = true;
				}
				else if (event.key.keysym.sym == SDLK_p)
				{
					input.pause = true;
				}
			}
		}

		Uint32 currentTimeMs = SDL_GetTicks();
		Uint32 deltaTimeMs = currentTimeMs - lastTimeMs;
		lastTimeMs = currentTimeMs;
		HP_UNUSED(deltaTimeMs);

		auto currentTime = std::chrono::high_resolution_clock::now();
		auto deltaTime = currentTime - lastTime;
		std::chrono::microseconds deltaTimeMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(deltaTime);
		float deltaTimeSeconds = 0.000001f * (float)deltaTimeMicroSeconds.count();
		lastTime = currentTime;

		m_Game->Update(input, deltaTimeSeconds);

		m_Renderer->Clear();
		m_Game->Draw(*m_Renderer);
		m_Renderer->Present();
	}
}
