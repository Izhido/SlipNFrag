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
// net_udp.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <cerrno>

#ifdef __sun__
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#ifdef __APPLE__
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#define USE_IFADDRS
#elif defined(__linux__)
#include <ifaddrs.h>
#include <net/if.h>
#define USE_IFADDRS
#else
extern "C" int gethostname (char *, int);
#endif

#include "net_udp.h"
#include "net_sock.h"

extern "C" int close (int);

extern cvar_t hostname;

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

static in6_addr myAddr { };

int UDP_OpenIPV4Socket(int port);

//=============================================================================

#ifdef USE_IFADDRS
int UDP_Init (void)
{
	char	buff[MAXHOSTNAMELEN];
	struct qsockaddr addr;

	if (COM_CheckParm ("-noudp"))
		return -1;

	// determine my name & address
	ifaddrs *allInterfaces;
	auto result = getifaddrs(&allInterfaces);
	if (result < 0)
	{
		return result;
	}
	if (hostname.string == "UNNAMED")
	{
		auto found = false;
		for (auto interface = allInterfaces; interface != nullptr; interface = interface->ifa_next)
		{
			auto flags = interface->ifa_flags;
			auto addr = interface->ifa_addr;
			if (((flags & (IFF_UP|IFF_RUNNING|IFF_LOOPBACK)) == (IFF_UP|IFF_RUNNING)) && addr->sa_family == AF_INET)
			{
				getnameinfo(addr, sizeof(sockaddr), buff, MAXHOSTNAMELEN, nullptr, 0, NI_NUMERICHOST);
				myAddr.s6_addr[10] = 255;
				myAddr.s6_addr[11] = 255;
				myAddr.s6_addr[12] = addr->sa_data[2];
				myAddr.s6_addr[13] = addr->sa_data[3];
				myAddr.s6_addr[14] = addr->sa_data[4];
				myAddr.s6_addr[15] = addr->sa_data[5];
				strncpy(my_tcpip_address, buff, NET_NAMELEN);
				Cvar_Set ("hostname", buff);
				found = true;
				break;
			}
		}
		freeifaddrs(allInterfaces);
		if (!found)
		{
			return -1;
		}
	}
	else
	{
		auto found = false;
		for (auto interface = allInterfaces; interface != nullptr; interface = interface->ifa_next)
		{
			auto flags = interface->ifa_flags;
			auto addr = interface->ifa_addr;
			if (((flags & (IFF_UP|IFF_RUNNING|IFF_LOOPBACK)) == (IFF_UP|IFF_RUNNING)) && addr->sa_family == AF_INET)
			{
				getnameinfo(addr, sizeof(sockaddr), buff, MAXHOSTNAMELEN, nullptr, 0, NI_NUMERICHOST);
				if (hostname.string == buff)
				{
					myAddr.s6_addr[10] = 255;
					myAddr.s6_addr[11] = 255;
					myAddr.s6_addr[12] = addr->sa_data[2];
					myAddr.s6_addr[13] = addr->sa_data[3];
					myAddr.s6_addr[14] = addr->sa_data[4];
					myAddr.s6_addr[15] = addr->sa_data[5];
					strncpy(my_tcpip_address, buff, NET_NAMELEN);
					found = true;
					break;
				}
			}
		}
		freeifaddrs(allInterfaces);
		if (!found)
		{
			return -1;
		}
	}

	if ((net_controlsocket = UDP_OpenIPV4Socket (0)) == -1)
		Sys_Error("UDP_Init: Unable to open control socket\n");

	broadcastaddr.data.resize(sizeof(sockaddr_in));
	auto address = (sockaddr_in*)broadcastaddr.data.data();
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_BROADCAST;
	address->sin_port = htons((unsigned short)net_hostport);

	Con_Printf("UDP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}
#else
int UDP_Init (void)
{
	char	buff[MAXHOSTNAMELEN];
	struct qsockaddr addr;
	
	if (COM_CheckParm ("-noudp"))
		return -1;

    // determine my name & address
    gethostname(buff, MAXHOSTNAMELEN);
    
    addrinfo hints { };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    
    addrinfo* results = nullptr;
    auto err = getaddrinfo(buff, "0", &hints, &results);
    if (err < 0)
    {
        return -1;
    }
    
    addrinfo* result;
    for (result = results; result != nullptr; result = result->ai_next)
    {
        if (result->ai_family == AF_INET6)
        {
            auto is_local = true;
            for (auto i = 0; i < 16; i++)
            {
                if (((sockaddr_in6*)result->ai_addr)->sin6_addr.s6_addr[i] != in6addr_loopback.s6_addr[i])
                {
                    is_local = false;
                    break;
                }
            }
            if (!is_local)
            {
                myAddr = ((sockaddr_in6*)result->ai_addr)->sin6_addr;
                inet_ntop(AF_INET6, &myAddr, my_tcpip_address, NET_NAMELEN);
                break;
            }
        }
    }
    
    if (result == nullptr)
    {
        for (result = results; result != nullptr; result = result->ai_next)
        {
            if (result->ai_family == AF_INET)
            {
                auto address = ((sockaddr_in*)result->ai_addr)->sin_addr;
                auto addressnumber = htonl(address.s_addr);
                myAddr.s6_addr[10] = 255;
                myAddr.s6_addr[11] = 255;
                myAddr.s6_addr[12] = addressnumber >> 24;
                myAddr.s6_addr[13] = (addressnumber >> 16) & 255;
                myAddr.s6_addr[14] = (addressnumber >> 8) & 255;
                myAddr.s6_addr[15] = addressnumber & 255;
                inet_ntop(AF_INET, &address, my_tcpip_address, NET_NAMELEN);
                break;
            }
        }
    }
    
    freeaddrinfo(results);

	// if the quake hostname isn't set, set it to the machine name
	if (Q_strcmp(hostname.string.c_str(), "UNNAMED") == 0)
	{
		Cvar_Set ("hostname", buff);
	}

	if ((net_controlsocket = UDP_OpenIPV4Socket (0)) == -1)
		Sys_Error("UDP_Init: Unable to open control socket\n");

    broadcastaddr.data.resize(sizeof(sockaddr_in));
    auto address = (sockaddr_in*)broadcastaddr.data.data();
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_BROADCAST;
    address->sin_port = htons((unsigned short)net_hostport);

	Con_Printf("UDP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}
#endif

//=============================================================================

void UDP_Shutdown (void)
{
	UDP_Listen (false);
	UDP_CloseSocket (net_controlsocket);
}

//=============================================================================

void UDP_Listen (qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = UDP_OpenSocket (net_hostport)) == -1)
			Sys_Error ("UDP_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	UDP_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int UDP_OpenSocket (int port)
{
    int newsocket;
    struct sockaddr_in6 address { };
    qboolean _true = true;
    int off = 0;

    if ((newsocket = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        return -1;
    
    if (setsockopt(newsocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off)) == -1)
        goto ErrorReturn;
    
    if (ioctl (newsocket, FIONBIO, (char *)&_true) == -1)
        goto ErrorReturn;
    
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons((unsigned short)port);
    if( bind (newsocket, (struct sockaddr*)&address, sizeof(address)) == -1)
        goto ErrorReturn;
    
    return newsocket;
    
ErrorReturn:
    close (newsocket);
    return -1;
}

//=============================================================================

int UDP_OpenIPV4Socket (int port)
{
	int newsocket;
	struct sockaddr_in address;
	qboolean _true = true;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	if (ioctl (newsocket, FIONBIO, (char *)&_true) == -1)
		goto ErrorReturn;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if( bind (newsocket, (struct sockaddr*)&address, sizeof(address)) == -1)
		goto ErrorReturn;

	return newsocket;

ErrorReturn:
	close (newsocket);
	return -1;
}

//=============================================================================

int UDP_CloseSocket (int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	return close (socket);
}


//=============================================================================

int UDP_CheckNewConnections (void)
{
	unsigned long	available;

	if (net_acceptsocket == -1)
		return -1;

	if (ioctl (net_acceptsocket, FIONREAD, &available) == -1)
		Sys_Error ("UDP: ioctlsocket (FIONREAD) failed\n");
	if (available)
		return net_acceptsocket;
	return -1;
}

//=============================================================================

int UDP_Read (int socket, std::vector<byte>& buf, struct qsockaddr *addr)
{
    u_long available = 0;
    auto err = ioctl(socket, FIONREAD, &available);
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
    socklen_t addrlen;
    if (socket == net_controlsocket)
    {
        addrlen = (socklen_t)sizeof(sockaddr_in);
    }
    else
    {
        addrlen = (socklen_t)sizeof(sockaddr_in6);
    }
    addr->data.resize(addrlen);
    auto read = recvfrom(socket, (char*)buf.data(), len, 0, (sockaddr*)addr->data.data(), &addrlen);
    if (read > 0 && read < len)
    {
        buf.resize(read);
    }
    return (int)read;
}

//=============================================================================

int UDP_MakeSocketBroadcastCapable (int socket)
{
	int				i = 1;

	// make this socket broadcast capable
	if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int UDP_Broadcast (int socket, byte *buf, int len)
{
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		ret = UDP_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return UDP_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int UDP_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret;

	ret = (int)sendto (socket, buf, (size_t)len, 0, (sockaddr*)addr->data.data(), (socklen_t)addr->data.size());
	if (ret == -1 && errno == EWOULDBLOCK)
		return 0;
	return ret;
}

//=============================================================================

int UDP_GetSocketAddr (int socket, struct qsockaddr *addr)
{
    return SOCK_GetSocketAddr (socket, net_controlsocket, myAddr, addr);
}

//=============================================================================

int UDP_GetAddrFromName(const char *name, struct qsockaddr *addr)
{
    return SOCK_GetAddrFromName(name, myAddr, addr);
}
