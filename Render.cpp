#include "Render.h"
#include "Debugger.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>

//Helper functions
static SDL_Color MakeSDL_Color(uint32_t rgba)
{
	SDL_Color color;
	color.r = (unsigned char)((rgba >> 24) & 0xff);
	color.g = (unsigned char)((rgba >> 16) & 0xff);
	color.b = (unsigned char)((rgba >> 8) & 0xff);
	color.a = (unsigned char)((rgba >> 0) & 0xff);
	return color;
}

static void PrintRendererInfo(SDL_RendererInfo info)
{
	printf("Renderer : %s software+%d accelerated=%d, presentvsync=%d targettexture=%d\n",
		info.name,
		(info.flags & SDL_RENDERER_SOFTWARE) != 0,
		(info.flags & SDL_RENDERER_ACCELERATED) != 0,
		(info.flags & SDL_RENDERER_PRESENTVSYNC) != 0,
		(info.flags & SDL_RENDERER_TARGETTEXTURE) != 0);
}

//================================================================================

Renderer::Renderer(SDL_Window & window, unsigned int Width, unsigned int Height)
	: m_Width(0)
	, m_Height(0)
	, m_SdlRenderer(nullptr)
	, m_Font(nullptr)
{
	int numDrivers = SDL_GetNumRenderDrivers();
	printf("%d render drivers: \n", numDrivers);
	for (int i = 0; i < numDrivers; ++i)
	{
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		printf("%d", i);
		PrintRendererInfo(info);
	}

	Uint32 rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	m_SdlRenderer = SDL_CreateRenderer(&window, -1, rendererFlags);
	if (!m_SdlRenderer)
	{
		printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
	}

	SDL_RendererInfo info;
	if (SDL_GetRendererInfo(m_SdlRenderer, &info) != 0)
	{
		printf("SDL_GetRendererInfo failed: %s\n", SDL_GetError());
	}
	printf("Created Renderer: \n");
	PrintRendererInfo(info);

	int displayWidth, displayHeight;
	SDL_GetWindowSize(&window, &displayWidth, &displayHeight);
	printf("Display size = (%d, %d)\n", displayWidth, displayHeight);
	printf("Renderer logical size = (%u, %u)\n", Width, Height);
	if (displayWidth != (int)Width || displayHeight != (int)Height)
	{
		printf("Logical size != display size (%u, %u) vs (%u, %u). Scaling will be applied\n", Width, Height, displayWidth, displayHeight);
	}
	const float displayAspect = (float)displayWidth / (float)displayHeight;
	const float logicalAspect = (float)Width / (float)Height;
	if (logicalAspect != displayAspect)
	{
		printf("Logical aspect != display aspect. Letterboxing will be applied\n");
	}

	m_Width = Width;
	m_Height = Height;
	SDL_RenderSetLogicalSize(m_SdlRenderer, Width, Height);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	int defaultFontSize = 48;
	m_Font = TTF_OpenFont("fonts/Coder's Crux.ttf", defaultFontSize);
	if (!m_Font)
	{
		printf("TTF_OpenFont failed");
	}
}

Renderer::~Renderer()
{
	TTF_CloseFont(m_Font);
	SDL_DestroyRenderer(m_SdlRenderer);
}

void Renderer::Clear()
{
	SDL_SetRenderDrawColor(m_SdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(m_SdlRenderer);
}

void Renderer::Present()
{
	SDL_RenderPresent(m_SdlRenderer);
}

void Renderer::DrawRect(int x, int y, int w, int h, uint32_t rgba)
{
	SDL_Color color = MakeSDL_Color(rgba);
	SDL_SetRenderDrawColor(m_SdlRenderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect = { x, y, w, h };
	SDL_RenderDrawRect(m_SdlRenderer, &rect);
}

void Renderer::DrawSolidRect(int x, int y, int w, int h, uint32_t rgba)
{
	SDL_Color color = MakeSDL_Color(rgba);
	SDL_SetRenderDrawColor(m_SdlRenderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect = { x, y, w, h };
	SDL_RenderFillRect(m_SdlRenderer, &rect);
}

void Renderer::DrawText(const char* text, int x, int y, uint32_t rgba)
{
	SDL_assert(text);

	SDL_Color color = MakeSDL_Color(rgba);

	SDL_Surface* surface = TTF_RenderText_Blended(m_Font, text, color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(m_SdlRenderer, surface);
	int width, height;
	SDL_QueryTexture(texture, NULL, NULL, &width, &height);
	SDL_Rect rect = { x, y, width, height };
	SDL_RenderCopy(m_SdlRenderer, texture, nullptr, &rect);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}
