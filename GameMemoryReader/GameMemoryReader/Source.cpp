#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>

#include <windows.h>
#include <Psapi.h> // Process status API for EnumProcessModules
#include <string>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>

/* config file */
#define CONFIG_FILE "./GMR.cfg"

/* some starting variables for offsets in the game's memory */
/* found via CheatEngine */
/* values are in hex */
/* PLAYER */
#define POINTER_TO_PLAYER 0x00170074
#define OFFSET_TO_PLAYER_X -0x48
#define OFFSET_TO_PLAYER_Y -0x44

#define POINTER_TO_LIVES_STRUCT 0x00170074	// pointer to whatever structure holds the lives for our player
#define OFFSET_TO_LIVES 0x54

#define ENEMY_COUNTER_ADDRESS 0x00640738
#define POINTER_TO_FIRST_ENEMY 0x00170074	// enemies are stored in a list, one enemy follows directly after another
#define OFFSET_TO_FIRST_ENEMY_X 0xA8
#define OFFSET_TO_FIRST_ENEMY_Y 0xAC
#define OFFSET_TO_FIRST_ENEMY_DIRECTION 0xB0
#define OFFSET_TO_FIRST_ENEMY_VELOCITY 0xB4
#define OFFSET_TO_FIRST_ENEMY_ROTATION 0xB8
#define OFFSET_TO_FIRST_ENEMY_TYPE 0xBC
#define OFFSET_TO_FIRST_ENEMY_ALIVE_BOOLEAN 0x6C
#define ENEMY_STRUCT_SIZE 0x88				// size of an enemy, meaning offset from this amount to get the next enemy if we have one's address


// error codes
#define DATA_READ_ERROR -300;

// holder for information related to the game
// global so as to avoid passsing these values between here and our Python code
//	also to avoid having to get the process handle or ID every time we want to get information from the game
struct GameProcess
{
	HANDLE handle;
	DWORD processID;
	HMODULE gameBaseAddress;
} gameProcess;

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

// starts the game and fills in the handle to the process as well as the process id
bool startGame()
{
	/* REWRITE TIME */
	/* because steam does not like me running an exe not through steam,  */
	std::cout << "Starting game ..." << std::endl;

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

	// sleep for 1 second to allow the process to start before getting its address
	Sleep(1000);

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
		std::cout << "Error Code: " << error << std::endl;
		data.data_dw = DATA_READ_ERROR;
		system("pause");
	}
	return data;
}

// sets the player's lives to the given value
// used for giving lives to the player for large training sessions
// hard coded so as to prevent writing to other addresses in the program leading to crashes or other undefined behavior
void setPlayerLives(uint32_t numLives)
{

}

DWORD getPlayerBaseAddress()
{
	return getDataFromAddress(POINTER_TO_PLAYER + (DWORD)gameProcess.gameBaseAddress).data_dw;
}

// returns the X coordinate of the player from the game's runtime memory
float getPlayerX()
{
	GameData gameData;

	// use the retrieved address to offset to the player's X location address and read from there
	// because it was a pointer and dynamically allocated, the address will not be relative to the game process
	gameData = getDataFromAddress(getPlayerBaseAddress() + OFFSET_TO_PLAYER_X);

	// return it as a float
	return gameData.data_f;
}

float getPlayerY()
{
	// same idea as getPlayerX, but using the Y offset
	GameData gameData;
	gameData = getDataFromAddress(getPlayerBaseAddress() + OFFSET_TO_PLAYER_Y);
	return gameData.data_f;
}

DWORD getEnemyBaseAddress()
{
	return getDataFromAddress(POINTER_TO_FIRST_ENEMY + (DWORD)gameProcess.gameBaseAddress).data_dw;
}

boost::python::tuple getPlayerCoords()
{
	float playerX = getPlayerX();
	float playerY = getPlayerY();
	return boost::python::make_tuple(playerX, playerY);
}

uint32_t getPlayerLives()
{
	GameData gameData = getDataFromAddress(POINTER_TO_LIVES_STRUCT + (DWORD)gameProcess.gameBaseAddress);
	gameData = getDataFromAddress(gameData.data_dw + OFFSET_TO_LIVES);

	return gameData.data_uint;
}

