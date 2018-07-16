#include "Platform.h"
#include "ErrorHandler.h"

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#include <winsock2.h>

ReturnCode ReturnCode_SocketInProgess = WSAEINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
#elif CC_BUILD_X11
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

ReturnCode ReturnCode_SocketInProgess = EINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;
#else
#error "You're not using BSD sockets, define the interface in Socket.c"
#endif

void Platform_SocketCreate(void** socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		ErrorHandler_FailWithCode(WSAGetLastError(), "Failed to create socket");
	}
}

ReturnCode Platform_SocketAvailable(void* socket, UInt32* available) {
	return ioctlsocket(socket, FIONREAD, available);
}

ReturnCode Platform_SocketSetBlocking(void* socket, bool blocking) {
	Int32 blocking_raw = blocking ? 0 : -1;
	return ioctlsocket(socket, FIONBIO, &blocking_raw);
}

ReturnCode Platform_SocketGetError(void* socket, ReturnCode* result) {
	Int32 resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Platform_SocketConnect(void* socket, STRING_PURE String* ip, Int32 port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip->buffer);
	addr.sin_port = htons((UInt16)port);

	ReturnCode result = connect(socket, (struct sockaddr*)(&addr), sizeof(addr));
	return result == -1 ? WSAGetLastError() : 0;
}

ReturnCode Platform_SocketRead(void* socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 recvCount = recv(socket, buffer, count, 0);
	if (recvCount == -1) { *modified = recvCount; return 0; }
	*modified = 0; return WSAGetLastError();
}

ReturnCode Platform_SocketWrite(void* socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 sentCount = send(socket, buffer, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return WSAGetLastError();
}

ReturnCode Platform_SocketClose(void* socket) {
	ReturnCode result = 0;
#if CC_BUILD_WIN
	ReturnCode result1 = shutdown(socket, SD_BOTH);
#else
	ReturnCode result1 = shutdown(socket, SHUT_RDWR);
#endif
	if (result1 == -1) result = WSAGetLastError();

	ReturnCode result2 = closesocket(socket);
	if (result2 == -1) result = WSAGetLastError();
	return result;
}

ReturnCode Platform_SocketSelect(void* socket, Int32 selectMode, bool* success) {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(socket, &set);

	struct timeval time = { 0 };
	Int32 selectCount;

	if (selectMode == SOCKET_SELECT_READ) {
		selectCount = select(1, &set, NULL, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_WRITE) {
		selectCount = select(1, NULL, &set, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_ERROR) {
		selectCount = select(1, NULL, NULL, &set, &time);
	} else {
		selectCount = -1;
	}

	if (selectCount == -1) { *success = false; return WSAGetLastError(); }
#if CC_BUILD_WIN
	*success = set.fd_count != 0; return 0;
#else
	*success = FD_ISSET(socket, &set); return 0;
#endif
}