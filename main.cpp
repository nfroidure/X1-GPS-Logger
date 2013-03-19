#include <iostream>
#include <string>
#include <fstream>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <syslog.h>

sigjmp_buf sigalarm_context;

void sigalarm_handler(int unused);
int main();
std::string device;

void sigalarm_handler(int unused)
	{
	siglongjmp(sigalarm_context, 1);
	}

int main()
	{
	// Opening a logging session
	openlog("X1Logger", LOG_PID, LOG_DAEMON);
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
		std::size_t pos=line.find(',');
		if(pos==std::string::npos)
			{
			syslog(LOG_INFO, std::string("No comma found in '"+line+"'. Step 3.").c_str());
			}
		else
			device=std::string(line, 0, pos);
		// Retry 1 time if no device found
		if(device.length()<5)
			{
			alarm(600);
			std::cin >> line;
			alarm(0);
			pos=line.find(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string("No comma found in '"+line+"', exiting.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
			else
				device=std::string(line, 0, pos);
			}
		// If still no device, exit.
		if(device.length()<5)
			{
			syslog(LOG_INFO, std::string("No device name, exiting.").c_str());
			closelog();
			exit(EXIT_FAILURE);
			}
		// Getting current date
		char date[9];
		time_t timestamp = time(NULL);
		strftime(date, sizeof(date), "%Y%m%d", localtime(&timestamp));
		// Creating log filename
		std::string filename;
		filename="/var/www/restfor/log/x1-"+device+"-"+std::string(date)+".log";
		//filename="/home/elitwork/restfor/log/x1-"+device+"-"+std::string(date)+".log";
		std::ofstream outputFile( filename.c_str() , std::ios::app);
		while(1)
			{
			if(line=="end")
				break;
			// Removing output and input values
			pos=line.find_last_of(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string(device+": No comma found in line '"+line+"'. Step 2.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
			line = line.substr(0,pos);
			pos=line.find_last_of(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string(device+": No comma found in line '"+line+"'. Step 2.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
			line = line.substr(0,pos);
			// Removing datetime given by the device
			pos=line.find(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string(device+": No comma found in line '"+line+"'. Step 3.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
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
		syslog(LOG_INFO, std::string(device+": Read timed out").c_str());
		closelog();
		exit(EXIT_FAILURE);
		}
	// Exit
	return EXIT_SUCCESS;
	}
