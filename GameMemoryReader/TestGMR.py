import GameMemoryReader as GMR
from enum import Enum
from os import system
from time import sleep

# RESULTS FROM SCANNING FOR UNKNOWN ENEMY TYPES
# BLUE CIRCLES THAT SPAWN WHEN RED CIRCLES DETONATE : 4e12a8
# another type : 4e016a ???
# small blue triangle : 4e1194
# 4e008e ????
# 4e030e ????

class GameState(Enum):
	MAIN_MENU = 0
	PLAYING = 2
	PAUSED = 3

if (GMR.launchGame() == True):
	print("Game successfully launched...")
else:
	exit()

highestEnemyCount = 0;
enemyCount = 0;

# wait for gamestate change
while GMR.getGameState() == GameState.MAIN_MENU.value:
	sleep(0.1) # sleep(seconds)

# game started, wait half a second to allow the game to populate its data before reading
sleep(0.5)

# start reading from the game
shouldExit = False
while shouldExit == False:
	system("cls")

	# enable godmode
	#GMR.resetPlayerShield()

	gameState = GMR.getGameState();
	print("Game State: ", gameState)

	# reading entities like the player and enemies could cause read errors if they havent been created
	# they are created when the game starts and while it is running
	# be sure to only attempt to read them while the game is playing or paused
	if gameState == GameState.PLAYING.value or gameState == GameState.PAUSED.value:

		enemyCount = GMR.getEnemyCount()
		print("Enemy Count : ", enemyCount)

		if enemyCount > highestEnemyCount:
			highestEnemyCount = enemyCount

		print("Highest Enemy Count: ", highestEnemyCount)

		print("Lives: ", GMR.getPlayerLives())
		xy = GMR.getPlayerCoords()
		print("X: ", xy[0])
		print("Y: ", xy[1])

		enemyList = GMR.getEnemyList()
		if len(enemyList) > 0:
			#print("First enemy: ",enemyList[0])
			for i, enemy in enumerate(enemyList):
				print("Enemy ", i, " at: (", enemy[1], ", ", enemy[2], enemy[0],")")
				if enemy[0] == -1:
					system("pause") # encountered unknown enemy, pause the terminal to view its type

		#input()

# program end
print("Press enter to exit > ")
input()