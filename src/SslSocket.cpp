/*
 * Copyright (c) 2014 Matt Fichman
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

#include "coro/Common.hpp"
#include "coro/SslSocket.hpp"

namespace coro {

SslError::SslError(unsigned long error) : what_(ERR_error_string(error, 0)) {
}

SslSocket::SslSocket(int type, int protocol) : Socket(type, protocol), eof_(false) {
    SSL_load_error_strings();
    SSL_library_init();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    context_ = SSL_CTX_new(SSLv23_client_method());
    assert(context_);

    conn_ = SSL_new(context_);
    assert(conn_);

    in_ = BIO_new(BIO_s_mem());
    out_ = BIO_new(BIO_s_mem());
    SSL_set_bio(conn_, in_, out_);
    SSL_set_connect_state(conn_);

}

SslSocket::~SslSocket() {
    SSL_free(conn_);
    SSL_CTX_free(context_);
}

void SslSocket::writeAllRaw(char const* buf, size_t len, int flags) {
// Write all data in 'buf'.  Block until all data is written, or the connection
// is closed.  Throws a SocketCloseException if the connection was closed by
// the other end.
    while(len > 0) {
        ssize_t bytes = Socket::write(buf, len, flags);
        if (bytes == 0) {
            throw SocketCloseException(); 
        } else if (bytes > 0) {
            buf += bytes;
            len -= bytes;
        } else {
            assert(!"unexpected negative byte value");
        }
    }
}

void SslSocket::writeFromBio(int flags) {
    char buf[4096];
    ssize_t pending = BIO_ctrl_pending(out_);
    if (!pending) { return; }
    pending++; pending--;
    ssize_t bytes = BIO_read(out_, buf, sizeof(buf));
    if (bytes > 0) {
        writeAllRaw(buf, bytes, flags);
    } else if (bytes == -1 || bytes == 0) {
        // No data
    } else {
        assert(!"bio error");
    }
}

void SslSocket::readToBio(int flags) {
    char buf[4096];
    ssize_t bytes = Socket::read(buf, sizeof(buf), flags);
    if (bytes > 0) {
        ssize_t written = BIO_write(in_, buf, bytes);
        assert(bytes==written);
    } else if (bytes == 0) {
        // No data
        eof_ = true;
    } else {
        assert(!"socket read error");
    }
}

ssize_t SslSocket::write(char const* buf, size_t len, int flags) {
retry:
    ssize_t bytes = SSL_write(conn_, buf, len);
	writeFromBio(0); // Write data if available
    if (bytes < 0) {
        int err = SSL_get_error(conn_, bytes);
        if (SSL_ERROR_WANT_WRITE == err) { // 
            writeFromBio(flags);
            goto retry;
        } else if (SSL_ERROR_WANT_READ == err) { // BIO has data available
            readToBio(0); 
            goto retry; 
        } else {
            throw SslError(err);
        }
    }
    return bytes; 
}

ssize_t SslSocket::read(char* buf, size_t len, int flags) {
retry:
    readToBio(flags); 
    ssize_t bytes = SSL_read(conn_, buf, len);
    if (bytes < 0) {
        int err = SSL_get_error(conn_, bytes);
        if (SSL_ERROR_WANT_WRITE == err) {
            writeFromBio(0);
            goto retry;
        } else if (SSL_ERROR_WANT_READ == err) {
            if (eof_) {
                return 0;
            }
            readToBio(flags); 
            goto retry; 
        } else {
            throw SslError(err);
        }
    }
    return bytes;
}

}
