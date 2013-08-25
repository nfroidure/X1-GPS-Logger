# X1 Intellitrac series GPS logger  [![Build Status](https://travis-ci.org/nfroidure/X1-GPS-Logger.png?branch=master)](https://travis-ci.org/nfroidure/X1-GPS-Logger)
This is a small program that log the given X1 Series GPS box to the specified file. Used with xinetd it brings a lightweigth server simply logging your fleet positions.

## Sample xinetd conf file
	# default: on
	# description: X1 GPS tracking server
	service x1server
	{
		port                    = 3333
		socket_type             = stream
		wait                    = no
		user                    = x1server
		server                  = /path/to/bin/x1server
		log_on_success          += USERID
		log_on_failure          += USERID
		disable                 = no
		instances               = 300
	}

