import neat
import ai_input as INPUT

class Ava(object):
    def __init__(self, genome, config):

        self.genome = genome
        self.network = neat.nn.RecurrentNetwork.create(genome, config)

        self.x = 0
        self.y = 0

        self.timeSurvived = 0
        self.lives = 3
        self.dead = False

    def decision(self, enemy, angle):
        # Setup the input layer
        # ava x,y nearest enemy x,y angle to nearest enemy
        input = (
            10000* self.x,
            10000* self.y,
            10000* enemy[0],
            10000* enemy[1],
            10000* angle,
        )

        # Get output from network
        outputs = self.network.activate(input)
        # print('THIS IS THE OUTPUT', outputs)

        # Make decision based on output
        INPUT.release_movement_keys()
        INPUT.release_aim_keys()
        if outputs[0] > 0.5:
            INPUT.moveUp()
        if outputs[1] > 0.5:
            INPUT.moveLeft()
        if outputs[2] > 0.5:
            INPUT.moveRight()
        if outputs[3] > 0.5:
            INPUT.moveDown()
        if outputs[4] > 0.5:
            INPUT.arrowUp()
        if outputs[5] > 0.5:
            INPUT.arrowLeft()
        if outputs[6] > 0.5:
            INPUT.arrowRight()
        if outputs[7] > 0.5:
            INPUT.arrowDown()

    def save_results(self, lives, score):
        self.check_pulse(lives)

        if self.dead == True:
            self.metadata = {
                'score': score,
                'genome': self.genome,
                'network': self.network,
                'timeSurvived': self.timeSurvived,
            }
            return True
        else:
            return False

    def check_pulse(self, lives):
        if lives != 3:
            self.dead = True