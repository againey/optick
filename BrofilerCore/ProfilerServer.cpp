#include "Common.h"
#include "ProfilerServer.h"

#include "Socket.h"
#include <winbase.h>
#include "Message.h"

#pragma comment( lib, "ws2_32.lib" )

namespace Brofiler
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const short DEFAULT_PORT = 31313;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server::Server(short port) : socket(new Socket()), acceptThread(0)
{
	socket->Bind(port, 8);
	socket->Listen();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::Update()
{
	CRITICAL_SECTION(lock);

	InitConnection();

	int length = -1;
	while ( (length = socket->Receive( buffer, BIFFER_SIZE ) ) > 0 )
	{
		networkStream.Append(buffer, length);
	}

	while (IMessage *message = IMessage::Create(networkStream))
	{
		message->Apply();
		delete message;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::Send(DataResponse::Type type, OutputDataStream& stream)
{
	CRITICAL_SECTION(lock);

	std::string data = stream.GetData();

	DataResponse response(type, (uint32)data.size());
	socket->Send((char*)&response, sizeof(response));
	socket->Send(data.c_str(), data.size());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Server::InitConnection()
{
	if (!acceptThread)
	{
		acceptThread = CreateThread(NULL, 0, &Server::AsyncAccept, this, 0, NULL);
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
	if (acceptThread)
	{
		TerminateThread(acceptThread, 0);
		WaitForSingleObject(acceptThread, INFINITE);
		CloseHandle(acceptThread);
		acceptThread = 0;
	}

	if (socket)
	{
		delete socket;
		socket = nullptr;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server & Server::Get()
{
	static Server instance(DEFAULT_PORT);
	return instance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Server::Accept()
{
	socket->Accept();
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI Server::AsyncAccept( LPVOID lpParam )
{
	Server* server = (Server*)lpParam;

	while (server->Accept())
	{
		Sleep(1000);
	}

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}