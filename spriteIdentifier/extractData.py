#!/usr/bin/env python3

# ShankBot project
# Mikael Hernvall 2016
# Connect sprites with Tibia objects.
# The script creates a folder for every
# object and copies that object's sprites into that folder.

import sys, getopt
import os.path
from shutil import copyfile

def bit16ToInt(byte1, byte2):
	return int.from_bytes([byte1, byte2], byteorder='little', signed=False)


def bit32ToInt(byte1, byte2, byte3, byte4):
	return int.from_bytes([byte1, byte2, byte3, byte4], byteorder='little', signed=False)


class Object:
	objectId = 0
	spriteIds = []
	
	def __init__(self, objId):
		self.objectId = objId
		self.spriteIds = []
		
class Sprite:
	spriteId = 0
	objectIds = []
	
	def __init__(self, spriteId):
		self.spriteId = spriteId
		self.objectIds = set()


def readData(tibiaDatPath, tibiaSprites, outPath):		
	b = open(tibiaDatPath, "rb").read()

	index = 0
	
	version = bit32ToInt(b[index], b[index + 1], b[index + 2], b[index + 3])
	index += 4
	
	numItems = bit16ToInt(b[index], b[index + 1])
	index += 2
	
	numOutfits = bit16ToInt(b[index], b[index + 1])
	index += 2
	
	numEffects = bit16ToInt(b[index], b[index + 1])
	index += 2
	
	numDistances = bit16ToInt(b[index], b[index + 1])
	index += 2
	
	numObjects = numItems + numOutfits + numEffects + numDistances

	objects = []
	itemId = 100
	maxSpriteId = 0
	print("Reading Tibia.dat...")
	for i in range(0, numObjects):
		if(itemId + i > numObjects):
			break;

		isOutfit = (itemId + i > numItems and itemId + i <= numItems + numOutfits)

		if(isOutfit):
			while(b[index] != 0xff):
				index += 1
				
			index += 1
			
		else:
			while(b[index] != 0xff):
				bCurrent = b[index]
				index += 1
				if(bCurrent == 0x00):
					index += 2
				elif(bCurrent == 0x08):
					index += 2
				elif(bCurrent == 0x09):
					index += 2
				elif(bCurrent == 0x16):
					index += 2
					index += 2
				elif(bCurrent == 0x19):
					index += 4
				elif(bCurrent == 0x1a):
					index += 2
				elif(bCurrent == 0x1d):
					index += 2
				elif(bCurrent == 0x1e):
					index += 2
				elif(bCurrent == 0x21):
					index += 2
				elif(bCurrent == 0x22):
					index += 2
					index += 2
					index += 2
					
					nameLength = bit16ToInt(b[index], b[index + 1])
					index += 2
					
					index += nameLength
					
					index += 2
					index += 2
				elif(bCurrent == 0x23):
					index += 2


			index += 1
		
		hasOutfitThingy = False
		if(isOutfit == True):
			hasOutfitThingy = (bit16ToInt(b[index], b[index + 1]) == 2)
			index += 2
		
		
		width = b[index]
		index += 1
		

		height = b[index]
		index += 1

		
		if(width != 1 or height != 1):
			index += 1
			
		blendFrames = b[index]
		index += 1

		
		
		divX = b[index]
		index += 1
		

		divY = b[index]
		index += 1

		
		divZ = b[index]
		index += 1
		
		animationLength = b[index]
		index += 1



		if(animationLength > 1):
			index += 6			
			index += animationLength * 2 * 4
		
		numSprites = width * height * blendFrames * divX * divY * divZ * animationLength
		sprites = set()
		for sprite in range(0, numSprites):
			sprites.add(bit32ToInt(b[index], b[index + 1], b[index + 2], b[index + 3]))
			index += 4
			
		
		if(isOutfit and hasOutfitThingy):
			index += 1
			
			width = b[index]
			index += 1

			height = b[index]
			index += 1
			
			if(width != 1 or height != 1):
				index += 1
				
			blendFrames = b[index]
			index += 1

			divX = b[index]
			index += 1

			divY = b[index]
			index += 1
			
			divZ = b[index]
			index += 1
			
			animationLength = b[index]
			index += 1
			
			if(animationLength > 1):
				index += 6			
				index += animationLength * 2 * 4
				
			numSprites = width * height * blendFrames * divX * divY * divZ * animationLength
			for sprite in range(0, numSprites):
				sprites.add(bit32ToInt(b[index], b[index + 1], b[index + 2], b[index + 3]))
				index += 4
				

		o = Object(itemId + i)
		for sprite in sprites:
			if(sprite != 0):
				#folder = outDir + "/" + str(itemId + i)
				#spriteFile = tibiaSprites + "/" + str(sprite) + ".png"
				
				#if(os.path.isdir(folder) == False):		
					#os.makedirs(folder) 
				
				#spriteDest = folder + "/" + str(sprite) + ".png"
				#if(os.path.isfile(spriteDest) == False):
					#print(spriteDest)
					#copyfile(spriteFile, spriteDest)
					
				o.spriteIds.append(sprite);
				if(sprite > maxSpriteId):
					maxSpriteId = sprite
				
				
				
		objects.append(o)
				
	
	spriteObjectBindings = []
	spritesPerList = 1000
	numLists = int(maxSpriteId / spritesPerList) + 1
	for i in range(0, numLists):
		spriteObjectBindings.append([])
	
	
	print("Creating bindings...")
	for o in objects:
		for s in o.spriteIds:
			duplicateAlreadyRegistered = False
			dupes = spriteObjectBindings[int(s / spritesPerList)]
			for d in dupes:
				if(d.spriteId == s):
					duplicateAlreadyRegistered = True
					d.objectIds.add(o.objectId)
					break
				
			if(duplicateAlreadyRegistered == False):
				duplicateSprite = Sprite(s)
				duplicateSprite.objectIds.add(o.objectId)
				dupes.append(duplicateSprite)
	

	outStr = ""
	for d in spriteObjectBindings:
		
		for s in d:
			#if(len(s.objectIds) > 1):
			assert(len(s.objectIds) > 0)
			outStr += str(s.spriteId)
			for o in s.objectIds:
				outStr += " " + str(o)
				
			outStr += "\n"

	print("Writing bindings to '" + outPath + "'.")
	f = open(outPath, 'w')
	f.write(outStr)
	f.close()
	
	
			
		