uint32_t getEnemyCount()
{
	return getDataFromAddress(ENEMY_COUNTER_ADDRESS + (DWORD)gameProcess.gameBaseAddress).data_uint;
}

float getEnemyX(DWORD enemyBaseAddress, size_t enemyIndex)
{
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_X + (enemyIndex * ENEMY_STRUCT_SIZE)).data_f;
}

float getEnemyY(DWORD enemyBaseAddress, size_t enemyIndex)
{
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_Y + (enemyIndex * ENEMY_STRUCT_SIZE)).data_f;
}

float getEnemyDirection(DWORD enemyBaseAddress, size_t enemyIndex)
{
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_DIRECTION + (enemyIndex * ENEMY_STRUCT_SIZE)).data_f;
}

float getEnemyVelocity(DWORD enemyBaseAddress, size_t enemyIndex)
{
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_VELOCITY + (enemyIndex * ENEMY_STRUCT_SIZE)).data_f;
}

float getEnemyRotation(DWORD enemyBaseAddress, size_t enemyIndex)
{
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_ROTATION + (enemyIndex * ENEMY_STRUCT_SIZE)).data_f;
}

// there is a value in the game that is different depending on what type of enemy the data represents
// use this value to determine exactly what type of enemy we are looking at
// this is important information because different enemy types exhibit different behavior
uint32_t getEnemyType(DWORD enemyBaseAddress, size_t enemyIndex)
{
	// gets as a float but returns as uint to truncate the unnecessary decimal value
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_TYPE + (enemyIndex * ENEMY_STRUCT_SIZE)).data_f;
}

bool enemyIsAlive(DWORD enemyBaseAddress, size_t enemyIndex)
{
	return getDataFromAddress(enemyBaseAddress + OFFSET_TO_FIRST_ENEMY_ALIVE_BOOLEAN + (enemyIndex * ENEMY_STRUCT_SIZE)).data_bool;
}

boost::python::tuple getEnemyData(DWORD enemyBaseAddress, size_t enemyIndex)
{
	float enemyX = getEnemyX(enemyBaseAddress, enemyIndex);
	float enemyY = getEnemyY(enemyBaseAddress, enemyIndex);
	float dir = getEnemyDirection(enemyBaseAddress, enemyIndex);
	float vel = getEnemyVelocity(enemyBaseAddress, enemyIndex);
	float rot = getEnemyRotation(enemyBaseAddress, enemyIndex);
	uint32_t type = getEnemyType(enemyBaseAddress, enemyIndex);
	return boost::python::make_tuple(enemyX, enemyY, dir, vel, rot, type);
}

// gets the information of every enemy that is alive in the game and returns it as a list of tuple
// (x location, y location, direction, velocity, rotation, type)
boost::python::list getEnemyDataList()
{
	/* TODO : a solution to these problems */
		// there exists the possibility that an enemy could be killed in the time between getting the initial enemy count and finding it
		// this would cause an infinite loop, or reading out of bounds of the enemy array, as the enemy would never be found as alive and the enemy count would never reach 0 as a result

		// there also exists the possibility that an enemy could spawn after reading the initial count, leading to some enemies that the net does not immeidately see
			// this is less of a problem, as these enemies would be picked up quickly by the next call of this function


	boost::python::list enemyDataList;

	uint32_t enemyCount = getEnemyCount();
	DWORD enemyBaseAddress = getEnemyBaseAddress();

	for (size_t i = 0; enemyCount > 0; i++)
	{
		// check if enemy at index is valid
		if (true == enemyIsAlive(enemyBaseAddress, i))
		{
			// found valid enemy, add to list and decrement search count
			enemyDataList.append(getEnemyData(enemyBaseAddress, i));
			enemyCount--;
		}
	}

	return enemyDataList;
}

/* --------------------- PYTHON MODULES --------------------- */

BOOST_PYTHON_MODULE(GameMemoryReader)
{
	using namespace boost::python;
	def("startGame", startGame);
	def("getPlayerCoords", getPlayerCoords);
	def("getPlayerLives", getPlayerLives);
	def("getPlayerX", getPlayerX);
	def("getPlayerY", getPlayerY);
	def("getEnemyCount", getEnemyCount);
	def("getEnemyDataList", getEnemyDataList);
}