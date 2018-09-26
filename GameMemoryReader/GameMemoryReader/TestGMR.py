import GameMemoryReader as GMR

#def getPlayerCoords():
	#playerCoords = GMR.getPlayerCoords();

#def printTuple(touple):
	#print("X: ", tuple[0],"     Y: ", tuple[1])

if (GMR.startGame() == True):
	print("Game Started!")
else:
	exit()

shouldExit = False
while shouldExit == False:
	#printTuple(GMR.getPlayerCoords())
	#print("X: ", GMR.getPlayerX)
	print("Lives: ", GMR.getPlayerLives())
	xy = GMR.getPlayerCoords()
	print("X: ", xy[0])
	print("Y: ", xy[1])
	input()

# program end
print("Press enter to exit > ")
input()