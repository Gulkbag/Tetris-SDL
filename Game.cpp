#include "Game.h"
#include "Debugger.h"
#include "Render.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//vars
static const unsigned int s_kFieldWidth = 10;
static const unsigned int s_kFieldHeight = 20;
static const unsigned int s_initialFramesPerStep = 48;
static const int s_deltaFramesPerStepPerLevel = 2;

//-----------------------------------------------------------------------------------

static const Tetromino s_tetrominos[kNumTetrominoTypes] =
{
	// I
	{
		0, 1, 1, 1, 2, 1, 3, 1,
		2, 0, 2, 1, 2, 2, 2, 3,
		0, 2, 1, 2, 2, 2, 3, 2,
		1, 0, 1, 1, 1, 2, 1, 3,
		0x00ffffff,
	},
	// J
	{
		0, 0, 0, 1, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 1, 2,
		0, 1, 1, 1, 2, 1, 2, 2,
		1, 0, 1, 1, 0, 2, 1, 2,
		0x0000ffff,
	},
	// L
	{
		2, 0, 0, 1, 1, 1, 2, 1,
		1, 0, 1, 1, 1, 2, 2, 2,
		0, 1, 1, 1, 2, 1, 0, 2,
		0, 0, 1, 0, 1, 1, 1, 2,
		0xffaa00ff,
	},
	// O
	{
		1, 0, 2, 0, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 2, 1,
		1, 0, 2, 0, 1, 1, 2, 1,
		0xffff00ff
	},
	// S
	{
		1, 0, 2, 0, 0, 1, 1, 1,
		1, 0, 1, 1, 2, 1, 2, 2,
		1, 1, 2, 1, 0, 2, 1, 2,
		0, 0, 0, 1, 1, 1, 1, 2,
		0x00ff00ff,
	},
	// T
	{
		1, 0, 0, 1, 1, 1, 2, 1,
		1, 0, 1, 1, 2, 1, 1, 2,
		0, 1, 1, 1, 2, 1, 1, 2,
		1, 0, 0, 1, 1, 1, 1, 2,
		0x9900ffff,
	},
	// Z
	{
		0, 0, 1, 0, 1, 1, 2, 1,
		2, 0, 1, 1, 2, 1, 1, 2,
		0, 1, 1, 1, 1, 2, 2, 2,
		1, 0, 0, 1, 1, 1, 0, 2,
		0xff0000ff,
	}
};

//---------------------------------------------------------------------------------------

static bool isOverLap(const TetrominoInstance& instance, const Field& field)
{
	const Tetromino& tetromino = s_tetrominos[instance.m_tetrominoType];
	const Tetromino::BlockCoords& blockCoords = tetromino.blockCoord[instance.m_rot];
	for (unsigned int i = 0; i < Tetromino::kNumBlocks; ++i)
	{
		const int x = instance.m_pos.x + blockCoords[i].x;
		const int y = instance.m_pos.y + blockCoords[i].y;

		// count going outside the field as an overlap
		if (x < 0 || x >= (int)field.width || y < 0 || y >= (int)field.height)
			return true;
		if (field.staticBlocks[x + y * field.width] != -1)
			return true;
	}

	return false;
}

static void SetBlock(Field & field, unsigned int ix, unsigned int iy, unsigned int val)
{
	HP_ASSERT(ix < field.width);
	HP_ASSERT(iy < field.height);
	field.staticBlocks[iy * field.width + ix] = val;
}

//---------------------------------------------------------------------------------------

Game::Game()
	: m_deltaTimeSeconds(0.0f)
	, m_framesUntilFall(s_initialFramesPerStep)
	, m_framesPerFallStep(s_initialFramesPerStep)
	, m_numUserDropsForTetromino(0)
	, m_numLinesCleared(0)
	, m_Level(0)
	, m_score(0)
	, m_hiScore(0)
	, m_gameState(kGameState_TitleScreen)
{
	m_field.staticBlocks = nullptr;
}

