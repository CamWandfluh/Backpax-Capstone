import random, sys, os, enum
import numpy as np
import GameMemoryReader as GMR
import ai_input as INPUT
from player import Ava
from enemy import Enemys
from time import sleep

class GeoWars(object):

    def __init__(self, genomes, config):

        self.score = 0
        self.results = []
        self.avas = [Ava(genome, config) for genome in genomes]
        self.enemy = Enemys()

    def play(self):

        while True:
            if self.on_loop():
                return
            else:
                sys.exit()


    #BREAK THIS STUFF OUT INTO MULTIPLE FUNCTIONS
    def on_loop(self):
        index = len(self.avas)
        for ava in self.avas:
            timeSurvived = 0
            #Training each clone until fail then itterating
            while ava.dead == False:
                #Locating nearest enemy to pass to input layer
                sleep(0.2)
                try:
                    enemyCords = self.enemy.get_nearest_enemy()
                    # print('Nearest enemy cords', enemyCords)
                except:
                    enemyCords = ()
                    print("No enemies are alive")

                try:
                    enemyAngle = self.enemy.get_angle_to_nearest_enemy((ava.x, ava.y), enemyCords)
                    # print('Angle to nearest enemy', enemyAngle)
                except:
                    enemyAngle = 0

                #Updating player cords for input layer
                playerCoords = GMR.getPlayerCoords()
                ava.x = playerCoords[0]
                ava.y = playerCoords[1]
                # print('Player X and Y', abs(ava.x), abs(ava.y))

                #If enenmy exists make a decision
                if(abs(ava.x) == 294 and abs(ava.y) == 194):
                    ava.dead = True

                if len(enemyCords) != 0:
                    ava.decision(enemyCords, enemyAngle)

                #Recording time spent alive
                timeSurvived += 1
                ava.timeSurvived = timeSurvived

                #Checking Ava for 3 lives
                buffer = GMR.getPlayerLives()
                numLives = buffer
                healthCheck = ava.save_results(numLives, self.score)
                if healthCheck:
                    self.results.append(ava.metadata)
                    # print(self.results)

                self.score = GMR.getScore() #Game score here
                self.enemy.enemyList = GMR.getEnemyList() #update enemylist
                # print('ENEMies', self.enemy.enemyList)

            INPUT.release_movement_keys()
            INPUT.release_aim_keys()
            self.restart_game()
            index -= 1
            if index < 1:
                return True
            sleep(1.5)

    def restart_game(self):
        INPUT.resetGame()

    def check_corners(self, x, y):
        print(x, y)
        if(abs(x) == 294 and abs(y) == 194):
            GMR.setLives(2)


if __name__ == '__main__':
    newGame = GeoWars()
    newGame.play()