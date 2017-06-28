#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

#include "server/Request.hpp"
#include "http/HttpStatus.hpp"
#include "server/TcpConnection.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "error/RequestError.hpp"
#include "error/ConnectionError.hpp"
#include "error/TodoError.hpp"


Request::Request(Config const& config, TcpConnection& conn) :
    m_config(config),
    m_conn(conn)
{
    std::string request_line = parse_raw_line();
    parse_method(request_line);
    parse_route(request_line);
    parse_version(request_line);

    // the previous three parse_* calls should consume the entire line
    if (!request_line.empty())
    {
        throw RequestError(HttpStatus::BadRequest, "Malformed request-line\n");
    }
    
    parse_headers();
    parse_body();
}

void Request::parse_method(std::string& raw_line)
{
  
  m_method = raw_line.substr(0, 3);
  
  if (m_method.compare("GET") != 0)
  {
    m_method = raw_line.substr(0, 4);
    if (m_method.compare("POST") != 0)
    {
      throw RequestError(HttpStatus::MethodNotAllowed, "Method not allowed\n");
    }
  }

  raw_line = raw_line.substr(m_method.length() + 1);
  
  //throw TodoError("2", "You have to implement parsing methods");
}

void Request::parse_route(std::string& raw_line)
{
  size_t pos = raw_line.find("/");
  size_t end = raw_line.find(" ");
  bool flag  = false;
  if (raw_line.find("?") != std::string::npos) flag = true;
  
  if (pos == std::string::npos)
  {
    throw RequestError(HttpStatus::BadRequest, "Malformed request-line\n");
  }

  if (flag == true) end = raw_line.find("?");

  m_path   = raw_line.substr((int) pos, (int) end - (int) pos);
  raw_line = raw_line.substr(m_path.length() + 1);
  if (flag == true)
  {
    size_t end        = raw_line.find(" ");
    std::string query = raw_line.substr(0, end - 1);
    parse_querystring(raw_line, m_query);
    raw_line = raw_line.substr(end + 1);
  }
  //throw TodoError("2", "You have to implement parsing routes");
}

void Request::parse_querystring(std::string query, std::unordered_map<std::string, std::string>& parsed)
{
  size_t amp = query.find("&");
  while (amp != std::string::npos)
  {
    std::string query_item = query.substr(0, amp - 1);
    std::string word1      = query_item.substr(0, query_item.find("=") - 1);
    std::string word2      = query_item.substr(query_item.find("=") + 1, amp - 1);
    parsed[word1]          = word2;
    
    query = query.substr(amp + 1);
    amp   = query.find("&");
  }

  std::string word1 = query.substr(0, query.find("=") - 1);
  std::string word2 = query.substr(query.find("=") + 1);
  parsed[word1]     = word2;
  
  //throw TodoError("6", "You have to implement parsing querystrings");
}

void Request::parse_version(std::string& raw_line)
{
  if      (raw_line.find("HTTP/1.0") != std::string::npos) m_version = "HTTP/1.0";
  else if (raw_line.find("HTTP/1.1") != std::string::npos) m_version = "HTTP/1.1";
  else    throw RequestError(HttpStatus::HttpVersionNotSupported, "Version not supported\n");
  raw_line = raw_line.substr(m_version.length() + 2);

  //throw TodoError("2", "You have to implement parsing HTTP version");
}

void Request::parse_headers()
{
  std::string raw_line = parse_raw_line();
  while (raw_line.length() != 2)
  {
    size_t pos        = raw_line.find(":");
    std::string key   = raw_line.substr(0, pos - 1);
    std::string value = raw_line.substr(pos + 1, raw_line.length() - 2 - pos - 1);
    m_headers[key]    = value;
    raw_line = parse_raw_line();
  }
  //throw TodoError("2", "You have to implement parsing headers");
}

void Request::parse_body()
{
  if (m_method == "GET") return;

  if (m_headers["Content-Type"] != "application/x-www-form-urlencoded")
  {
    throw RequestError(HttpStatus::UnsupportedMediaType, "Unsupported media type\n");
  }

  unsigned char c;
  int count  = 0;
  int length = stoi(m_headers["Content-Length"], nullptr, 10);
  char body[length + 1];
  
  if (length > 4096)
  {
    throw RequestError(HttpStatus::Forbidden, "Forbidden\n");
  }

  while (count < length)
  {
    m_conn.getc(&c);
    body[count++] = c;
  }
  body[count] = '\0';

  std::string body_data = body;
  parse_querystring(body_data, m_body_data);  
  //throw TodoError("6", "You have to implement parsing request bodies");
}

std::string Request::parse_raw_line()
{
  std::string request_line;
  unsigned char c;
  bool getc = true;
  int bytes = 0;

  while (bytes < m_max_buf && (getc = m_conn.getc(&c)) != false)
  {
    if (c == EOF) return request_line;

    if (c == '\r')
    {
      request_line += c;
      getc = m_conn.getc(&c);
      if (c == '\n')
      {
	request_line += c;
	return request_line;
      }
    }
    request_line += c;
    bytes++;
  }
  
  if (getc == false)
  {
    throw ConnectionError("Connection error\n");
  }
  
  return request_line;
  //throw TodoError("2", "You need to implement line fetching");
}

void Request::print() const noexcept
{
    std::cout << m_method << ' ' << m_path << ' ' << m_version << std::endl;
#ifdef DEBUG    
    for (auto const& el : m_headers)
    {
        std::cout << el.first << ": " << el.second << std::endl;
    }

    for (auto const& el : m_query)
    {
        std::cerr << el.first << ": " << el.second << std::endl;
    }

    for (auto const& el : m_body_data)
    {
        std::cerr << el.first << ": " << el.second << std::endl;
    }
#endif	
}

bool Request::try_header(std::string const& key, std::string& value) const noexcept
{
    if (m_headers.find(key) == m_headers.end())
    {
        return false;
    }
    else
    {
        value = m_headers.at(key);
        return true;
    }
}

std::string const& Request::get_path() const noexcept
{
    return m_path;
}

std::string const& Request::get_method() const noexcept
{
    return m_method;
}

std::string const& Request::get_version() const noexcept
{
    return m_version;
}

std::unordered_map<std::string, std::string> const& Request::get_headers() const noexcept
{
    return m_headers;
}

std::unordered_map<std::string, std::string> const& Request::get_query() const noexcept
{
    return m_query;
}

std::unordered_map<std::string, std::string> const& Request::get_body() const noexcept
{
    return m_body_data;
}
