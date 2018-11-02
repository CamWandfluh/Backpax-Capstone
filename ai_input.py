from time import sleep
import ctypes

# controls
#	left mouse button down: stop shooting (should it be given this option? can be useful for killing green squares)
#	left mouse button up: resume shooting
#	W: move player up
#	A: move player left
#	S: move player down
#	D: move player right
#	space: bomb

# dont allow inputs outside of the PLAYING game state
# dont allow for the AI to pause the game (ESC key)

# instead of clicking in the bounds of the window:
	# get the x and y coordinate of where the mouse cursor is in-game
		# using the target mouse control scheme there should exist a location for the HUD
			# use these coordinates as a more accurate representation of mouse position in-game
			# because the position will be relative to the game, rather than the window

			# problem: still have to move the mouse to coordinates in screen space

			# solution: 
			#	instead of using win32gui screen coordinates,
			#		write to program memory the new coordinates to shoot to
			#			possible problem of the location constantly reseting to mouse position

# useful reference for properly using ctypes for this application:
# https://stackoverflow.com/questions/11906925/python-simulate-keydown

# macro ctypes data types for readability
LONG = ctypes.c_long
DWORD = ctypes.c_ulong
ULONG_PTR = ctypes.POINTER(DWORD)
WORD = ctypes.c_ushort
UINT = ctypes.c_uint

# tagMOUSEINPUT
# https://docs.microsoft.com/en-us/windows/desktop/api/winuser/ns-winuser-tagmouseinput
class MOUSEINPUT(ctypes.Structure):
	# tuple containing the structure data
	_fields_ = [('dx', LONG), 					# x pos of the mouse
				('dy', LONG), 					# y pos of the mouse
				('mouseData', DWORD),
				('flags', DWORD),
				('time', DWORD),				
				('dwExtraInfo', ULONG_PTR)]

# tagKEYBDINPUT
# https://docs.microsoft.com/en-us/windows/desktop/api/winuser/ns-winuser-tagkeybdinput
class KEYBDINPUT(ctypes.Structure):
	_fields_ = [('wVk', WORD),
				('wScan', WORD),
				('flags', DWORD),			# flags include KEYEVENTF_KEYUP (key being released), no flags specified: the key is being pressed
				("time", DWORD),			# timestamp for the event, if 0: the system will provide its own timestamp
				("dwExtraInfo", ULONG_PTR)]

# tagHARDWAREINPUT
# https://docs.microsoft.com/en-us/windows/desktop/api/winuser/ns-winuser-taghardwareinput
	# not directly used, but needed for INPUTu
class HARDWAREINPUT(ctypes.Structure):
	_fields_ = [('uMsg', DWORD),
				('wParamL', WORD),
				('wParamH', WORD)]

# tagInput
# https://docs.microsoft.com/en-us/windows/desktop/api/winuser/ns-winuser-taginput
	# union data type for INPUT
class INPUTu(ctypes.Union):
	_fields_ = [('mi', MOUSEINPUT),
				('ki', KEYBDINPUT),
				('hi', HARDWAREINPUT)]

class INPUT(ctypes.Structure):
	_fields_ = [('type', DWORD),
				('union', INPUTu)]

# https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-sendinput
	# returns UINT representing the number of events that were successfully inserted into the KB or mouse input stream
	# argument cInputs obtained by taking the length of the inputs list
	# pInputs is the input list
	# cbSize is the size (in bytes) of the list
def SendInputs(*inputs): # * operator unpacks a list or tuple, allows sending all items in the tuple without specifying the length
	cInputs = len(inputs)
	LPINPUT = INPUT * cInputs
	pInputs = LPINPUT(*inputs) # cast as LPINPUT
	cbSize = ctypes.c_int(ctypes.sizeof(INPUT))
	return ctypes.windll.user32.SendInput(cInputs, pInputs, cbSize) # run the c function with parameters and return the result (UINT)

# hex values for keys, need to use the DirectInput scan codes here:
#	http://www.gamespp.com/directx/directInputKeyboardScanCodes.html
DIR_W = 0x11
DIR_A = 0x1E
DIR_S = 0x1F
DIR_D = 0x20
DIR_SPACE = 0x39

# defines for input type
mi = ctypes.c_ulong(0)
ki = ctypes.c_ulong(1)
# hi = ctypes.c_ulong(2) # unused

KEYEVENTF_SCANCODE = 0x0008 # use hardware scan code for the key to simulate key press, instead of virtual-key-code
# returns an INPUT constructed from a KEYBDINPUT defining an input with the given key code and that it is being pressed
def createKeyPress(keyCode):
	# create KEYBDINPUT
	key_in = INPUTu()
	key_in.ki = KEYBDINPUT(0, keyCode, KEYEVENTF_SCANCODE, 0, None) # 0 argument for virtual-key-code and timestamp, none for extra-info
	# give new KEYBDINPUT to an INPUT instance and return
	return INPUT(ki, key_in)

KEYEVENTF_KEYUP = 0x0002
# returns an INPUT constructed from a KEYBDINPUT KEYBDINPUT defining an input with the given key code and that it is being released
def createKeyRelease(keyCode):	
	# create KEYBDINPUT
	key_in = INPUTu()
	key_in.ki = KEYBDINPUT(0, keyCode, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0, None) # 0 argument for virtual-key-code and timestamp, none for extra-info
	# give new KEYBDINPUT to an INPUT instance and return
	return INPUT(ki, key_in)

# duration that a key should be held down for
key_down_dur = 0.1


#def stopShooting():


#def resumeShooting():


def moveUp():
	SendInputs(createKeyPress(DIR_W))
	sleep(key_down_dur)
	SendInputs(createKeyRelease(DIR_W))

def moveLeft():
	SendInputs(createKeyPress(DIR_A))
	sleep(key_down_dur)
	SendInputs(createKeyRelease(DIR_A))


def moveDown():
	SendInputs(createKeyPress(DIR_S))
	sleep(key_down_dur)
	SendInputs(createKeyRelease(DIR_S))


def moveRight():
	SendInputs(createKeyPress(DIR_D))
	sleep(key_down_dur)
	SendInputs(createKeyRelease(DIR_D))


#def setMousePos(PosX, PosY):


#def useBomb():

