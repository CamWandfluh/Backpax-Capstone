import GameMemoryReader as GMR
from math import atan2, degrees, sqrt

class Enemys(object):
    def __init__(self):

        self.nearestEnemy = ()
        self.enemyList = []
        self.angleToEnemy = 0
        # self.enemyCount = GMR.getEnemyCount()

    def get_nearest_enemy(self):
        self.enemyList = GMR.getEnemyList()
        playerCoords = GMR.getPlayerCoords()
        currDistance = 1000
        for index, enemy in enumerate(self.enemyList):
            enemyCoords = (enemy[1], enemy[2])
            newDistance = self.distance(playerCoords, enemyCoords)
            if newDistance < currDistance:
                currDistance = newDistance
                buffer = self.enemyList[index]
                self.nearestEnemy = (buffer[1], buffer[2])
        #We can sort these an provide close enemy 2 etc.
        return self.nearestEnemy

    def get_angle_to_nearest_enemy(self, player, enemy):
        xDiff = enemy[0] - player[0]
        yDiff = enemy[1] - player[1]
        angle = degrees(atan2(yDiff, xDiff))
        return angle


    def update_enemy_list(self):
        self.enemyList = GMR.getEnemyList()

    def distance(self, player, enemy):
        return sqrt((player[0] - enemy[0])**2 + (player[1] - enemy[1])**2)