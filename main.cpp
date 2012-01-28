#include <iostream>
#include <string>
#include <fstream>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

sigjmp_buf sigalarm_context;

void sigalarm_handler(int unused);
int main();

void sigalarm_handler(int unused)
	{
	siglongjmp(sigalarm_context, 1);
	}

int main()
	{
	// Setting timeout
	struct sigaction action;
	action.sa_handler=sigalarm_handler;
	action.sa_flags=0;
	sigfillset(&action.sa_mask);
	sigaction(SIGALRM, & action, NULL);
	if(sigsetjmp(sigalarm_context,1)==0)
		{
		alarm(120);
		// Read and output the sync message
		char buffer[8];
		read (0, buffer, sizeof buffer);
		write (1, buffer, sizeof buffer);
		alarm(0);
		// Write line by line to the log file
		alarm(600);
		std::string line;
		std::cin >> line;
		alarm(0);
		// Finding device name
		std::string device;
		std::size_t pos=line.find(',');
		if(pos==std::string::npos)
			{
			//std::cerr << "No comma found 1\n";
			exit(EXIT_FAILURE);
			}
		device=std::string(line, 0, pos);
		if(device.length()<5) // crap fix
			{
			alarm(600);
			std::cin >> line;
			alarm(0);
			pos=line.find(',');
			if(pos==std::string::npos)
				{
				//std::cerr << "No comma found 2\n";
				exit(EXIT_FAILURE);
				}
			device=std::string(line, 0, pos);
			}
		// Getting current date
		char date[9];
		time_t timestamp = time(NULL);
		strftime(date, sizeof(date), "%Y%m%d", localtime(&timestamp));
		// Creating log filename
		std::string filename;
		filename="/var/www/restfor/log/x1"+device+"-"+std::string(date)+".log.txt";
		//filename="/home/elitwork/restfor/log/x1"+device+"-"+std::string(date)+".log.txt";
		std::ofstream outputFile( filename.c_str() , std::ios::app);
		while(1)
			{
			if(line=="end")
				break;
			// Removing output and input values
			pos=line.find_last_of(',');
			if(pos==std::string::npos)
				exit(EXIT_FAILURE);
			line = line.substr(0,pos);
			pos=line.find_last_of(',');
			if(pos==std::string::npos)
				exit(EXIT_FAILURE);
			line = line.substr(0,pos);
			// Removing datetime given by the device
			pos=line.find(',');
			if(pos==std::string::npos)
				exit(EXIT_FAILURE);
			line = line.substr(0,pos) + line.substr(pos+15,line.length()-1);
			// Time is in the line, maybe trust this information is better ? maybe delete device date (n others) to decrease log weight ?
			char curtime[9];
			time_t timestamp = time(NULL);
			strftime(curtime, sizeof(curtime), "%X", localtime(&timestamp));
			outputFile << std::string(curtime) << "," << line << std::endl;
			alarm(1200);
			std::cin >> line;
			alarm(0);
			}
		}
	else
		{
		//std::cerr << "X1Server (" << getpid() << ") : Read timed out\n";
		exit(EXIT_FAILURE);
		}
	// Exit
	return EXIT_SUCCESS;
	}