Game::~Game()
{
}

bool Game::Init()
{
	return true;
}

void Game::Shutdown()
{
	delete[] m_field.staticBlocks;
	m_field.staticBlocks = nullptr;
}

void Game::Reset()
{
}

bool Game::SpawnTetromino()
{
	m_activeTetromino.m_tetrominoType = (TetrominoType)(rand() % kNumTetrominoTypes);
	m_activeTetromino.m_rot = 0;
	m_activeTetromino.m_pos.x = (m_field.width - 4) / 2;
	m_activeTetromino.m_pos.y = 0;

	if (isOverLap(m_activeTetromino, m_field))
	{
		return false;
	}

	m_framesUntilFall = s_initialFramesPerStep;
	m_numUserDropsForTetromino = 0;
	return true;
}

void Game::Update(const GameInput & input, float deltaTimeSeconds)
{
	m_deltaTimeSeconds = deltaTimeSeconds;

	switch (m_gameState)
	{
	case kGameState_TitleScreen:
		if (input.start)
		{
			InitPlaying();
			m_gameState = kGameState_Playing;
		}
		break;
	case kGameState_Playing:
		UpdatePlaying(input);
		break;
	case kGameState_GameOver:
		if (input.start)
		{
			m_gameState = kGameState_TitleScreen;
		}
		break;

	default:
		HP_FATAL_ERROR("Unhandled case");
	}
}

void Game::InitPlaying()
{
	m_field.width = s_kFieldWidth;
	m_field.height = s_kFieldHeight;
	delete[] m_field.staticBlocks;
	m_field.staticBlocks = new int[m_field.width * m_field.height];

	for (unsigned int iy = 0; iy < m_field.height; ++iy)
	{
		for (unsigned int ix = 0; ix < m_field.width; ++ix)
		{
			m_field.staticBlocks[iy * m_field.width + ix] = -1;
		}
	}

	srand((unsigned int)time(NULL));

	SpawnTetromino();

	m_numLinesCleared = 0;
	m_Level = 0;
	m_framesPerFallStep = s_initialFramesPerStep;
	m_score = 0;
}

void Game::UpdatePlaying(const GameInput & input)
{
#ifdef _DEBUG
	if (input.bDebugChangeTetromino)
	{
		m_activeTetromino.m_tetrominoType = (TetrominoType)(((unsigned int)m_activeTetromino.m_tetrominoType + 1) % (unsigned int)kNumTetrominoTypes);
	}
	if (input.bDebugMoveLeft)
	{
		--m_activeTetromino.m_pos.x;
	}
	if (input.bDebugMoveRight)
	{
		++m_activeTetromino.m_pos.x;
	}
	if (input.bDebugMoveUp)
	{
		--m_activeTetromino.m_pos.y;
	}
	if (input.bDebugMoveDown)
	{
		++m_activeTetromino.m_pos.y;
	}
#endif

	if (input.moveLeft)
	{
		//try move
		TetrominoInstance testInstance = m_activeTetromino;
		--testInstance.m_pos.x;
		if (!isOverLap(testInstance, m_field))
			m_activeTetromino.m_pos.x = testInstance.m_pos.x;
	}

	if (input.moveRight)
	{
		//try move
		TetrominoInstance testInstance = m_activeTetromino;
		++testInstance.m_pos.x;
		if (!isOverLap(testInstance, m_field))
			m_activeTetromino.m_pos.x = testInstance.m_pos.x;
	}

	//rotate
	if (input.rotClockwise)
	{
		TetrominoInstance testInstace = m_activeTetromino;
		if (testInstace.m_rot == 0)
		{
			testInstace.m_rot = 3;
		}
		else
		{
			--testInstace.m_rot;
		}

		if (isOverLap(testInstace, m_field))
		{
			testInstace.m_pos.x = m_activeTetromino.m_pos.x - 1;
			if (!isOverLap(testInstace, m_field))
			{
				m_activeTetromino = testInstace;
			}
			else
			{
				testInstace.m_pos.x = m_activeTetromino.m_pos.x + 1;
				if (!isOverLap(testInstace, m_field))
				{
					m_activeTetromino = testInstace;
				}
			}
		}
		else
		{
			m_activeTetromino = testInstace;
		}
	}

	if (input.rotAnticlockwise)
	{
		m_activeTetromino.m_rot = (m_activeTetromino.m_rot + 1) % Tetromino::kNumRots;
	}

	m_framesUntilFall -= 1;
	if (m_framesUntilFall <= 0)
	{
		m_framesUntilFall = m_framesPerFallStep;

		TetrominoInstance testInstance = m_activeTetromino;
		testInstance.m_pos.y += 1;
		if (isOverLap(testInstance, m_field))
		{
			AddTetronimoToField(m_field, m_activeTetromino);
				if (!SpawnTetromino())
					m_gameState = kGameState_GameOver;
		}
		else
		{
			m_activeTetromino.m_pos.y = testInstance.m_pos.y;
		}
	}

	if (input.hardDrop)
	{
		TetrominoInstance testInstace = m_activeTetromino;
		while (!isOverLap(testInstace, m_field))
		{
			++testInstace.m_pos.y;
			++m_numUserDropsForTetromino;
		}
		--testInstace.m_pos.y;
		--m_numUserDropsForTetromino;
		AddTetronimoToField(m_field, testInstace);
		if (!SpawnTetromino())
			m_gameState = kGameState_GameOver;
	}
}