def printHelp():
	print("This script is used in conjunction with the extractSprites script.")
	print("The sprites location is the output directory of that script.")
	print('Usage: extractData.py -i <Tibia.dat path> -s <sprites directory> -o <output directory path>')

def main(argv):
	tibiaDat = '';
	tibiaSprites = '';
	outPath = '';
	try:
		opts, args = getopt.getopt(argv,"hi:s:o:")
	except getopt.GetoptError:
		printHelp();
		sys.exit(2)
	
	argCount = 0
	for opt, arg in opts:
		if opt == '-h':
			printHelp();
			sys.exit()
		elif opt in ("-i"):
			tibiaDat = arg
			argCount += 1
		elif opt in ("-s"):
			tibiaSprites = arg
			argCount += 1
		elif opt in ("-o"):
			outPath = arg
			argCount += 1

	if(argCount < 3):
		printHelp()
		sys.exit()

	if(os.path.isfile(tibiaDat) == False):
		print("Could not find file '", tibiaDat, "'.")
		sys.exit(2)
		
	if(os.path.isdir(tibiaSprites) == False):
		print("Could not find directory '", tibiaSprites, "'.")
		sys.exit(2)

	readData(tibiaDat, tibiaSprites, outPath)



if(__name__ == "__main__"):
	main(sys.argv[1:])
