#pragma once
#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

struct SDL_Window;

class Game;
class Renderer;

class App
{
public:
	App();
	bool Init(bool FullScreen, unsigned int Width, unsigned int Height);
	void ShutDown();
	void Run();

private:

	SDL_Window* m_Window;
	Renderer* m_Renderer;
	Game* m_Game;
};

#endif // APP_H_INCLUDED
