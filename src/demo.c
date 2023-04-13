/*
Copyright 2023 Shaun Nicholson - 3DOHD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// 
//	CEL Grid Rendering Demo - Good entry point for familiarizing yourself with
//	some of the 3DO graphics basics
//

*/

#include "demo.h"
#include "celutils.h"
#include "HD3DO.h" 

void initSPORT();
void initGraphics();
void initSystem();
void loadData();
void GameLoop();
void InitGame();
void SetDefaultBlockPositions();
void HandleInput(); 
void DrawGamePlayScreen();
void DisplayGameplayScreen();
void AnimateBlocks();
void ApplySelectedColorPalette();
void SwapBackgroundImage(char *file, int imgIdx);

void InitCCBFlags(CCB *cel); // Lives in HD3DO.c

/* ----- GAME VARIABLES -----*/

static ScreenContext screen;

static Item bitmapItems[SCREEN_PAGES];
static Bitmap *bitmaps[SCREEN_PAGES];

static Item VRAMIOReq;
static Item vsyncItem;
static IOInfo ioInfo;

static DebugData dData;
static int frameCount = 0;
bool ShowDebugStats = true;

static int visibleScreenPage = 0;

static int BlockImageIdx[7] =  { 0, 1, 2, 3, 4, 5, 6 }; // Can be customized

static int Palettes[5][7] =
{
	{ 1, 2, 3, 4, 5, 6, 12 }, // Default Rainbow
	{ 7, 8, 9, 10, 11, 12, 13 }, // Pinks and Purple
	{ 14, 14, 15, 15, 1, 1, 1 }, // Black red white
	{ 1, 2, 3, 4, 3, 2, 1 }, // Candy
	{ 1, 1, 14, 14, 5, 5, 5 } // Red White Blue
};

static CCB *cel_AllBlockImages[16]; // Load all potential game block data

// Joypad stuff

bool kpLeft = false;
bool kpRight = false;
bool kpUp = false;
bool kpDown = false;
bool kpLS = false;
bool kpRS = false;
bool kpA = false;
bool kpB = false;
bool kpC = false;
bool kpStart = false;
bool kpStop = false;

bool movingLeft = false;
bool movingRight = false;

int swLeft = 0;
int swRight = 0;
int swUp = 0;
int swDown = 0;
int swLS = 0;
int swRS = 0;
int swA = 0;
int swB = 0;
int swC = 0;

//

static int previousTemplate = -1;
static int selectedTemplate = 0;

static int startingIdx = 0;
static int counter = 0;

// Animation stuff

int drawDelay = 1;

static bool doAnimation = true;

AnimData animData[5] =
{
	{ true, 0, 1, 0 },
	{ true, 2, 2, 1 },
	{ true, 4, 3, 2 },
	{ true, 6, 4, 3 },
	{ true, 8, 5, 4 }
};

const int ANIM_MAX = 5;

static bool drawMode = 1; // 1 - DrawCels / 2 - DrawScreenCels

CCB *cels_GPB[26][20]; 	// Potential screen block CELs

static ubyte *backgroundBufferPtr1 = NULL; // Pointer to IMAG file passed to the SPORT buffer for the background image

int main() // Main entry point
{
	initSystem(); // Required system inits
	initGraphics(); // Required graphic inits

	InitNumberCels(6); // 3DOHD Initialize 6 sets of number CCBs for chaining

	InitNumberCel(0, 10, 180, 0, true);    // 1 - DrawCels / 2 - DrawScreenCels
	InitNumberCel(1, 10, 200, 0, true);    // Seconds elapsed since game start
	InitNumberCel(2, 10, 220, 0, true);    // Time to DrawCels in ms
	InitNumberCel(3, 237, 180, 0, false);  // Last full round trip game play and render time
	InitNumberCel(4, 237, 200, 0, false);  // Time to render 30 fields (should be around 500 or 1/2 a second)
	InitNumberCel(5, 237, 220, 0, false);  // Average ms to render - 16 is the goal usually
	
	loadData(); // Load and initialize CELs
	
	ApplySelectedColorPalette(); // Make them pretty

	initSPORT(); // Turn the background black
		
	GameLoop(); // The actual game play happens here - Nothing above gets called again
}

