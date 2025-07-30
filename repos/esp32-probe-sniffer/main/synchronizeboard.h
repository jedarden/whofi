#pragma once

class SynchronizeBoard
{
	static void initialize_sntp(void);

public:
	static void obtain_time(void);
	static void print_time(void);
	/**
	  * @brief Utility function to get current time in struct tm format
	  *
	  * @return    
	  *    - struct tm containing current time
	  */  
	static struct tm getTime(void);
	/**
	  * @brief Convert time passed in tm format to a string human readable format
	  *
	  * @param     time	raw sniffed packet
	  * @param     timestampCStr string passed by reference which will be copied
	  *			   the value of time	
	  */  
	static void tmToCStr(struct tm time, char (&timestampCStr)[64]);
 
};
