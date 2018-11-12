import sys, enum
import neat
import pickle
from time import sleep
from geowars import GeoWars
import GameMemoryReader as GMR

class GameState(enum.Enum):
	MAIN_MENU = 0
	PLAYING = 2
	PAUSED = 3

def evolutionary_driver():
    # Load configuration.
    config = neat.Config(neat.DefaultGenome, neat.DefaultReproduction,
                         neat.DefaultSpeciesSet, neat.DefaultStagnation,
                         'config')

    # Create the population, which is the top-level object for a NEAT run.
    p = neat.Population(config)

    # Add a stdout reporter to show progress in the terminal.
    p.add_reporter(neat.StdOutReporter(True))

    # Run until we achive n.
    winner = p.run(eval_genomes, n=15)

    # saving winner to pkl file
    with open('winner-feedforward', 'wb') as f:
        pickle.dump(winner, f)


def eval_genomes(genomes, config):

    # Play game and get results
    genome_id, genomes = zip(*genomes)

    learner = GeoWars(genomes, config)
    print(genomes)
    learner.play()
    results = learner.results

    # Calculate fitness and top score
    top_score = 0
    for result in results:
        score = result['score']
        timeSurvived = result['timeSurvived']
        genome = result['genome']

        fitness = timeSurvived + score * 2
        genome.fitness = -1 if fitness == 0 else fitness
        if top_score < score:
            top_score = score

    #print score
    print('The top score was:', top_score)

def main():
    if (GMR.launchGame() == True):
	    print("Game successfully launched...")
    else:
	    exit()

    while GMR.getGameState() == GameState.MAIN_MENU.value:
	    sleep(0.1) # sleep(seconds)

    # game started, wait half a second to allow the game to populate its data before reading
    sleep(1)
    evolutionary_driver()

if __name__ == "__main__":
	main()
