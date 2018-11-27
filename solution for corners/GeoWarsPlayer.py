import GameMemoryReader as GMR
import ai_input

import ctypes				# used to check for a keypress to signal program to exit

from enum import Enum
from os import system
from time import sleep

class GameState(Enum):
	MAIN_MENU = 0
	PLAYING = 2
	PAUSED = 3

# start the game
if GMR.launchGame() == True:
	print("Game launched successfully...")
else:
	exit("Unexpected error occurred while launching the game.")

# wait for game to start (identified by the game not being in the PLAYING state)
while GMR.getGameState() != GameState.PLAYING.value:
	sleep(0.1) # sleep(seconds)

# game started, wait to allow the game to initialize necessary data before reading
sleep(0.33)

# keep the terminal ready to recieve inputs
# need to get input without using input() (because input() would pause the game)
	# could use threading
should_run = True
while should_run == True:

	enemy_list = GMR.getEnemyList()				# get information about all enemies

	player_lives = GMR.getPlayerLives()			# get all relevant information about the player
	player_coords = GMR.getPlayerCoords()

	# send data to neural net



	# network makes a decision and returns a value
	# get command associated with value
	# send command to game

	# check for exit request
	if ctypes.windll.user32.GetKeyState(0x1B) > 1: 		# ESC key
		exit("Program exit via escape character.")