/* Author: Timothy Shea */

#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>

#include <windows.h>
#include <Psapi.h> // Process status API for EnumProcessModules
#include <string>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>

/*
 * ALL READS FROM THE GAME'S MEMORY ARE DONE AS A DWORD (4 BYTES)
*/

/* config file */
#define CONFIG_FILE "./GMR.cfg"

/* locations for variables in the game's memory
 *	found via CheatEngine
 *	values are in hex */

/* GAME DATA STRUCTURE
 *	Note that this does not hold the player directly, just information about the player	*/
#define STATIC_POINTER_TO_GAME_DATA_STRUCT 0x00170074	// pointer used to get to the player data
//#define OFFSET_TO_PLAYER_POINTER 0x0	// pointer to the player entity, unneeded because the player is the first thing in the game data struct
#define OFFSET_TO_BOMBS 0x40
#define OFFSET_TO_LIVES 0x54
#define OFFSET_TO_SHIELD 0x38
	/* 4 byte float representing the time remaining on the player's shield */
/* HUD. These offsets are relative to the game data struct.
 *	These coordinates represent the diamond indicator of the mouse cursor when the aim mode is set to "TARGET"
 *	Use these coordinates instead of coordinates returned by some mouse cursor read function to get a more accurate reading of where the player is looking */
#define OFFSET_TO_CURSOR_X 0x2C
#define OFFSET_TO_CURSOR_Y 0x30

/* PLAYER ENTITY
 *	use the address at POINTER_TO_PLAYER_DATA_STRUCT and add OFFSET_TO_PLAYER_POINTER and read the resulting address to access the player
 *	the player ship is of the same type as the enemies in the game */
#define OFFSET_TO_X_LOCATION 0x4
#define OFFSET_TO_Y_LOCATION 0x8
#define OFFSET_TO_DIRECTION 0xC
#define OFFSET_TO_VELOCITY 0x10
#define OFFSET_TO_ROTATION 0x14
#define OFFSET_TO_NEXT_ENEMY_DOUBLE_POINTER 0x5C

/* ENEMIES */
#define ENEMY_COUNTER_STATIC_ADDRESS 0x00240738
/* enemy type/behavior data/function addresses. Unique value to whatever distinct enemy type occupies that structure
 *	uncertain if this is data or a function
 *	can be read to determine the type of enemy */
#define PINWHEEL 0x004E1444
#define DIAMOND 0x004E1110
#define GREEN_SQ 0x004E146C
#define RED_SQ 0x004E13E4
#define SMALL_RED_SQ 0x004E1418
#define RED_CIRCLE 0x004E10E8
#define SNAKE_HEAD 0x004E13BC
#define SNAKE_TAIL_PIECE 0x004E135C
#define RED_SHIP 0x004E12EC
#define BLUE_CIRCLE 0x004e12a8 // these spawn when the red circles detonate
#define BLUE_TRIANGLE 0x004e1194

// the number of iterations to search for in the enemy loop until its time to abandon ship if all enemies have not been found
#define ENTITY_LIST_TIMEOUT 250

// game states
#define GAME_STATE_STATIC_ADDRESS 0x0023C8C4
#define GAME_STATE_MAIN_MENU 0
#define GAME_STATE_PLAYING 2
#define GAME_STATE_PAUSED 3

// error codes
#define DATA_READ_ERROR -300

// holder for information related to the game
// global so as to avoid passsing these values between here and our Python code
//	also to avoid having to get the process handle or ID every time we want to get information from the game
struct GameProcess
{
	HANDLE handle;
	DWORD processID;
	HMODULE gameBaseAddress;
} gameProcess;

// union for various data types that can be read from the game
union GameData
{
	DWORD data_dw;
	float data_f;
	uint32_t data_uint;
	bool data_bool;
};

// convert string to LPCWSTR
LPCWSTR strToLPCWSTR(const std::string& str)
{
	int len;
	int slength = str.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, buf, len);
	return buf;
}

