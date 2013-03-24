#include <iostream>
#include <string>
#include <fstream>
#include <time.h>
#include <algorithm>

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
		char buffer[9];
		read (0, buffer, 8);
		write (1, buffer, 8);
		buffer[8]=NULL;
		syslog(LOG_INFO, std::string("Buffer set : '"+std::string(buffer)+"', v5.").c_str());
		alarm(0);
		// Write line by line to the log file
		alarm(400);
		//syslog(LOG_INFO, std::string("Waiting for the first line.").c_str());
		std::string line;
		std::cin >> line;
		//syslog(LOG_INFO, std::string("First line is '"+line+"'.").c_str());
		alarm(0);
		// Finding device name
		std::size_t pos=line.find(',');
		if(pos==std::string::npos)
			{
			syslog(LOG_INFO, std::string("No comma found in '"+line+"', case 1.").c_str());
			}
		else if(pos==2) // The sync message was part of the line
			{
			std::string newLine;
			newLine=std::string(buffer);
			newLine.append(line);
			line=newLine;
			pos=line.find(',');
			device=std::string(line, 0, pos);
			syslog(LOG_INFO, std::string("Device is '"+device+"', case 1.").c_str());
			}
		else
			{
			device=std::string(line, 0, pos);
			syslog(LOG_INFO, std::string("Device is '"+device+"', case 2.").c_str());
			}
		// Retry 1 time if no device found
		if(device.length()<5)
			{
			alarm(400);
			//syslog(LOG_INFO, std::string("Waiting for the second line.").c_str());
			std::cin >> line;
			//syslog(LOG_INFO, std::string("Second line is '"+line+"'.").c_str());
			alarm(0);
			pos=line.find(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string("No comma found in '"+line+"', case 2, exiting.").c_str());
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
		//filename="/home/ecogom/vigisystem/log/x1-"+device+"-"+std::string(date)+".log";
		std::ofstream outputFile( filename.c_str() , std::ios::app);
		//syslog(LOG_INFO, std::string("Opening file '"+filename+"'.").c_str());
		while(1)
			{
			if(line=="end")
				break;
			// Removing output and input values
			pos=line.find_last_of(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string(device+": No comma found in line '"+line+"', case 3.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
			line = line.substr(0,pos);
			pos=line.find_last_of(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string(device+": No comma found in line '"+line+"', case 4.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
			line = line.substr(0,pos);
			// Removing datetime given by the device
			pos=line.find(',');
			if(pos==std::string::npos)
				{
				syslog(LOG_INFO, std::string(device+": No comma found in line '"+line+"', case 5.").c_str());
				closelog();
				exit(EXIT_FAILURE);
				}
			line = line.substr(0,pos) + line.substr(pos+15,line.length()-1);
			// Time is in the line, maybe trust this information is better ?
			if(std::count(line.begin(), line.end(), ',')>4)
				{
				char curtime[9];
				time_t timestamp = time(NULL);
				strftime(curtime, sizeof(curtime), "%X", localtime(&timestamp));
				outputFile << std::string(curtime) << "," << line << std::endl;
				syslog(LOG_INFO, std::string("Logging "+line+".").c_str());
				alarm(1200);
				std::cin >> line;
				alarm(0);
				}
			else
				syslog(LOG_INFO, std::string("Not logged "+line+".").c_str());
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
