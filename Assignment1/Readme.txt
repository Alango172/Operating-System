Name: Chaofan Cai
V-Number: V00940471
Section: CSC 360 A01

There are 5 files submitted: 
	main.c
	linked_list.c
	linked_list.h
	Makefile
	test.c
	
To successfully run the PMan programm, you just need to type "make" into the command line.
The Makefile will delete all the existing exe files and compile the new C files for you.
Type ./PMan to the command line to execute the PMan program.

The PMan support the following commands:
	bg command(e.g. bg ./test, bg ls)
	bglist
	bgkill PID(the pid of the process created by bg command) 
	bgstart PID
	bgstop PID  
	pstat PID
	q (for exiting the programm)

When the PMan programm is running,choose one of the command listed above and type them after "PMan>" to test the 
functionality of the programm.
I Strongly recommend to bg a process first so you can have a valid pid to execute the other commands.

1. bg command supports all the linux command like ls, ps, cd. And can also execute the .exe files. For this situation,
   I recommend you to run bg ./test since it is a programm that can run for a very long time if no interruption is made 
   by users. Except that, navigation and finding commands like ls and ps will be terminaned quickly, so they will not be
   shown in the process list
   
2. bglist simply prints all the process created by the bg and gives a total number of existing process in the list. 

3. bgkill PID will kill a process and delete node from the list. If PID is invalid(not exist) the programm will report 
   an error of "no process exist" 
   
4. bgstart PID will resume a hanged process stopped by bgstop

5. bgstop PID will stop a process from sleeping or running status

6. pstat PID receives a valid PID and reads some basic information from /proc/pid/status and /proc/pid/stat including:
	name, status, utime, stime, rss, voluntary_ctxt_switches, nonvoluntary_ctxt_switches
   