LPCWSTR getGamePath()
{
	std::ifstream in;
	in.open(CONFIG_FILE);
	std::string path;
	getline(in, path);
	in.close();
	return strToLPCWSTR(path);
}

std::string getExeName()
{
	/* read path from config file */
	std::ifstream in;
	in.open(CONFIG_FILE);
	std::string path;
	getline(in, path);
	in.close();

	/* check if exe location is a path or just local file name */
	if (path.find_first_of('/') == std::string::npos)
		return path;

	/* parse path to return only the exe name */
	// return a substring consisting of the character after the last '/' and the remaining characters of the string
	return path.substr(path.find_last_of('/') + 1, std::string::npos);
}

bool getBaseAddress()
{
	std::string exeName = getExeName();

	// need wstring for comparisons
	std::wstring wExeName = strToLPCWSTR(exeName);

	//The number of bytes required to store all module handles in the lphModule arr
	DWORD cbNeeded;

	/* HMODULE array to hold the list of module handles recieved from EnumProcessesModules */
	HMODULE mods[1024];

	// retrieves a handle for each module in the specified process
	if (EnumProcessModules(gameProcess.handle, mods, sizeof(mods), &cbNeeded))
	{
		for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			// buffer to hold paths to modules
			TCHAR modNames[MAX_PATH];

			// retrieves the fully qualified path for the file containing the specified module
			// used to check modules against the game executable name
			if (GetModuleFileNameEx(gameProcess.handle, mods[i], modNames, sizeof(modNames) / sizeof(TCHAR)))
			{
				// using wide string to compare to our LPCWSTR exe name
				std::wstring wstrModName = modNames;

				// check if module is the game process
				if (wstrModName.find(wExeName) != std::string::npos)
				{
					// found game process
					//CloseHandle(gameProcess.handle);
					gameProcess.gameBaseAddress = mods[i];
					std::cout << "Found base address : " << std::hex << gameProcess.gameBaseAddress << std::endl;
					return true;
				}
			}
		}
	}
	// process not found or other error
	std::cout << "Could not find game process base address" << std::endl;
	return false;
}

// launches the game and fills in the handle to the process as well as the process id
bool launchGame()
{
	/* REWRITE TIME */
	/* because steam does not like me running an exe not through steam,  */
	std::cout << "Launching game ..." << std::endl;

	STARTUPINFO su_info;
	PROCESS_INFORMATION p_info;

	ZeroMemory(&su_info, sizeof(su_info));
	ZeroMemory(&p_info, sizeof(p_info));

	su_info.cb = sizeof(su_info);

	LPCWSTR exePath = getGamePath();

	if (CreateProcess(
		exePath,	// file path of the game's executable, read from the config file GMR.cfg
		NULL,			// command line arguments
		NULL,			// process handle cannot be inherited
		NULL,			// thread handle cannot be inherited
		FALSE,			// handle inheritence flag
		0,				// 0 creation flags
		NULL,			// use the caller's enviornment block
		NULL,			// use parent's starting directory
		&su_info,		// address of STARTUPINFO
		&p_info			// address of PROCESS_INFORMATION
	) == false)
	{
		std::cout << "Unable to create the game process." << std::endl;
		std::cout << "Does GMR.cfg contain the correct path to the executable?" << std::endl;
		delete[] exePath;
		return false;
	}

	delete[] exePath;				// delete the LPCWSTR we allocated space for in getGamePath()
	CloseHandle(p_info.hThread);	// close, as it it un-needed for our purposes

	gameProcess.handle = p_info.hProcess;
	gameProcess.processID = p_info.dwProcessId;

	std::cout << "Process ID : ";
	std::cout << gameProcess.processID << std::endl;

	// sleep for 2 seconds to allow the process to start before getting its address
	Sleep(2000);

	getBaseAddress();

	return true;
}

