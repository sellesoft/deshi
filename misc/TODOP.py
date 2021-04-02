import os
import sys
import fileinput
from github import Github
import github
from flask import Flask, request, Response
import json




src_dir = "C:\\Users\\sushi\\Documents\\GitHub\\deshi\\src"

ignore_dirs = ["imgui", "saschawillems", "draudio", "stb", "tinyobjloader"] #put the name of folders you want to ignore here

def find_files(dir_name, exts):
	filepaths = []
	for root, dirs, files in os.walk(dir_name):
		if os.path.split(root)[1] not in ignore_dirs:	
			print(os.path.split(root)[1])
			for file in files:
				for ext in exts:
					if file.endswith(ext):
						filepaths.append(os.path.join(root, file))
	return filepaths

def getTODOs(file):
	TODOs = []
	line_num = 0
	for line in file:
		if "TODO(" in line:
			TODO = line[line.find("TODO("):]
			arguments = TODO[TODO.find("(") + 1:TODO.find(")")].split(",")
			body = TODO[TODO.find(")") + 1:]
			TODOs.append((file.name, line_num, arguments, body))
		line_num += 1
	return TODOs
	
def getTags(tagsList):
	Tags = []
	for tag in tagsList:
		if "Ph" in tag:
			Tags.append("physics")
		if "Re" in tag:
			Tags.append("render")
		if "En" in tag:
			Tags.append("entity")
		if "In" in tag:
			Tags.append("input")
		if "Ma" in tag:
			Tags.append("math")
		if "Op" in tag:
			Tags.append("optimization")
		if "Ge" in tag:
			Tags.append("general")
		if "Cl" in tag:
			Tags.append("clean up")
		if "UI" in tag:
			Tags.append("UI")
		if "Vu" in tag:
			Tags.append("Vulkan")
		if "Sh" in tag:
			Tags.append("shader")
		if "Co" in tag:
			Tags.append("core")
		if "Geo" in tag:
			Tags.append("geometry")
		if "So" in tag:
			Tags.append("sound")
		if "Fs" in tag:
			Tags.append("filesystem")
		if "Cmd" in tag:
			Tags.append("command")
		if "Con" in tag:
			Tags.append("console")
		if "Wi" in tag:
			Tags.append("window")
		if "Fu" in tag:
			Tags.append("fun")
		if "Oth" in tag:
			Tags.append("other")
	if len(Tags) == 0:
		Tags.append("No Tags")
	return Tags



TODOs = []
	
print("\n-Collecting TODOs")
	
filePaths = find_files(src_dir, ['.cpp', '.h'])
for filePath in filePaths:
	file = open(filePath, 'r+')
	TODOs.extend(getTODOs(file))
#print(filePaths)
	
print("-Collected %d TODOs from %d files" % (len(TODOs), len(filePaths)))

#make room for listing number of TODOs later
TODOList = open("TODOs.txt", "w+")
TODOList.write("TODO List successfully generated with " + str(len(TODOs)) + " TODOs found.\n\n")
TODO_num = 0
for file_name, line_num, arguments, body in TODOs:

	#writes all the TODOs to the file with formatting
	TODOList.write("~~~~~~~ TODO " + str(TODO_num) + " ~~~~~~~\n")
	#write the creator's name
	TODOList.write("Creator: " + arguments[0] + "\n")
	TODOList.write("----------------------\n")
	Tags = []
	if len(arguments) >= 2:
		Tags = getTags(arguments[1])
	print(file_name, line_num, body)
	#write the file the TODO was found in and what line
	TODOList.write(file_name[file_name.rfind("\\") + 1:] + ", Line: " + str(line_num) + "\n")
	
	#write the date signed on the TODO
	if len(arguments) >= 3: TODOList.write("Date: " + arguments[2] + "\n")
	#write tags
	if len(Tags) > 0:
		TODOList.write("Tags: ")
		for i, tag in enumerate(Tags):
			TODOList.write(tag)
			if i != len(Tags) - 1: TODOList.write(", ")
	else:
		TODOList.write("No tags.")
	TODOList.write("\n----------------------\n")
	if body[0] == " ":
		TODOList.write(body[1:])
	else:
		TODOList.write(body)
	TODOList.write("\n")
	TODO_num += 1
		
	#updates all the TODO issues
		


#updates the repo's TODO.txt file
TODOList.seek(0)

	
	
