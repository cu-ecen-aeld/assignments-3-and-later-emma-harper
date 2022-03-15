#!/bin/sh
#This script first checks to see if both arguments are specified with
#Argument one being the path to a file, if path does not exist then it returns an error
#Argument two is a text string to search in the specified file
#The output of the script is the number of files in that directory and the
#number of matching lines in the file
#Author: Emma Harper
#Date: 01/11/22

#Check if both parameters are specified
if [ ! $# -eq 2 ]	
then
	if [ $# -eq 0 ]
	then
		echo "Error: No parameters specified"
	#if not see if first argument is a file and is a string
	elif [ $# -lt 2 ] && [ -n $1 ]
	then
		echo "Error: Not enough parameters - missing first argument, the path to file directory"
	#if not see if first argument is a file and is a directory
	elif [ $# -lt 2 ] && [ -d $1 ] 
	then
		echo "Error: Not enough parameters - missing second argument, the string to search"
	elif [ $# -gt 2 ]
	then
		echo "Error: Only 2 parameters taken" 
	fi
	echo "Argument format is <path/to/directory> <string_to_search>"

	exit 1
fi

#Check if first argument is a directory
if [ ! -d $1 ]
then
	echo "Error: Argument one '$1' is not a directory"
	exit 1
fi

#Check if second argument is a string
if [ ! -n $2 ]
then
	echo "Error: Argument two '$2' is not a directory"
	exit 1
fi

#If all arguments checkout then print number of files in directory and number of matching lines for string
echo "The number of files are $(find "$1" -type f | wc -l) and the number of matching lines are $(grep -or "$2" "$1" | wc -w)"
exit 0
#for testing autos