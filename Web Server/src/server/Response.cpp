#include <string>
#include <cstring>
#include <map>

#include "server/Response.hpp"
#include "server/TcpConnection.hpp"
#include "http/HttpStatus.hpp"
#include "error/ResponseError.hpp"
#include "error/TodoError.hpp"
#include "Config.hpp"

Response::Response(Config const& config, TcpConnection& conn) :
    m_config(config),
    m_conn(conn),
    m_headers_sent(false)
{
    // We want every response to have this header
    // It tells browsers that we want separate connections per request
    m_headers["Connection"] = "close";
}

void Response::send(void const* buf, size_t bufsize, bool raw)
{
  std::string status = "HTTP/1.0 " + m_status_text + "\r\n";
  m_conn.puts(status.c_str());

  if(raw == false) send_headers();
  std::string empty_line = "\r\n";
  m_conn.puts(empty_line.c_str());
  m_conn.putbuf(buf, bufsize);
  m_conn.shutdown();
  //throw TodoError("2", "You need to implement sending responses");
}

void Response::send_headers()
{
  std::string header;
  for (auto const& element : m_headers)
  {
    std::string key = element.first;
    std::string val = element.second;
    std::string header = key;

    header += ": ";
    header += val;
    header += "\r\n";

    m_conn.puts(header.c_str());
  }
  //throw TodoError("2", "You need to implement sending headers");
}

void Response::set_header(std::string const& key, std::string const& value)
{
  m_headers[key] = value;
  //throw TodoError("2", "You need to implement controllers setting headers");
}

void Response::set_status(HttpStatus const& status)
{
    m_status_text = status.to_string();
}
