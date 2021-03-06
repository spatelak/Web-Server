#ifndef CS252_EXECSCRIPTCONTROLLER_H
#define CS252_EXECSCRIPTCONTROLLER_H

#include <string>
#include <fstream>

#include "Config.hpp"
#include "controller/Controller.hpp"
#include "server/Request.hpp"
#include "server/Response.hpp"

class ExecScriptController : public Controller
{
private:
    /**
     * m_ignore's purpose is that when we request, say, GET /script/echo.sh, we actually want
     * to run <m_config.exec_dir>/echo.sh.
     * The fact that both are /script may not always be true!
     * So, when this controller is constructed, it will be told what part of the path
     * from the request that we should cut off of the front before prepending exec_dir
    **/
    std::string m_ignore;

    /**
     * :: TODO ::
     * Helper function to set all of the correct environment variables for the child process
    **/
    bool set_environment(Request const& req) const noexcept;

  /**                                                                                                                                                                           
   * Gets the content type of a given filename.                                                                                                                                 
   * Whatever this returns can be used when setting the Content-Type header.                                                                                                    
   * In order to do this, you need to call the `xdg-mime` command by fork()ing,                                                                                                 
   * exec()ing, and then reading its output through a pipe, similar to lab 3.                                                                                                   
   * Luckily for you, we have given you this code.                                                                                                                              
   * Use it wisely.                                                                                                                                                             
   **/
  std::string get_content_type(std::string const& filename, Request const& req) const;

  bool set_var(std::unordered_map<std::string, std::string> mapping, std::string key) const;
  int get_content_length(std::fstream& fs) const;
public:
    /**
     * Constructs an ExecScriptController with the given config and ignore path.
    **/
    ExecScriptController(Config const& config, std::string const& ignore);
    
    /**
     * :: TODO ::
     * When the ExecScriptController runs, it should attempt to execute the requested file.
     * Controller::resolve_requested_path should be used to validate and load the path of the file.
     * We always need to exec in a child process and read the output back when the child has finished.
     * Before execing, make sure to set up the scripts environment properly.
    **/
    void run(Request const& req, Response& res) const override;
};

#endif
