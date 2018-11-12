import sys, enum, os
import neat
import pickle
from time import sleep
from geowars import GeoWars
import GameMemoryReader as GMR

class GameState(enum.Enum):
	MAIN_MENU = 0
	PLAYING = 2
	PAUSED = 3

def run_winner(n=1):
    # Load configuration.
    config = neat.Config(neat.DefaultGenome, neat.DefaultReproduction,
                         neat.DefaultSpeciesSet, neat.DefaultStagnation,
                         'config')

    with open('winner-feedforward', 'rb') as f:
        c = pickle.load(f)

    genomes = (c, c)

    learner = GeoWars(genomes, config)
    learner.play()

def main():
    if (GMR.launchGame() == True):
	    print("Game successfully launched...")
    else:
	    exit()

    while GMR.getGameState() == GameState.MAIN_MENU.value:
	    sleep(0.1) # sleep(seconds)

    # game started, wait half a second to allow the game to populate its data before reading
    sleep(1)
    run_winner()

if __name__ == "__main__":
	main()