void InitGame()
{
	ResetCelNumbers();
	
	// SwapBackgroundImage("data/bgblack.img", 0); 	// This call will change the background image on the next VBlank. Must be 320x240 (NTSC) 3DO IMAG fiile
	
	SampleSystemTimeTV(&dData.TvInit);				// Used for timing and performance benchmarking
	SampleSystemTimeTV(&dData.TvFrames30Start);		// Used for timing and performance benchmarking
}

void GameLoop()
{	
	while (true)
	{		
		InitGame();
		
		InitEventUtility(1, 0, LC_Observer); // Turn on the joypad listener
		
		while (true) // In a real game some game over condition would break the loop back to the main start screen
		{
			HandleInput();		

			DrawGamePlayScreen();

			DisplayGameplayScreen();
		}
		
		KillEventUtility();
	}
}

void loadData()
{
	int x, y;
		
	cel_AllBlockImages[0] = LoadCel("data/block_teal.cel", MEMTYPE_CEL);
	cel_AllBlockImages[1] = LoadCel("data/block_red.cel", MEMTYPE_CEL);
	cel_AllBlockImages[2] = LoadCel("data/block_orange.cel", MEMTYPE_CEL);
	cel_AllBlockImages[3] = LoadCel("data/block_yellow.cel", MEMTYPE_CEL);
	cel_AllBlockImages[4] = LoadCel("data/block_green.cel", MEMTYPE_CEL);
	cel_AllBlockImages[5] = LoadCel("data/block_blue.cel", MEMTYPE_CEL);
	cel_AllBlockImages[6] = LoadCel("data/block_purple.cel", MEMTYPE_CEL);
	
	cel_AllBlockImages[7] = LoadCel("data/j1.cel", MEMTYPE_CEL);
	cel_AllBlockImages[8] = LoadCel("data/j2.cel", MEMTYPE_CEL);
	cel_AllBlockImages[9] = LoadCel("data/j3.cel", MEMTYPE_CEL);
	cel_AllBlockImages[10] = LoadCel("data/j4.cel", MEMTYPE_CEL);
	cel_AllBlockImages[11] = LoadCel("data/j5.cel", MEMTYPE_CEL);
	cel_AllBlockImages[12] = LoadCel("data/j6.cel", MEMTYPE_CEL);
	cel_AllBlockImages[13] = LoadCel("data/j7.cel", MEMTYPE_CEL);
	
	cel_AllBlockImages[14] = LoadCel("data/block_white.cel", MEMTYPE_CEL);
	cel_AllBlockImages[15] = LoadCel("data/block_black.cel", MEMTYPE_CEL);
	
	for (x = 0; x < 16; x++)
	{		
		InitCCBFlags(cel_AllBlockImages[x]);		
	}
	
	// Initialize the game block CCBs
	
	for (x = 0; x < 26; x++)
	{
		for (y = 0; y < 20; y++)
		{
			cels_GPB[x][y] = CopyCel(cel_AllBlockImages[0]); 
		}
	}
	
	SetDefaultBlockPositions(); // Position the Gameplay Block CCBs
	
	// Now chain them together
	for (x = 0; x < 26; x++)
	{
		for (y = 0; y < 19; y++)  
		{
			cels_GPB[x][y]->ccb_NextPtr = cels_GPB[x][y + 1]; 	
		}

		if (x < 25)
		{
			cels_GPB[x][19]->ccb_NextPtr = cels_GPB[x + 1][0];
		}
	}

	cels_GPB[25][19]->ccb_NextPtr = TrackedNumbers[0].cel_NumCels[0];
}

void initSPORTwriteValue(unsigned value)
{
	WaitVBL(vsyncItem, 1); // Prevent screen garbage presumably
	
    memset(&ioInfo,0,sizeof(ioInfo));
    ioInfo.ioi_Command = FLASHWRITE_CMD;
    ioInfo.ioi_CmdOptions = 0xffffffff;
    ioInfo.ioi_Offset = value; // background colour
    ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
    ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;	
}

