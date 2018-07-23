#include "Platform.h"
#include "ErrorHandler.h"

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#include <winsock2.h>

ReturnCode ReturnCode_SocketInProgess = WSAEINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
#define Socket__Error() WSAGetLastError()
#elif CC_BUILD_NIX
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

ReturnCode ReturnCode_SocketInProgess = EINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;
#define Socket__Error() errno
#else
#error "You're not using BSD sockets, define the interface in Socket.c"
#endif

void Platform_SocketCreate(SocketPtr* socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		ErrorHandler_FailWithCode(Socket__Error(), "Failed to create socket");
	}
}

ReturnCode Platform_ioctl(SocketPtr socket, UInt32 cmd, Int32* data) {
#if CC_BUILD_WIN
	return ioctlsocket(socket, cmd, data);
#else
	return ioctl(socket, cmd, data);
#endif
}

ReturnCode Platform_SocketAvailable(SocketPtr socket, UInt32* available) {
	return Platform_ioctl(socket, FIONREAD, available);
}
ReturnCode Platform_SocketSetBlocking(SocketPtr socket, bool blocking) {
	Int32 blocking_raw = blocking ? 0 : -1;
	return Platform_ioctl(socket, FIONBIO, &blocking_raw);
}

ReturnCode Platform_SocketGetError(SocketPtr socket, ReturnCode* result) {
	Int32 resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Platform_SocketConnect(SocketPtr socket, STRING_PURE String* ip, Int32 port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip->buffer);
	addr.sin_port = htons((UInt16)port);

	ReturnCode result = connect(socket, (struct sockaddr*)(&addr), sizeof(addr));
	return result == -1 ? Socket__Error() : 0;
}

ReturnCode Platform_SocketRead(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 recvCount = recv(socket, buffer, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Platform_SocketWrite(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 sentCount = send(socket, buffer, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Platform_SocketClose(SocketPtr socket) {
	ReturnCode result = 0;
#if CC_BUILD_WIN
	ReturnCode result1 = shutdown(socket, SD_BOTH);
#else
	ReturnCode result1 = shutdown(socket, SHUT_RDWR);
#endif
	if (result1 == -1) result = Socket__Error();

#if CC_BUILD_WIN
	ReturnCode result2 = closesocket(socket);
#else
	ReturnCode result2 = close(socket);
#endif
	if (result2 == -1) result = Socket__Error();
	return result;
}

ReturnCode Platform_SocketSelect(SocketPtr socket, Int32 selectMode, bool* success) {
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

	if (selectCount == -1) { *success = false; return Socket__Error(); }
#if CC_BUILD_WIN
	*success = set.fd_count != 0; return 0;
#else
	*success = FD_ISSET(socket, &set); return 0;
#endif
}