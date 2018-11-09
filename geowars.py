import random, sys, os, enum
import numpy as np
import GameMemoryReader as GMR
from player import Ava
from enemy import Enemys
from time import sleep

class GeoWars(object):

    def __init__(self, genomes, config):

        self.score = 0
        self.avaInfo = []
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

        for ava in self.avas:

            index = 0
            timeSurvived = 0
            #Training each clone until fail then itterating
            while ava.dead == False:

                #if you sleep long enough here you can make the next ava train
                #we need a way inbetween itterations to go back to main menu and start new game
                sleep(.5)

                #Locating nearest enemy to pass to input layer
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
                # print('Player X and Y', ava.x, ava.y)

                #If enenmy exists make a decision
                if len(enemyCords) != 0:
                    ava.decision(enemyCords, enemyAngle)

                #Recording time spent alive
                timeSurvived += 1
                ava.timeSurvived = timeSurvived

                #Checking Ava for 3 lives
                buffer = GMR.getPlayerLives()
                numLives = buffer
                healthCheck = ava.save_results(numLives, self.score)
                if healthCheck == True:
                    self.avaInfo.append((('network id', id(ava)), ava.metadata))
                    print(self.avaInfo)
                    del self.avas[index]
                    if len(self.avas) == 0:
                        avas.extenct = True

                self.score = GMR.getScore() #Game score here
                self.enemy.enemyList = GMR.getEnemyList() #update enemylist
                # print('ENEMies', self.enemy.enemyList)

            index += 1


if __name__ == '__main__':
    newGame = GeoWars()
    newGame.play()