#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
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
unsigned int deviceId;

void sigalarm_handler(int unused)
	{
	siglongjmp(sigalarm_context, 1);
	}

int main()
	{
	// Opening a logging session
	openlog("X1Logger", LOG_PID, LOG_DAEMON);
//	syslog(LOG_INFO, std::string("V6").c_str());
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
			alarm(1200);
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
			sbuffer[8]=NULL;
			// Test if buffer is a sync header
			if(buffer[1] == (unsigned char) 0xF8&&buffer[0] == (unsigned char) 0xFA)
				{
				// Echoing the header
				//write (1, buffer, sizeof buffer);
				size_t written;
				written = fwrite (buffer, (size_t) 1, sizeof buffer, stdout);
				fflush(stdout);
				alarm(0);
				//sleep(10);
				//syslog(LOG_INFO, std::string("Sync header '"+std::string(sbuffer)+"'.").c_str());
				// Getting the device id
				deviceId = ((buffer[7] << 24)
						+(buffer[6] << 16)
						+(buffer[5] << 8)
						+(buffer[4]));
				std::stringstream ss;
				ss << "Device id is '" << deviceId << "', written " << written << " bytes.";
				syslog(LOG_INFO, std::string(ss.str()).c_str());
				exit(EXIT_SUCCESS);
				continue;
				}
			// it's a line
			else
				{
				// Getting the end of the line
				std::string curline;
				std::cin >> curline;
				syslog(LOG_INFO, std::string("Curline is '"+curline+"'.").c_str());
				alarm(0);
				// Merging
				std::string line;
				line.append(std::string(sbuffer));
				line.append(curline);
				syslog(LOG_INFO, std::string("First line is '"+line+"'.").c_str());
				std::size_t pos;
				std::ofstream outputFile;
				// Finding device name
				if(device.empty())
					{
					pos=line.find(',');
					if(pos==std::string::npos)
						{
						syslog(LOG_INFO, std::string("No comma found in '"+line+"', case 1.").c_str());
						closelog();
						exit(EXIT_FAILURE);
						}
					device=std::string(line, 0, pos);
					syslog(LOG_INFO, std::string("Device is '"+device+"', cought from a line.").c_str());
					// Testing device name
					if(device.length()<5)
						{
						syslog(LOG_INFO, std::string("Bad device name.").c_str());
						closelog();
						exit(EXIT_FAILURE);
						continue;
						}
					// Getting current date
					char date[9];
					time_t timestamp = time(NULL);
					strftime(date, sizeof(date), "%Y%m%d", localtime(&timestamp));
					// Creating log filename
					std::string filename;
					filename="/var/www/restfor/log/x1-"+device+"-"+std::string(date)+".log";
					//filename="/home/ecogom/vigisystem/log/x1-"+device+"-"+std::string(date)+".log";
					outputFile.open(filename.c_str(), std::ios::app);
					syslog(LOG_INFO, std::string("Opening file '"+filename+"'.").c_str());
					}
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
					continue;
					}
				else
					{
					syslog(LOG_INFO, std::string("Not logged "+line+".").c_str());
					exit(EXIT_FAILURE);
					}
				}
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
