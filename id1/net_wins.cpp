/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_wins.c

#include "quakedef.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#include "net_wins.h"
#include "net_sock.h"

#pragma comment(lib, "Ws2_32.lib")

extern cvar_t hostname;

#define MAXHOSTNAMELEN		256

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

static in6_addr myAddr;
static bool myAddr_initialized = false;

qboolean	winsock_lib_initialized;

int winsock_initialized = 0;
WSADATA		winsockdata;

int WINS_OpenIPV4Socket(int port);

//=============================================================================

void WINS_GetLocalAddress()
{
	struct hostent	*local = NULL;
	char			buff[MAXHOSTNAMELEN];
	unsigned long	addr;

	if (myAddr_initialized)
	{
		return;
	}

	if (gethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
		return;

	addrinfo hints{ };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	addrinfo* results = nullptr;
	auto err = getaddrinfo(buff, "0", &hints, &results);
	if (err < 0)
	{
		return;
	}

	addrinfo* result;
	for (result = results; result != nullptr; result = result->ai_next)
	{
		if (result->ai_family == AF_INET)
		{
			auto address = ((sockaddr_in*)result->ai_addr)->sin_addr;
			myAddr = in6addr_v4mappedprefix;
			myAddr.u.Byte[12] = address.S_un.S_un_b.s_b1;
			myAddr.u.Byte[13] = address.S_un.S_un_b.s_b2;
			myAddr.u.Byte[14] = address.S_un.S_un_b.s_b3;
			myAddr.u.Byte[15] = address.S_un.S_un_b.s_b4;
			myAddr_initialized = true;
			inet_ntop(AF_INET, &address, my_tcpip_address, NET_NAMELEN);
			break;
		}
	}

	freeaddrinfo(results);
}


int WINS_Init (void)
{
	int		i;
	char	buff[MAXHOSTNAMELEN + 1];
	char	*p;
	int		r;
	WORD	wVersionRequested;

	winsock_lib_initialized = true;

	if (COM_CheckParm ("-noudp"))
		return -1;

	if (winsock_initialized == 0)
	{
		wVersionRequested = MAKEWORD(2, 2); 

		r = WSAStartup (wVersionRequested, &winsockdata);

		if (r)
		{
			Con_SafePrintf ("Winsock initialization failed.\n");
			return -1;
		}
	}
	winsock_initialized++;

	// determine my name
	if (gethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
	{
		Con_DPrintf ("Winsock TCP/IP Initialization failed.\n");
		if (--winsock_initialized == 0)
			WSACleanup ();
		return -1;
	}

	// if the quake hostname isn't set, set it to the machine name
	if (hostname.string == "UNNAMED")
	{
		// see if it's a text IP address (well, close enough)
		for (p = buff; *p; p++)
			if ((*p < '0' || *p > '9') && *p != '.')
				break;

		// if it is a real name, strip off the domain; we only want the host
		if (*p)
		{
			for (i = 0; i < 15; i++)
				if (buff[i] == '.')
					break;
			buff[i] = 0;
		}
		Cvar_Set ("hostname", buff);
	}

	i = COM_CheckParm ("-ip");
	if (i)
	{
		if (i < com_argc-1)
		{
			auto err = inet_pton(AF_INET6, com_argv[i+1], &myAddr);
			if (err <= 0)
			{
				in_addr address { };
				unsigned long addr = 0;
				err = inet_pton(AF_INET, com_argv[i+1], &address);
				if (err <= 0)
				{
					Sys_Error("%s is not a valid IP address", com_argv[i + 1]);
				}
				myAddr = in6addr_v4mappedprefix;
				myAddr.u.Byte[12] = address.S_un.S_un_b.s_b1;
				myAddr.u.Byte[13] = address.S_un.S_un_b.s_b2;
				myAddr.u.Byte[14] = address.S_un.S_un_b.s_b3;
				myAddr.u.Byte[15] = address.S_un.S_un_b.s_b4;
				myAddr_initialized = true;
			}
			else
			{
				myAddr_initialized = true;
			}
			strcpy(my_tcpip_address, com_argv[i+1]);
			Cvar_Set("hostname", my_tcpip_address);
		}
		else
		{
			Sys_Error ("NET_Init: you must specify an IP address after -ip");
		}
	}
	else
	{
		WINS_GetLocalAddress();
	}

	if ((net_controlsocket = WINS_OpenIPV4Socket (0)) == -1)
	{
		Con_Printf("WINS_Init: Unable to open control socket\n");
		if (--winsock_initialized == 0)
			WSACleanup ();
		return -1;
	}

	broadcastaddr.data.resize(sizeof(sockaddr_in));
	auto address = (sockaddr_in*)broadcastaddr.data.data();
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_BROADCAST;
	address->sin_port = htons((unsigned short)net_hostport);

	Con_Printf("Winsock TCP/IP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void WINS_Shutdown (void)
{
	WINS_Listen (false);
	WINS_CloseSocket (net_controlsocket);
	if (--winsock_initialized == 0)
		WSACleanup ();
}

//=============================================================================

void WINS_Listen (qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		WINS_GetLocalAddress();
		if ((net_acceptsocket = WINS_OpenSocket (net_hostport)) == -1)
			Sys_Error ("WINS_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	WINS_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int WINS_OpenSocket (int port)
{
	int newsocket;
	qsockaddr addr;
	u_long _true = 1;
	int off = 0;
	sockaddr_in6* address = nullptr;

	if ((newsocket = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	if (setsockopt(newsocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off)) == -1)
		goto ErrorReturn;

	if (ioctlsocket (newsocket, FIONBIO, &_true) == -1)
		goto ErrorReturn;

	addr.data.resize(sizeof(sockaddr_in6));
	address = (sockaddr_in6*)addr.data.data();
	address->sin6_family = AF_INET6;
	address->sin6_addr = in6addr_any;
	address->sin6_port = htons((unsigned short)port);
	if( bind (newsocket, (const sockaddr*)address, (int)addr.data.size()) == 0)
		return newsocket;

	Sys_Error ("Unable to bind to %s", SOCK_AddrToString(&addr));
ErrorReturn:
	closesocket (newsocket);
	return -1;
}

//=============================================================================

int WINS_OpenIPV4Socket(int port)
{
	int newsocket;
	qsockaddr addr;
	u_long _true = 1;
	sockaddr_in* address = nullptr;

	if ((newsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	if (ioctlsocket(newsocket, FIONBIO, &_true) == -1)
		goto ErrorReturn;

	addr.data.resize(sizeof(sockaddr_in));
	address = (sockaddr_in*)addr.data.data();
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons((unsigned short)port);
	if (bind(newsocket, (const sockaddr*)address, (int)addr.data.size()) == 0)
		return newsocket;

	Sys_Error("Unable to bind to %s", SOCK_AddrToString(&addr));
ErrorReturn:
	closesocket(newsocket);
	return -1;
}

//=============================================================================

int WINS_CloseSocket (int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	return closesocket (socket);
}

//=============================================================================

int WINS_CheckNewConnections (void)
{
	char buf[4096];

	if (net_acceptsocket == -1)
		return -1;

	if (recvfrom (net_acceptsocket, buf, sizeof(buf), MSG_PEEK, NULL, NULL) > 0)
	{
		return net_acceptsocket;
	}
	return -1;
}

//=============================================================================

int WINS_Read (int socket, std::vector<byte>& buf, struct qsockaddr *addr)
{
	u_long available = 0;
	auto err = ioctlsocket(socket, FIONREAD, &available);
	if (err < 0)
	{
		return err;
	}
	if (available == 0)
	{
		return 0;
	}
	auto len = (int)available;
	buf.resize(len);
	int addrlen;
	if (socket == net_controlsocket)
	{
		addrlen = (int)sizeof(sockaddr_in);
	}
	else
	{
		addrlen = (int)sizeof(sockaddr_in6);
	}
	addr->data.resize(addrlen);
	recvfrom(socket, (char*)buf.data(), len, 0, (sockaddr*)addr->data.data(), &addrlen);
	return len;
}

//=============================================================================

int WINS_MakeSocketBroadcastCapable (int socket)
{
	int	i = 1;

	// make this socket broadcast capable
	if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int WINS_Broadcast (int socket, byte *buf, int len)
{
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		WINS_GetLocalAddress();
		ret = WINS_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return WINS_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int WINS_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret;
	ret = sendto (socket, (char*)buf, len, 0, (sockaddr*)addr->data.data(), (int)addr->data.size());
	if (ret == -1)
	{
		auto err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			return 0;
	}
	return ret;
}

//=============================================================================

int WINS_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	return SOCK_GetSocketAddr(socket, net_controlsocket, myAddr, addr);
}

//=============================================================================

int WINS_GetAddrFromName(const char *name, struct qsockaddr *addr)
{
	return SOCK_GetAddrFromName(name, myAddr, addr);
}