// takes the address (relative to the executable) in which to read from
// returns a DWORD (4 bytes) of data from that address in the executable's memory
// address to read is NOT relative to the game memory start address
GameData getDataFromAddress(DWORD readAddress)
{
	DWORD buffer;
	DWORD numBytesRead = 0;
	GameData data;

	if (ReadProcessMemory(
		gameProcess.handle,						// handle to the process
		(LPCVOID)readAddress,					// address to read memory from
		&buffer,								// buffer to read into
		sizeof(DWORD),							// number of bytes to read
		&numBytesRead							// holder for number of bytes that were read (NULL to ignore)
	))
	{
		data.data_dw = buffer;
	}
	else
	{
		DWORD error = GetLastError();
		std::cout << "Error reading memory.\nError Code: " << error << std::endl;
		data.data_dw = DATA_READ_ERROR;
		//system("pause");
	}
	return data;
}

/* 
	wraps getDataFromAddress for the purpose of simplifying pointer arithmetic
		takes a pointer (relative to the game's memory) and returns the address it holds */
DWORD deref(DWORD pointer)
{
	return getDataFromAddress(pointer).data_dw;
}

// writes to the given address 
bool writeDataToAddress(LPCVOID data, DWORD writeAddress, SIZE_T numBytesToWrite)
{
	SIZE_T numBytesWritten;

	if (WriteProcessMemory(
		gameProcess.handle,						// handle to the process
		(LPVOID)writeAddress,					// address to read memory from
		data,									// buffer to write from
		numBytesToWrite,						// number of bytes to write
		&numBytesWritten						// holder for number of bytes that were written (NULL to ignore)
	))
	{
		if (numBytesToWrite != numBytesWritten)
			return false;	// not all bytes were written
	}
	else
	{
		DWORD error = GetLastError();
		std::cout << "Error writing memory.\nError Code: " << error << std::endl;
		//system("pause");
		return false;
	}
	return true;
}

uint32_t getGameState()
{
	return getDataFromAddress(GAME_STATE_STATIC_ADDRESS + (DWORD)gameProcess.gameBaseAddress).data_uint;
}

DWORD getPlayerBaseAddress()
{
	return deref(deref(STATIC_POINTER_TO_GAME_DATA_STRUCT + (DWORD)gameProcess.gameBaseAddress));
}

/* sets the player's lives to the given value
	used for giving lives to the player for large training sessions */
void setLives(uint32_t numLives)
{
	writeDataToAddress(&numLives, deref(STATIC_POINTER_TO_GAME_DATA_STRUCT + (DWORD)gameProcess.gameBaseAddress) + OFFSET_TO_LIVES, sizeof(float));
}

// resets the player shield granting the player invincibility until it wears off
// keep calling this function for godmode
void resetPlayerShield()
{
	/* 4 byte value to set 8 byte shield variable to
	 *	 seems arbitrary but this value was obtained by looking at the initial value of the shield in the game's memory */
	float val = 3.33f;
	writeDataToAddress(&val, deref(STATIC_POINTER_TO_GAME_DATA_STRUCT + (DWORD)gameProcess.gameBaseAddress) + OFFSET_TO_SHIELD, sizeof(float));
}

// returns the X coordinate of an entity from the game's runtime memory. This includes the player
float getEntityX(DWORD baseAddress)
{
	// use the retrieved address to offset to the entity's X location address and read from there
	// because it was a pointer and dynamically allocated, the address will not be relative to the game process
	return getDataFromAddress(baseAddress + OFFSET_TO_X_LOCATION).data_f;
}

float getEntityY(DWORD baseAddress)
{
	// same idea as getEntityX, but using the Y offset
	return getDataFromAddress(baseAddress + OFFSET_TO_Y_LOCATION).data_f;
}

boost::python::tuple getPlayerCoords()
{
	DWORD playerBaseAddress = getPlayerBaseAddress();
	float playerX = getEntityX(playerBaseAddress);
	float playerY = getEntityY(playerBaseAddress);
	return boost::python::make_tuple(playerX, playerY);
}

uint32_t getPlayerLives()
{
	return getDataFromAddress(deref(STATIC_POINTER_TO_GAME_DATA_STRUCT + (DWORD)gameProcess.gameBaseAddress) + OFFSET_TO_LIVES).data_uint;
}