void initSPORTcopyImage(ubyte *srcImage)
{	
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = SPORTCMD_COPY;
	ioInfo.ioi_Offset = 0xffffffff; // mask
	ioInfo.ioi_Send.iob_Buffer = srcImage;
	ioInfo.ioi_Send.iob_Len = SCREEN_SIZE_IN_BYTES;
	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;
	
	//WaitVBL(vsyncItem, 1); // Prevent screen garbage presumably
}

void initSPORT()
{
	VRAMIOReq = CreateVRAMIOReq(); // Obtain an IOReq for all SPORT operations
	
	SwapBackgroundImage("data/bgblack.img", -99);

	initSPORTcopyImage(backgroundBufferPtr1);
}

void SwapBackgroundImage(char *file, int imgIdx)
{	
	if (backgroundBufferPtr1 != NULL)
	{
		UnloadImage(backgroundBufferPtr1);
		backgroundBufferPtr1 = NULL;
	}

	backgroundBufferPtr1 = LoadImage(file, NULL, (VdlChunk **)NULL, &screen);
}

void initGraphics()
{
	int i;

	CreateBasicDisplay(&screen, DI_TYPE_DEFAULT, SCREEN_PAGES);

	for(i = 0; i < SCREEN_PAGES; i++)
	{
		bitmapItems[i] = screen.sc_BitmapItems[i];
		bitmaps[i] = screen.sc_Bitmaps[i];
	}

	DisableVAVG(screen.sc_Screens[0]);
	DisableHAVG(screen.sc_Screens[0]);
	DisableVAVG(screen.sc_Screens[1]);
	DisableHAVG(screen.sc_Screens[1]);

	vsyncItem = GetVBLIOReq();
}

void initSystem()
{
    OpenGraphicsFolio();
	OpenMathFolio();
	OpenAudioFolio();
}

void DisplayGameplayScreen()
{
	TimeVal tvElapsed, tvDrawCels, tvRoundTrip, tvCurrLoopElapsed;
	frameCount++;
	
	if (frameCount >= 30)
	{
		TimeVal tv30Elapsed;
		
		SampleSystemTimeTV(&dData.TvFrames30End);
		
		SubTimes(&dData.TvFrames30Start, &dData.TvFrames30End, &tv30Elapsed);	
		
		dData.Last30Time = tv30Elapsed.tv_Microseconds / 1000; // 30 frames should be around 500 I think
		
		SampleSystemTimeTV(&dData.TvFrames30Start);
		
		frameCount = 0;
	}	
	
	SampleSystemTimeTV(&dData.TvCurrLoopEnd);
	
	SubTimes(&dData.TvDrawCelsStart, &dData.TvCurrLoopEnd, &tvCurrLoopElapsed);
	
	dData.MsCount++;
	dData.TotElapsedMS += tvCurrLoopElapsed.tv_Microseconds / 1000;
	dData.AvgMS = dData.TotElapsedMS / dData.MsCount;
	
	if (dData.MsCount > 120)
	{
		dData.MsCount = dData.TotElapsedMS = 0;
	}
	
	if (ShowDebugStats)							// Some of these stats are in Microseconds so divide by 1000 to get ms. 16ms roundtrip is 60fps. 33 is 30fps
	{
		SetCelNumbers(0, drawMode);				// 1 - DrawCels / 2 - DrawScreenCels
		SetCelNumbers(1, dData.LastSeconds);	// Seconds elapsed since game start
		SetCelNumbers(2, dData.LastDrawCels);	// Time to DrawCels in ms
		SetCelNumbers(3, dData.LastRoundTrip);	// Last full round trip game play and render time
		SetCelNumbers(4, dData.Last30Time);		// Time to render 30 fields (should be around 500 or 1/2 a second)
		SetCelNumbers(5, dData.AvgMS);			// Average ms to render - 16 is the goal usually
	}
	
	SampleSystemTimeTV(&dData.TvDrawCelsStart);	
	
	if (drawMode == 1)
	{
		DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cels_GPB[0][0]); 
	}
	else
	{		
		DrawScreenCels(screen.sc_Screens[visibleScreenPage], cels_GPB[0][0]); 
	}
	
	SampleSystemTimeTV(&dData.TvDrawCelsEnd);		
	
	SubTimes(&dData.TvInit, &dData.TvDrawCelsEnd, &tvElapsed);	
	SubTimes(&dData.TvDrawCelsStart, &dData.TvDrawCelsEnd, &tvDrawCels);
	SubTimes(&dData.TvRenderEnd, &dData.TvDrawCelsEnd, &tvRoundTrip);
	
	dData.LastSeconds = tvElapsed.tv_Seconds;
	dData.LastDrawCels = tvDrawCels.tv_Microseconds;
	dData.LastRoundTrip = tvRoundTrip.tv_Microseconds;
	
    DisplayScreen(screen.sc_Screens[visibleScreenPage], 0);
	
	visibleScreenPage = (1 - visibleScreenPage);

	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	DoIO(VRAMIOReq, &ioInfo);

	SampleSystemTimeTV(&dData.TvRenderEnd);	
}