void Game::AddTetronimoToField(const Field & field, const TetrominoInstance & instance)
{
	const Tetromino& tetromino = s_tetrominos[instance.m_tetrominoType];
	const Tetromino::BlockCoords& blockCoords = tetromino.blockCoord[instance.m_rot];
	for (unsigned int i = 0; i < Tetromino::kNumBlocks; ++i)
	{
		const int x = instance.m_pos.x + blockCoords[i].x;
		const int y = instance.m_pos.y + blockCoords[i].y;

		HP_ASSERT((x >= 0) && (x < (int)field.width && (y >= 0) && (y < (int)field.height)))
			field.staticBlocks[x + y * field.width] = (unsigned int)instance.m_tetrominoType;
	}

	unsigned int numLinesCleared = 0;
	for (unsigned int y = 0; y < field.height; ++y)
	{
		bool RowFull = true;
		for (unsigned int x = 0; x < field.width; ++x)
		{
			if (field.staticBlocks[x + y * field.width] == -1)
			{
				RowFull = false;
				break;
			}
		}

		if (RowFull)
		{
			++numLinesCleared;

			for (unsigned int yy = y; yy > 0; --yy)
			{
				for (unsigned int x = 0; x < field.width; ++x)
				{
					field.staticBlocks[x + yy * field.width] = field.staticBlocks[x + (yy - 1) * field.width];
				}
			}
		}
	}

	unsigned int previousLevel = m_numLinesCleared / 10;
	m_numLinesCleared += numLinesCleared;
	m_Level = m_numLinesCleared / 10;

	if (m_Level != previousLevel)
	{
		m_framesPerFallStep -= s_deltaFramesPerStepPerLevel;
		if (m_framesPerFallStep < 1)
		{
			m_framesPerFallStep = 1;
		}
	}

	if (numLinesCleared > 0)
	{
		unsigned int multiplier = 0;
		switch (numLinesCleared)
		{
		case 1:
			multiplier = 40;
			break;
		case 2:
			multiplier = 100;
			break;
		case 3:
			multiplier = 300;
			break;
		case 4:
			multiplier = 1200;
		}

		unsigned int score = multiplier * (previousLevel + 1);
		score += m_numUserDropsForTetromino;
		m_score += score;
		if (m_score > m_hiScore)
			m_hiScore = m_score;
	}
}

