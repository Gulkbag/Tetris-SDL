#pragma once
#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

class Renderer;

struct uint2
{
	unsigned int x;
	unsigned int y;
};

struct int2
{
	int x;
	int y;
};

struct Field
{
	unsigned int width;
	unsigned int height;
	int* staticBlocks;
};

struct Tetromino
{
	static const unsigned int kNumBlocks = 4;
	static const unsigned int kNumRots = 4;
	typedef uint2 BlockCoords[kNumBlocks];

	BlockCoords blockCoord[kNumRots];
	unsigned int rgba;
};

enum TetrominoType
{
	kTetrominoType_I = 0,
	kTetrominoType_J,
	kTetrominoType_L,
	kTetrominoType_O,
	kTetrominoType_S,
	kTetrominoType_T,
	kTetrominoType_Z,
	kNumTetrominoTypes
};

struct TetrominoInstance
{
	TetrominoType m_tetrominoType;
	int2 m_pos;
	unsigned int m_rot;
};

struct GameInput
{
	bool start;
	bool moveLeft;
	bool moveRight;
	bool rotClockwise;
	bool rotAnticlockwise;
	bool hardDrop;
	bool softDrop;
	bool pause;


#ifdef _DEBUG
	bool bDebugChangeTetromino;
	bool bDebugMoveLeft;
	bool bDebugMoveRight;
	bool bDebugMoveUp;
	bool bDebugMoveDown;
#endif
};

//-----------------------------------------Game Class-----------------------------------

class Game
{
public:
	Game();
	~Game();

	bool Init();
	void Shutdown();
	void Reset();
	void Update(const GameInput& input, float deltaTimeSeconds);
	void Draw(Renderer& renderer);
private:
	void InitPlaying();
	void UpdatePlaying(const GameInput& input);
	void DrawPlaying(Renderer& renderer);

	bool SpawnTetromino();
	void AddTetronimoToField(const Field& field, const TetrominoInstance& instance);

	float m_deltaTimeSeconds;
	Field m_field;
	TetrominoInstance m_activeTetromino;

	int m_framesUntilFall;
	int m_framesPerFallStep;

	unsigned int m_numUserDropsForTetromino;

	//score
	unsigned int m_numLinesCleared;
	unsigned int m_Level;
	unsigned int m_score;
	unsigned int m_hiScore;

	enum GameState
	{
		kGameState_TitleScreen = 0,
		kGameState_Playing,
		kGameState_GameOver,
		kNumGameStates
	};

	GameState m_gameState;
};

#endif // GAME_H_INCLUDED
