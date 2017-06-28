#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

#include "Utils.hpp"
#include "Config.hpp"
#include "server/TcpConnection.hpp"
#include "error/ConnectionError.hpp"
#include "error/SocketError.hpp"
#include "error/TodoError.hpp"

TcpConnection::TcpConnection(Config const& config, int master_fd) :
    m_config(config),
    m_master(master_fd),
    //m_conn(accept(m_master, NULL, NULL)),
    m_shutdown(false)
{
  m_conn = accept(m_master, NULL, NULL);
  if (m_conn == -1)
  {
    throw ConnectionError("accept");
  }
  //throw TodoError("2", "You need to implement construction of TcpConnections");
}

TcpConnection::~TcpConnection() noexcept
{
    d_printf("Closing connection on %d", m_conn);

    if (close(m_conn) == -1) d_errorf("Could not close connection %d", m_conn);
}

void TcpConnection::shutdown()
{
    d_printf("Shutting down connection on %d", m_conn);
    
    if (::shutdown(m_conn, SHUT_RDWR) == -1) d_errorf("Could not shut down connection %d", m_conn);

    m_shutdown = true;
}

bool TcpConnection::getc(unsigned char* c)
{
  return read(m_conn, c, 1) == 1;

  //throw TodoError("2", "You have to implement reading from connections");
}

void TcpConnection::putc(unsigned char c)
{
  write(m_conn, &c, 1);
  //throw TodoError("2", "You need to implement writing characters to connections");
}

void TcpConnection::puts(std::string const& str)
{
  write(m_conn, str.c_str(), str.length());
  //throw TodoError("2", "You need to implement writing strings to connections");
}

void TcpConnection::putbuf(void const* buf, size_t bufsize)
{
  write(m_conn, buf, bufsize);
  //throw TodoError("2", "You need to implement writing buffers to connections");
}
