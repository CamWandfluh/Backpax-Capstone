import sys, enum
import pickle
import neat
import os
import NEAT.visualize
from time import sleep
from NEAT.geowars import GeoWars
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
    stats = neat.StatisticsReporter()
    p.add_reporter(stats)
    p.add_reporter(neat.Checkpointer(5))

    # Run until we achive n.
    winner = p.run(eval_genomes, n=3)
#    pe = neat.ParallelEvaluator(4, eval_genomes)
#    winner = p.run(pe.evaluate)

    # Display the winning genome.
    print('\nBest genome:\n{!s}'.format(winner))

    visualize.draw_net(config, winner, True)
    visualize.plot_stats(stats, ylog=False, view=True)
    visualize.plot_species(stats, view=True)

    # saving winner to pkl file
    with open('winner-feedforward', 'wb') as f:
        pickle.dump(winner, f)


def eval_genomes(genomes, config):

    # Play game and get results
    genome_id, genomes = zip(*genomes)
    learner = GeoWars(genomes, config)
    learner.play()
    results = learner.results

    # Calculate fitness
    for result in results:
        score = result['score']
        timeSurvived = result['timeSurvived']
        genome = result['genome']

        fitness = score + timeSurvived
        genome.fitness = -1 if fitness < 1 else fitness
        print('Genome fitness', genome.fitness)


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