void Game::Draw(Renderer & renderer)
{
	switch (m_gameState)
	{
	case kGameState_TitleScreen:
		renderer.DrawText("Press Space To Start", renderer.GetWidth() / 2 - 100, renderer.GetHeight() / 2);
		break;
	case kGameState_Playing:
		DrawPlaying(renderer);
		break;
	case kGameState_GameOver:
		DrawPlaying(renderer);
		renderer.DrawText("GAME OVER", renderer.GetWidth() / 2 - 100, renderer.GetHeight() / 2, 0xffffffff);
		break;
	default:
		HP_FATAL_ERROR("Unhandled Case");
	}

	float fps = 1.0f / m_deltaTimeSeconds;
	char text[128];
	snprintf(text, sizeof(text), "FPS: %.1f", fps);
	renderer.DrawText(text, 0, 0, 0x8080ffff);
}

void Game::DrawPlaying(Renderer& renderer)
{
	static unsigned int blockSizePixels = 32;

	unsigned int fieldWidthPixels = m_field.width * blockSizePixels;
	unsigned int fieldHeightPixels = m_field.height * blockSizePixels;

	unsigned int fieldOffsetPixelsX = 0;
	if (renderer.GetWidth() > fieldWidthPixels)
	{
		fieldOffsetPixelsX = (renderer.GetWidth() - fieldWidthPixels) / 2;
	}

	unsigned int fieldOffsetPixelsY = 0;
	if (renderer.GetHeight() > fieldHeightPixels)
	{
		fieldOffsetPixelsY = (renderer.GetHeight() - fieldHeightPixels) / 2;
	}

	for (unsigned int iy = 0; iy < m_field.height; ++iy)
	{
		const unsigned int y = fieldOffsetPixelsY + iy * blockSizePixels;

		for (unsigned int ix = 0; ix < m_field.width; ++ix)
		{
			const unsigned int x = fieldOffsetPixelsX + ix * blockSizePixels;

			const int blockState = m_field.staticBlocks[iy * m_field.width + ix];
			unsigned int blockRgba = 0x202020ff;
			if (blockState != -1)
			{
				HP_ASSERT(blockState < kNumTetrominoTypes);
				blockRgba = s_tetrominos[blockState].rgba;
			}

			renderer.DrawSolidRect(x, y, blockSizePixels, blockSizePixels, blockRgba);
			renderer.DrawRect(x, y, blockSizePixels, blockSizePixels, 0X404040ff);
		}
	}

	for (unsigned int i = 0; i < 4; ++i)
	{
		const Tetromino& tetromino = s_tetrominos[m_activeTetromino.m_tetrominoType];
		const Tetromino::BlockCoords& blockCoords = tetromino.blockCoord[m_activeTetromino.m_rot];
		unsigned int tetrominoRgba = tetromino.rgba;
		const unsigned int x = fieldOffsetPixelsX + (m_activeTetromino.m_pos.x + blockCoords[i].x) * blockSizePixels;
		const unsigned int y = fieldOffsetPixelsY + (m_activeTetromino.m_pos.y + blockCoords[i].y) * blockSizePixels;
		renderer.DrawSolidRect(x, y, blockSizePixels, blockSizePixels, tetrominoRgba);
	}

	char text[128];
	snprintf(text, sizeof(text), "Lines: %u", m_numLinesCleared);
	renderer.DrawText(text, 0, 100, 0xffffffff);
	snprintf(text, sizeof(text), "Level: %u", m_Level);
	renderer.DrawText(text, 0, 140, 0xffffffff);
	snprintf(text, sizeof(text), "Score: %u", m_score);
	renderer.DrawText(text, 0, 180, 0xffffffff);
	snprintf(text, sizeof(text), "High score: %u", m_hiScore);
	renderer.DrawText(text, 0, 220, 0xffffffff);

#ifdef _DEBUG
	snprintf(text, sizeof(text), "Frames per fall: %u", m_framesPerFallStep);
	renderer.DrawText(text, 0, 400, 0X404040ff);
#endif
}