void DrawGamePlayScreen() // TODO - This can and should be easily cleaned up
{
	if (++counter > drawDelay)
	{
		if (++startingIdx > 6) startingIdx = 0;	
		
		counter = 0;
		
		if (doAnimation)
		{		
			int x;
			
			for (x = 0; x < 5; x++)
			{
				animData[x].MoveX += animData[x].DirX; // Move current speed
				
				if (animData[x].AcclX == true)
				{
					animData[x].DirX++; // Accelerate
				}
				else
				{
					animData[x].DirX--; // Decelerate
				}
				
				if (++animData[x].MoveCountX >= ANIM_MAX) // Hit the speed limit
				{
					animData[x].AcclX =! animData[x].AcclX; // Reverse direction
					animData[x].MoveCountX = (ANIM_MAX + 2) * -1; // Now move the opposite direction + starting point offset
				}
			}	

			AnimateBlocks();
		}	

		ApplySelectedColorPalette(); // 
	}
}

/*
	Handle input includes additional checks for debouncing and intenionally articifially
	introducing input delays. Otherwise button presses register far too quickly
	
	Any of the "kp" variables keep track of a button being pressed, and held
	Any of the "sw" variables keep track of the length of time in screen refreshes a button has been pressed
	
	Much of this may be overkill, but it is leftover from my Tetris app	
*/

static ControlPadEventData cped;

