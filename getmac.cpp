//////////////////////////////////////////////////////////////////////
//
// getmac.cpp
//
// Developed by Gabriel Ferri.
//
// This source code may be used, modified, compiled, duplicated, and/or
// distributed without restriction provided this copyright notice remains intact.
// Cyotec Systems Limited and/or its employees cannot be held responsible for any
// direct or indirect damage or loss of any kind that may result from using this
// code, and provides no warranty, guarantee, or support.
//
//////////////////////////////////////////////////////////////////////
#include "getmac.h"

#ifdef WIN32
	#pragma comment( lib, "rpcrt4.lib" )
	#include <rpc.h>
#else
	#include <stdio.h>
	#include <fcntl.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <linux/if.h>
#endif

#include <mss.h>


int GetMACAddress( unsigned char *mac_address )
{
	#ifdef WIN32
		UUID uuid;
		UuidCreateSequential( &uuid );    // Ask OS to create UUID
	
		for (int i=2; i<8; i++)  // Bytes 2 through 7 inclusive are MAC address
			mac_address[i - 2] = uuid.Data4[i];
	#else
		struct ifreq ifr;
		struct ifreq *IFR;
		struct ifconf ifc;
		char buf[1024];
		int s, i;
		int ok = 0;

		s = socket(AF_INET, SOCK_DGRAM, 0);
		if( s==-1 )
			return -1;

		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = buf;
		ioctl(s, SIOCGIFCONF, &ifc);
 
		IFR = ifc.ifc_req;
		for( i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++)
		{
			strcpy(ifr.ifr_name, IFR->ifr_name);
			if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0) {
				if (! (ifr.ifr_flags & IFF_LOOPBACK)) {
					if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
						ok = 1;
						break;
					}
				}
			}
		}

		close(s);
		if( ok )
			bcopy( ifr.ifr_hwaddr.sa_data, mac_address, 6);
		else
			return -1;
		return 0;
	#endif
}
