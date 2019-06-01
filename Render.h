#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

#include <SDL.h>
#include <SDL_ttf.h>

class Texture;

struct SDL_Window;
struct SDL_Renderer;

class Renderer
{
public:
	Renderer(SDL_Window& window, unsigned int Width, unsigned int Height);
	~Renderer();

	void Clear();
	void Present();

	unsigned int GetWidth() const { return m_Width; }
	unsigned int GetHeight() const { return m_Height; }

	void DrawRect(int x, int y, int w, int h, uint32_t rgba = 0xfffffffff);
	void DrawSolidRect(int x, int y, int w, int h, uint32_t rgba = 0xfffffffff);
	void DrawText(const char* text, int x, int y, uint32_t rgba = 0xfffffffff);

private:
	unsigned int m_Width;
	unsigned int m_Height;

	SDL_Renderer* m_SdlRenderer;

	TTF_Font* m_Font;
};

#endif // RENDER_H_INCLUDED