void HandleInput()
{
	uint32 joyBits;
	
	GetControlPad(1, 0, &cped); //  

	joyBits = cped.cped_ButtonBits;
	
	if (joyBits & ControlX) 
	{
		if (kpStop == false)
		{
			doAnimation = false;
			
			SetDefaultBlockPositions();
		}

		kpStop = true;
	}
	else
	{
		kpStop = false;
	}

	if (joyBits & ControlStart)
	{
		if (kpStart == false)
		{
			doAnimation = true;
		}

		kpStart = true;
	}
	else
	{
		kpStart = false;
	}

	if (joyBits & ControlLeftShift) 
	{
		if (kpLS == false)
		{
			drawMode = 1; // DrawCels
		}

		kpLS = true;
		swLS++;
	}
	else
	{
		swLS = 0;
		kpLS = false;
	}

	if (joyBits & ControlRightShift) 
	{
		if (kpRS == false)
		{
			drawMode = 2; // DrawScreenCels
		}

		kpRS = true;
		swRS++;

		if (swRS >= 60) swRS = 0;
	}
	else
	{
		swRS = 0;
		kpRS = false;
	}

	if (joyBits & ControlA)
	{
		if (kpA == false || ++swA >= 15)
		{
			if (--selectedTemplate < 0) selectedTemplate = 4;
		}

		kpA = true;

		if (swA >= 15) swA = 0;
	}
	else
	{
		kpA = false;
		swA = 0;
	}

	if (joyBits & ControlB)
	{
		if (kpB == false)
		{
			if (ShowDebugStats == true)
			{
				ShowDebugStats = false;
				
				HideNumberCels();
			}
			else
			{
				ShowDebugStats = true;
				
				ShowNumberCels();
			}
		}

		kpB = true;
	}
	else
	{
		kpB = false;
		swB = 0;
	}

	if (joyBits & ControlC)
	{
		if (kpC == false || ++swC >= 15)
		{
			if (++selectedTemplate > 4) selectedTemplate = 0;
		}

		kpC = true;

		if (swC >= 15) swC = 0;
	}
	else
	{
		kpC = false;
		swC = 0;
	}

	if (joyBits & ControlUp)
	{
		if (kpUp == false || ++swUp >= 15) 
		{
			// Nothing to do here
		}

		kpUp = true;

		if (swUp >= 15) swUp = 0;
	}
	else
	{
		swUp = 0;
		kpUp = false;
	}

	if (joyBits & ControlDown)
	{
		if (kpDown == false || ++swDown >= 3) 
		{
			// Nothing to do here
		}

		kpDown = true;

		if (swDown >= 3) swDown = 0;
	}
	else
	{
		swDown = 0;
		kpDown = false;
	}

	if (joyBits & ControlLeft)
	{
		if (kpLeft == false || (swLeft >= 8 && movingLeft == false) || (swLeft >= 4 && movingLeft == true)) // Slightly stagger the increase speed
		{
			drawDelay++;
		}

		kpLeft = true;
		swLeft++;

		if (movingLeft && swLeft > 4) swLeft = 0;

		if (movingLeft == false && swLeft > 8)
		{
			movingLeft = true;
		}
	}
	else
	{
		swLeft = 0;
		kpLeft = false;
		movingLeft = false;
	}

	if (joyBits & ControlRight)
	{
		if (kpRight == false || (swRight >= 8 && movingRight == false) || (swRight >= 4 && movingRight == true)) // Slightly stagger the increase speed
		{
			if (--drawDelay < 0) drawDelay = 0;
		}

		kpRight = true;
		swRight++;

		if (movingRight && swRight > 4) swRight = 0;

		if (movingRight == false && swRight > 8)
		{
			movingRight = true;
		}
	}
	else
	{
		swRight = 0;
		kpRight = false;
		movingRight = false;
	}
}

void ApplySelectedColorPalette() // Assigns the index of the CEL images we want our game board to render
{
	int x, y;
	int cbIdx = startingIdx;
	
	if (selectedTemplate != previousTemplate)
	{	
		previousTemplate = selectedTemplate; // Track for next time
	
		for (x = 0; x < 7; x++)
		{
			BlockImageIdx[x] = Palettes[selectedTemplate][x]; 
		}
	}

	for (x = 0; x < 26; x++)
	{
		for (y = 0; y < 20; y++)
		{
			cels_GPB[x][y]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[cbIdx]]->ccb_SourcePtr; // Swaps the displayed CELs with the newly selected template's CELs

			if (++cbIdx > 6) cbIdx = 0;
		}
	}
}

void SetDefaultBlockPositions() // PositionCelColumn is a helper method which moves in fixed 12px increments
{
	int x, y;
	
	for (x = 0; x < 26; x++)
	{
		for (y = 0; y < 20; y++)
		{			
			PositionCelColumn(cels_GPB[x][y], x, y, 4, 0); // CEL, X Column, Y Column, X pixel offset, Y pixel offset
		}
	}
}

void AnimateBlocks() // Move the blocks in a wavy pattern
{	
	int x, y;
	int i = 0;
	bool accel = true;
	
	for (y = 0; y < 20; y++)
	{
		for (x = 0; x < 26; x++)
		{		
			PositionCelColumn(cels_GPB[x][y], x - 1, y, animData[i].MoveX, 0);			
		}
		
		if ((accel && i >= 4) || (!accel && i <= 0)) accel = !accel; // Do we need to reverse direction?
				
		i+= accel ? 1 : -1;
	}
}


 