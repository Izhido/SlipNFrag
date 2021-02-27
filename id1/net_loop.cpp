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
// net_loop.c

#include "quakedef.h"
#include "net_loop.h"

qboolean	localconnectpending = false;
qsocket_t	*loop_client = NULL;
qsocket_t	*loop_server = NULL;

int Loop_Init (void)
{
	if (cls.state == ca_dedicated)
		return -1;
	return 0;
}


void Loop_Shutdown (void)
{
}


void Loop_Listen (qboolean state)
{
}


void Loop_SearchForHosts (qboolean xmit)
{
	if (!sv.active)
		return;

	hostCacheCount = 1;
	if (Q_strcmp(hostname.string.c_str(), "UNNAMED") == 0)
		Q_strcpy(hostcache[0].name, "local");
	else
		Q_strcpy(hostcache[0].name, hostname.string.c_str());
	Q_strcpy(hostcache[0].map, pr_strings + sv.name);
	hostcache[0].users = net_activeconnections;
	hostcache[0].maxusers = svs.maxclients;
	hostcache[0].driver = net_driverlevel;
	Q_strcpy(hostcache[0].cname, "local");
}


qsocket_t *Loop_Connect (const char *host)
{
	if (Q_strcmp(host,"local") != 0)
		return NULL;
	
	localconnectpending = true;

	if (!loop_client)
	{
		if ((loop_client = NET_NewQSocket ()) == NULL)
		{
			Con_Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		loop_client->address = "localhost";
	}
    loop_client->receiveMessage.clear();
	loop_client->sendMessage.clear();
	loop_client->canSend = true;

	if (!loop_server)
	{
		if ((loop_server = NET_NewQSocket ()) == NULL)
		{
			Con_Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		loop_server->address = "LOCAL";
	}
	loop_server->receiveMessage.clear();
	loop_server->sendMessage.clear();
	loop_server->canSend = true;

	loop_client->driverdata = (void *)loop_server;
	loop_server->driverdata = (void *)loop_client;
	
	return loop_client;	
}


qsocket_t *Loop_CheckNewConnections (void)
{
	if (!localconnectpending)
		return NULL;

	localconnectpending = false;
	loop_server->sendMessage.clear();
	loop_server->receiveMessage.clear();
	loop_server->canSend = true;
	loop_client->sendMessage.clear();
	loop_client->receiveMessage.clear();
	loop_client->canSend = true;
	return loop_server;
}


static int IntAlign(int value)
{
	return (value + (sizeof(int) - 1)) & (~(sizeof(int) - 1));
}


int Loop_GetMessage (qsocket_t *sock)
{
	int		ret;
	int		length;

	if (sock->receiveMessage.size() == 0)
		return 0;

	ret = sock->receiveMessage[0];
	length = sock->receiveMessage[1] + (sock->receiveMessage[2] << 8) + (sock->receiveMessage[3] << 16) + (sock->receiveMessage[4] << 24);
	// alignment byte skipped here
	SZ_Clear (&net_message);
	SZ_Write (&net_message, &sock->receiveMessage[8], length);

	length = IntAlign(length + 8);
    sock->receiveMessage.erase(sock->receiveMessage.begin(), sock->receiveMessage.begin() + length);

	if (sock->driverdata && ret == 1)
		((qsocket_t *)sock->driverdata)->canSend = true;

	return ret;
}


int Loop_SendMessage (qsocket_t *sock, sizebuf_t *data)
{
	byte *buffer;
	int bufferLength;

	if (!sock->driverdata)
		return -1;

	bufferLength = ((qsocket_t *)sock->driverdata)->receiveMessage.size();

	((qsocket_t *)sock->driverdata)->receiveMessage.resize(IntAlign(bufferLength + data->data.size() + 8));

	buffer = ((qsocket_t *)sock->driverdata)->receiveMessage.data() + bufferLength;

	// message type
	*buffer++ = 1;

	// length
	*buffer++ = data->data.size() & 0xff;
	*buffer++ = (data->data.size() >> 8) & 0xff;
    *buffer++ = (data->data.size() >> 16) & 0xff;
    *buffer++ = (data->data.size() >> 24) & 0xff;

	// align
	buffer += 3;

	// message
	memcpy(buffer, data->data.data(), data->data.size());

	sock->canSend = false;
	return 1;
}


int Loop_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data)
{
	byte *buffer;
    int bufferLength;

	if (!sock->driverdata)
		return -1;

	bufferLength = ((qsocket_t *)sock->driverdata)->receiveMessage.size();

    ((qsocket_t *)sock->driverdata)->receiveMessage.resize(IntAlign(bufferLength + data->data.size() + 8));

    buffer = ((qsocket_t *)sock->driverdata)->receiveMessage.data() + bufferLength;

	// message type
	*buffer++ = 2;

	// length
    *buffer++ = data->data.size() & 0xff;
    *buffer++ = (data->data.size() >> 8) & 0xff;
    *buffer++ = (data->data.size() >> 16) & 0xff;
    *buffer++ = (data->data.size() >> 24) & 0xff;

	// align
    buffer += 3;

	// message
	Q_memcpy(buffer, data->data.data(), data->data.size());
	return 1;
}


qboolean Loop_CanSendMessage (qsocket_t *sock)
{
	if (!sock->driverdata)
		return false;
	return sock->canSend;
}


qboolean Loop_CanSendUnreliableMessage (qsocket_t *sock)
{
	return true;
}

int Loop_MaxMessageSize (qsocket_t *sock)
{
    return 0;
}

int Loop_MaxUnreliableMessageSize (qsocket_t *sock)
{
    return 0;
}

void Loop_Close (qsocket_t *sock)
{
	if (sock->driverdata)
		((qsocket_t *)sock->driverdata)->driverdata = NULL;
	sock->receiveMessage.clear();
	sock->sendMessage.clear();
	sock->canSend = true;
	if (sock == loop_client)
		loop_client = NULL;
	else
		loop_server = NULL;
}
