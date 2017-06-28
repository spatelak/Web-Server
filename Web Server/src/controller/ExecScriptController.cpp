#include <string>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <climits>
#include <cstdlib>
#include <vector>

#include "Config.hpp"
#include "Utils.hpp"
#include "controller/Controller.hpp"
#include "controller/ExecScriptController.hpp"
#include "server/Request.hpp"
#include "server/Response.hpp"
#include "http/HttpStatus.hpp"
#include "error/ControllerError.hpp"
#include "error/TodoError.hpp"

ExecScriptController::ExecScriptController(Config const& config, std::string const& ignore) :
    Controller(config),
    m_ignore(ignore)
{

}

void ExecScriptController::run(Request const& req, Response& res) const
{
  std::string resolved_path;
  std::string path = req.get_path().substr(m_ignore.length());
  if (Controller::resolve_requested_path(path, m_config.exec_dir, resolved_path) == false)
    {
      Controller::send_error_response(res, HttpStatus::NotFound, path + " could not be found\n");
    }

  // open the file to get the size and contents of the file and close it. Send the response
  std::fstream fs;
  fs.open(resolved_path, std::fstream::in | std::fstream::binary);

  if (fs.is_open())
  {
      int size      = get_content_length(fs);
      char *content = (char *) malloc(size + 1);
      fs.clear();
      fs.seekp(fs.beg);
      fs.read(content, size);
      content[size] = '\0';
      fs.close();
      //set_environment(req);
      res.set_status(HttpStatus::Ok);
      res.set_header("Content-Type", get_content_type(resolved_path, req));
      res.set_header("Content-Length", std::to_string(size));
      res.send(content, size);

      free(content);
  }
  else
  {
      Controller::send_error_response(res, HttpStatus::NotFound, path + " could not be found\n");
  }
  //throw TodoError("4", "You need to implement SendFileController");
}

int ExecScriptController::get_content_length(std::fstream& fs) const
{
  int length = 0;
  while (fs.get() != EOF)
  {
      length++;
  }

  return length;
  //throw TodoError("4", "You need to implement getting the length of a file");
}
  //throw TodoError("6", "You need to implement ExecScriptController");

bool ExecScriptController::set_environment(Request const& req) const noexcept
{
  std::string http = "HTTP:";
  if (setenv((http + "METHOD=").c_str(), req.get_method().c_str(), 1) == -1) return false;
  if (setenv((http + "PATH=").c_str(), req.get_path().c_str(), 1) == -1)     return false;

  if (set_var(req.get_headers(), http + "HEADER:") == false) return false;
  if (set_var(req.get_query(), http + "QUERY:") == false)    return false;
  if (set_var(req.get_body(), http + "BODY:") == false)      return false;

  return true;
  //throw TodoError("6", "You need to implement setting environment variables for the child process");
}

bool ExecScriptController::set_var(std::unordered_map<std::string, std::string>  mapping, std::string key) const
{
  for (auto const& element : mapping)
  {
    if (setenv((key + element.first + "=").c_str(), element.second.c_str(), 1) == -1) return false;
  }

  return true;
}
std::string ExecScriptController::get_content_type(std::string const& filename, Request const& req) const
{
  int child_stdout[2];
  if (pipe(child_stdout) == -1)
    {
      throw ControllerError("Could not create pipe to communicate with `xdg-mime`");
    }

  d_printf("Getting content type. child_stdout = {%d, %d}", child_stdout[0], child_stdout[1]);

  pid_t child = fork();
  if (child == -1)
    {
      throw ControllerError("Could not create child process to find MIME type");
    }
  if (child == 0)
    {
      if (dup2(child_stdout[1], STDOUT_FILENO) == -1)
	{
	  exit(1);
	}
      set_environment(req);
      execlp("xdg-mime", "xdg-mime", "query", "filetype", filename.c_str(), 0);
      exit(1);
    }

  if (close(child_stdout[1]) == -1)
    {
      throw ControllerError("Could not close pipe input");
    }

  int status;
  if (waitpid(child, &status, 0) == -1)
    {
      throw ControllerError("Error while waiting for finding MIME type to finish");
    }

  if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) || WIFSIGNALED(status))
    {
      throw ControllerError("`xdg-mime` exited with errors");
    }

  //size_t const max_content_length = 50;
  //char buf[max_content_length];
  //memset(buf, 0, max_content_length);
  char *buf = (char *) malloc(5000);
  char *c   = NULL;
  int res   = read(child_stdout[0], c, 1);;
  int count = 0;
  int x     = 1;
  
  while (res > 0)
  {
    if (res == -1)
    {
      throw ControllerError("Could not read output of `xdg-mime`");
    }
    
    buf[count++] = *c;
    if (count == x * 5000)
    {
      x++;
      buf = (char *) realloc(buf, x * 5000);
    }
    
    res = read(child_stdout[0], c, 1);
  }
  buf[4999] = '\0';
  
  if (close(child_stdout[0]) == -1)
    {
      throw ControllerError("Could not close pipe output");
    }

  // remove the trailing newline before returning
  std::string content_type(buf);
  content_type = content_type.substr(0, content_type.size() - 1);

  d_printf("Got content type of %s for %s", content_type.c_str(), filename.c_str());

  return content_type;
}