uint32_t getEnemyCount()
{
		return getDataFromAddress(ENEMY_COUNTER_STATIC_ADDRESS + (DWORD)gameProcess.gameBaseAddress).data_uint;
}

float getEntityDirection(DWORD baseAddress)
{
	return getDataFromAddress(baseAddress + OFFSET_TO_DIRECTION).data_f;
}

float getEntityVelocity(DWORD baseAddress)
{
	return getDataFromAddress(baseAddress + OFFSET_TO_VELOCITY).data_f;
}

float getEntityRotation(DWORD baseAddress)
{
	return getDataFromAddress(baseAddress + OFFSET_TO_ROTATION).data_f;
}

/* returns a signed int representing the type of entity, -1 for unknown type*/
int32_t getEntityType(DWORD baseAddress)
{
	/* get the type from the enemy struct (0 offset) */
	DWORD hexType = getDataFromAddress(baseAddress).data_dw;

	/* look up entity type and return */
	switch (hexType)
	{
	case PINWHEEL:
		return 0;

	case DIAMOND:
		return 1;

	case GREEN_SQ:
		return 2;

	case RED_SQ:
		return 3;

	case SMALL_RED_SQ:
		return 4;

	case RED_CIRCLE:
		return 5;

	case SNAKE_HEAD:
		return 6;

	case SNAKE_TAIL_PIECE:
		return 7;

	case RED_SHIP:
		return 8;

	case BLUE_CIRCLE:
		return 9;

	case BLUE_TRIANGLE:
		return 10;

	default:
		std::cout << "Unknown enemy type encountered. Its behavior function address is " << std::hex << hexType << std::endl;
		return -1; // unknown type
	}
}

boost::python::tuple getEnemyData(DWORD enemyBaseAddress)
{
	int32_t type = getEntityType(enemyBaseAddress);
	float enemyX = getEntityX(enemyBaseAddress);
	float enemyY = getEntityY(enemyBaseAddress);
	float dir = getEntityDirection(enemyBaseAddress);
	float vel = getEntityVelocity(enemyBaseAddress);
	float rot = getEntityRotation(enemyBaseAddress);
	return boost::python::make_tuple(type, enemyX, enemyY, dir, vel, rot);
}

// gets the information of every enemy that is alive in the game and returns it as a list of tuple
// (x location, y location, direction, velocity, rotation, type)
boost::python::list getEnemyList()
{
	boost::python::list enemyList;

	/* the player is the head of the list that holds the enemies
	 *	at offest 0x5C there is a pointer which points to another pointer that points to the next enemy
		*	if this enemy address is nullptr (0x00000000), then end of the list
	 *	enemies are removed from the linked list when they are destroyed, and the hole is closed
	*/

	/* access head of list (the player) */
	DWORD currentBaseAddress = getPlayerBaseAddress();
	DWORD nextEntityPtr;

	for (size_t i = 0; i < ENTITY_LIST_TIMEOUT; i++)
	{
		// get next entity pointer
		nextEntityPtr = deref(currentBaseAddress + OFFSET_TO_NEXT_ENEMY_DOUBLE_POINTER);

		// dereference enemy pointer to get to the base address of the next entity and move to it
		currentBaseAddress = deref(nextEntityPtr);

		if (0x00000000 == currentBaseAddress) // if currentBaseAddress == nullptr
			return enemyList; // end of list

		//else, read entity data and add to the list
		enemyList.append(getEnemyData(currentBaseAddress));
	}

	return enemyList;
}

/* --------------------- PYTHON MODULES --------------------- */

BOOST_PYTHON_MODULE(GameMemoryReader)
{
	using namespace boost::python;
	def("launchGame", launchGame);
	def("getPlayerCoords", getPlayerCoords);
	def("getPlayerLives", getPlayerLives);
	def("getEnemyCount", getEnemyCount);
	def("getEnemyList", getEnemyList);
	def("getGameState", getGameState);
	def("resetPlayerShield", resetPlayerShield);
}