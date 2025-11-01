#include "process.hpp"
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>

using namespace std;
using namespace TinyProcessLib;

static void check(std::vector<std::string> cmd, bool ok) {
  {
    Process process(cmd);
    if(ok)
      assert(process.get_exit_status() == 0);
    else
      assert(process.get_exit_status() != 0);
  }

  {
    Process process(cmd, "", Process::environment_type{});
    if(ok)
      assert(process.get_exit_status() == 0);
    else
      assert(process.get_exit_status() != 0);
  }
}

int main() {
#ifndef _WIN32
  {
    // default PATH
    check({"echo"}, true);
  }

  {
    // custom PATH
    setenv("PATH", "/bin:/usr/bin", 1);
    check({"echo"}, true);
  }

  {
    // empty dirs in PATH
    setenv("PATH", ":::/bin::::/usr/bin:::", 1);
    check({"echo"}, true);
  }

  {
    // one dir in PATH is longer than PATH_MAX
    static char path[PATH_MAX * 3] = {};
    for(int i = 0; i < PATH_MAX * 2; i++) {
      strcat(path, "x");
    }
    strcat(path, ":/bin:/usr/bin");
    setenv("PATH", path, 1);
    check({"echo"}, true);
  }

  {
    // each dir is short, but PATH in total is longer than PATH_MAX
    static char path[PATH_MAX * 3] = {};
    for(int i = 0; i < PATH_MAX; i++) {
      strcat(path, "x:");
    }
    strcat(path, "/bin:/usr/bin");
    setenv("PATH", path, 1);
    check({"echo"}, true);
  }

  {
    // PATH is not set (_CS_PATH should be used)
    unsetenv("PATH");
    check({"echo"}, true);
  }

  {
    // PATH is set to ""
    setenv("PATH", "", 1);
    check({"echo"}, false); // ERROR
  }

  {
    // PATH is set to empty dirs only
    setenv("PATH", "::::", 1);
    check({"echo"}, false); // ERROR
  }

  {
    // PATH is set to "", but search in PATH is not needed
    setenv("PATH", "", 1);
    check({"/bin/echo"}, true);
  }

  {
    // exe name is longer than NAME_MAX
    setenv("PATH", "/bin:/usr/bin", 1);
    std::string name;
    for(int i = 0; i < NAME_MAX + 1; i++) {
      name += "x";
    }
    check({name}, false); // ERROR
  }

  {
    // exe name is longer than PATH_MAX
    setenv("PATH", "/bin:/usr/bin", 1);
    std::string name;
    for(int i = 0; i < PATH_MAX + 1; i++) {
      name += "x";
    }
    check({name}, false); // ERROR
  }

  {
    // exe name is empty
    setenv("PATH", "/bin:/usr/bin", 1);
    check({""}, false); // ERROR
  }
#endif
}
