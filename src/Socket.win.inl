/*
 * Copyright (c) 2013 Matt Fichman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, APEXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

namespace coro {

void Socket::connect(SocketAddr const& addr) {
// Windows asynchronous connect

    // Initialize a bunch of Windows-specific crap needed to load a pointer to
    // the ConnectEx function...

    // Get a pointer to the ConnectEx() function.  Sigh.  Windows.  This 
    // function never blocks, however, so we don't have to worry about I/O 
    // completion ports.
    DWORD code = SIO_GET_EXTENSION_FUNCTION_POINTER;
    GUID guid = WSAID_CONNECTEX;
    LPFN_CONNECTEX ConnectEx = 0;
    DWORD bytes = 0; 
    DWORD len = sizeof(ConnectEx);
    WSAIoctl(sd_, code, &guid, sizeof(guid), &ConnectEx, len, &bytes, 0, 0);

    // To use ConnectEx, bind() must be called first to assign a port number to
    // the socket.
    struct sockaddr_in sin{0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = 0;
    if (::bind(sd_, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
        throw SystemError();
    }

    // Initialize the OVERLAPPED structure that contains the user I/O data used
    // to resume the coroutine when ConnectEx completes.
    Overlapped op{0};
    OVERLAPPED* evt = &op.overlapped;
    op.coroutine = current().get();

    // Now call ConnectEx to begin connecting the socket.  The call will return
    // immediately, allowing this function to yield to the I/O manager.
    sin = addr.sockaddr();
    if (!ConnectEx(sd_, (struct sockaddr*)&sin, sizeof(sin), 0, 0, 0, evt)) { 
        if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        } 
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        throw SystemError(op.error);
    }

    // The following setsockopt() call is needed when calling ConectEx.  From
    // the MSDN documentation: 
    //
    // When the ConnectEx function returns TRUE, the socket s is in the default
    // state for a connected socket. The socket s does not enable previously
    // set properties or options until SO_UPDATE_CONNECT_CONTEXT is set on the
    // socket. Use the setsockopt function to set the SO_UPDATE_CONNECT_CONTEXT
    // option.
    if (::setsockopt(sd_, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0)) {
         throw SystemError();
    }
}

SocketHandle Socket::acceptRaw() {
    // Waits for a client to connect, then returns a pointer to the established
    // connection.  The code below is a bit tricky, because Windows expects the
    // call to accept() to happen before the I/O event can be triggered.  For
    // Unix systems, the wait happens first, and then accept() is used to
    // receive the incoming socket afterwards.

    // Create a new socket for AcceptEx to use when a peer connects.
    SOCKET ls = sd_;
    SOCKET sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) {
        throw SystemError();
    }

    // Get a pointer to the AcceptEx() function.
    DWORD code = SIO_GET_EXTENSION_FUNCTION_POINTER;
    GUID guid = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX AcceptEx = 0;
    DWORD bytes = 0;
    DWORD len = sizeof(AcceptEx);
    WSAIoctl(sd, code, &guid, sizeof(guid), &AcceptEx, len, &bytes, 0, 0);
    
    // Initialize the OVERLAPPED structure that contains the user I/O data used
    // to resume the coroutine when AcceptEx completes. 
    Overlapped op{0};
    op.coroutine = current().get();
    OVERLAPPED* evt = &op.overlapped;

    // Now call ConnectEx to begin accepting peer connections.  The call will
    // return immediately, allowing this function to yield to the I/O manager.
    char buffer[(sizeof(struct sockaddr_in)+16)*2]; // Windows BS, apparently
    DWORD socklen = sizeof(struct sockaddr_in)+16; // Ditto
    DWORD read = 0;
    if (!AcceptEx(ls, sd, buffer, 0, socklen, socklen, &read, evt)) {
        if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        } 
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        throw SystemError(op.error);
    }
    
    // The following setsockopt() call is needed when calling AcceptEx.  From
    // the MSDN documentation: 
    //
    // When the AcceptEx function returns, the socket sAcceptSocket is in the
    // default state for a connected socket. The socket sAcceptSocket does not
    // inherit the properties of the socket associated with sListenSocket
    // parameter until SO_UPDATE_ACCEPT_CONTEXT is set on the socket. Use the
    // setsockopt function to set the SO_UPDATE_ACCEPT_CONTEXT option,
    // specifying sAcceptSocket as the socket handle and sListenSocket as the
    // option value.
    char const* opt = (char const*)&ls;
    int const optlen = int(sizeof(ls));
    if (::setsockopt(sd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, opt, optlen)) {
        throw SystemError();
    }

    return sd;
}

bool isSocketCloseError(DWORD error) {
// Returns true if the given error code should be converted to a socket close
// exception.
    switch (error) {
    case ERROR_NETNAME_DELETED:
    case ERROR_CONNECTION_ABORTED:
    case WSAENETRESET:
    case WSAECONNABORTED:
    case WSAECONNRESET:
        return true;
    default:
        return false;
    }
}

ssize_t Socket::read(char* buf, size_t len, int flags) {
// Read from the socket asynchronously.  Returns the # of bytes read.
    WSABUF wsabuf = { ULONG(len), buf };
    Overlapped op{0};
    op.coroutine = current().get();
    OVERLAPPED* evt = &op.overlapped;
    DWORD flg = flags;

    if (sd_ == -1) {
        throw SocketCloseException(); // Socket closed locally
    }

    if(WSARecv(sd_, &wsabuf, 1, NULL, &flg, evt, NULL)) {
        if (isSocketCloseError(GetLastError())) {
            throw SocketCloseException(); // Socket closed remotely
        } else if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        } 
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        if (isSocketCloseError(op.error)) {
            throw SocketCloseException(); // Socket closed remotely during read
        } else {
			throw SystemError(op.error);
        }
    }
    assert(op.bytes >= 0);
    return op.bytes;
}

ssize_t Socket::write(char const* buf, size_t len, int flags) {
// Write to the socket asynchronously.  Returns the # of bytes written.
    WSABUF wsabuf = { ULONG(len), LPSTR(buf) };
    Overlapped op{0};
    op.coroutine = current().get();
    OVERLAPPED* evt = &op.overlapped;

    if (sd_ == -1) {
        throw SocketCloseException(); // Socket closed locally
    }

    if(WSASend(sd_, &wsabuf, 1, NULL, flags, evt, NULL)) {
        if (isSocketCloseError(GetLastError())) {
            throw SocketCloseException(); // Socket closed remotely 
        } else if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        } 
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        if (isSocketCloseError(op.error)) {
            throw SocketCloseException(); // Socket closed remotely during write
        } else {
            throw SystemError(op.error);
        }
    }
    assert(op.bytes >= 0);
    return op.bytes;
}

}
