#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <time.h>
#include <algorithm>
#include <dirent.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <syslog.h>

sigjmp_buf sigalarm_context;

void sigalarm_handler(int unused);
void exitWell(int status, std::string message);
int main(int argc, char **argv);
std::string device;
std::string logdir;
unsigned int deviceId;
std::ofstream outputFile;

void sigalarm_handler(int unused)
	{
	siglongjmp(sigalarm_context, 1);
	}

void exitWell(int status, std::string message)
	{
	syslog(LOG_INFO, std::string(device+": "+message).c_str());
	if(outputFile.is_open())
		outputFile.close();
	closelog();
	exit(status);
	}

int main(int argc, char **argv)
	{
	// Catching log destination
	if(argc>1)
		{
		logdir=std::string(argv[1]);
		}
	else
		logdir="/var/log";
	// Opening a logging session
	openlog("X1Logger", LOG_PID, LOG_DAEMON);
	syslog(LOG_DEBUG, std::string("V6").c_str());
	// Testing logdir
	DIR* dir = opendir(logdir.c_str());
	if (errno==ENOENT)
		{
		exitWell(EXIT_FAILURE,"Given logdir is not a folder.");
		}
	closedir(dir);
	// Setting timeout
	struct sigaction action;
	action.sa_handler=sigalarm_handler;
	action.sa_flags=0;
	sigfillset(&action.sa_mask);
	sigaction(SIGALRM, & action, NULL);
	if(sigsetjmp(sigalarm_context,1)==0)
		{
		while(1)
			{
			unsigned char buffer[8];
			alarm(200);
			// Reading the header
			//read (0, buffer, sizeof buffer);
			fread (buffer, (size_t) 1, sizeof buffer, stdin);
			// Converting buffer to c string
			char sbuffer[9];
			sbuffer[0]=(char) buffer[0];
			sbuffer[1]=(char) buffer[1];
			sbuffer[2]=(char) buffer[2];
			sbuffer[3]=(char) buffer[3];
			sbuffer[4]=(char) buffer[4];
			sbuffer[5]=(char) buffer[5];
			sbuffer[6]=(char) buffer[6];
			sbuffer[7]=(char) buffer[7];
			sbuffer[8]=(char) NULL;
			// Test if buffer is a sync header
			if(buffer[1] == (unsigned char) 0xF8&&buffer[0] == (unsigned char) 0xFA)
				{
				// Echoing the header
				//write (1, buffer, sizeof buffer);
				size_t written;
				written = fwrite (buffer, (size_t) 1, sizeof buffer, stdout);
				fflush(stdout);
				alarm(0);
				syslog(LOG_DEBUG, std::string("Sync header '"+std::string(sbuffer)+"'.").c_str());
				// Getting the device id
				deviceId = ((buffer[7] << 24)
						+(buffer[6] << 16)
						+(buffer[5] << 8)
						+(buffer[4]));
				std::stringstream ss;
				ss << "Device id is '" << deviceId << "', written " << written << " bytes.";
				syslog(LOG_DEBUG, std::string(ss.str()).c_str());
				continue;
				}
			// it's a line
			else
				{
				// Getting the end of the line
				std::string curline;
				std::cin >> curline;
				syslog(LOG_DEBUG, std::string("Curline is '"+curline+"'.").c_str());
				alarm(0);
				// Merging
				std::string line;
				line.append(std::string(sbuffer));
				line.append(curline);
				while((!line.empty())&&(line[0]=='\r'||line[0]=='\n'))
					line.erase(0,1);
				syslog(LOG_DEBUG, std::string("First line is '"+line+"'.").c_str());
				std::size_t pos;
				// Finding device name
				if(device.empty())
					{
					pos=line.find(',');
					if(pos==std::string::npos)
						{
						exitWell(EXIT_FAILURE,"No comma found in '"+line+"', case 1.");
						}
					device=std::string(line, 0, pos);
					syslog(LOG_DEBUG, std::string("Device is '"+device+"', cought from a line.").c_str());
					// Testing device name
					if(device.length()<5)
						{
						exitWell(EXIT_FAILURE,"Bad device name.");
						}
					// Getting current date
					char date[9];
					time_t timestamp = time(NULL);
					strftime(date, sizeof(date), "%Y%m%d", localtime(&timestamp));
					// Creating log filename
					std::string filename;
					filename=logdir+"/x1-"+device+"-"+std::string(date)+".log";
					outputFile.open(filename.c_str(), std::ios::app);
					if(!outputFile.is_open())
						exitWell(EXIT_FAILURE,"Failed opening file '"+filename+"'.");
					syslog(LOG_DEBUG, std::string("Opening file '"+filename+"'.").c_str());
					}
				// Removing output and input values
				pos=line.find_last_of(',');
				if(pos==std::string::npos)
					{
					exitWell(EXIT_FAILURE,"No comma found in line '"+line+"', case 3.");
					}
				line = line.substr(0,pos);
				pos=line.find_last_of(',');
				if(pos==std::string::npos)
					{
					exitWell(EXIT_FAILURE,"No comma found in line '"+line+"', case 4.");
					}
				line = line.substr(0,pos);
				// Removing datetime given by the device
				pos=line.find(',');
				if(pos==std::string::npos)
					{
					exitWell(EXIT_FAILURE,"No comma found in line '"+line+"', case 5.");
					}
				line = line.substr(0,pos) + line.substr(pos+15,line.length()-1);
				// Time is in the line, maybe trust this information is better ?
				if(std::count(line.begin(), line.end(), ',')>4)
					{
					char curtime[9];
					time_t timestamp = time(NULL);
					strftime(curtime, sizeof(curtime), "%X", localtime(&timestamp));
					if(!outputFile.is_open())
						exitWell(EXIT_FAILURE,"Failed writing to file.");
					outputFile << std::string(curtime) << "," << line << std::endl;
					syslog(LOG_DEBUG, std::string("Logging "+line+".").c_str());
					continue;
					}
				else
					{
					exitWell(EXIT_FAILURE,"Not logged "+line+".");
					}
				}
			}
		}
	else
		{
		exitWell(EXIT_FAILURE, std::string(device+": Read timed out").c_str());
		}
	// Exit
	return EXIT_SUCCESS;
	}
