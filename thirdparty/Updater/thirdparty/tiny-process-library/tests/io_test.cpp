#include "process.hpp"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;
using namespace TinyProcessLib;

int main() {
  auto output = make_shared<string>();
  auto error = make_shared<string>();
  auto eof = make_shared<std::atomic<int>>(0);
  {
    Process process("echo Test", "", [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }

  {
    freopen("io_test_stdout.txt", "w", stdout);
    Process process("echo Test");
    assert(process.get_exit_status() == 0);
    stringstream ss;
    ifstream ifs("io_test_stdout.txt");
    ss << ifs.rdbuf();
    assert(ss.str().substr(0, 4) == "Test");
  }

  {
    Process process(std::vector<string>{"/bin/echo", "Test"}, "", [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }

  {
    Process process(std::vector<string>{"/bin/echo", "Test"}, "", {{"VAR1", "value1"}, {"VAR2", "value2"}}, [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }

  {
    Config config;
    Process process(
        std::vector<string>{"echo", "Test"}, "", [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        nullptr, false, config);
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }

#ifndef _WIN32
  {
    Process process("pwd", "/usr", [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "/usr");
    output->clear();
  }

  {
    Process process(std::vector<string>{"/bin/pwd"}, "/usr", [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "/usr");
    output->clear();
  }

  {
    Process process(std::vector<string>{"/bin/sh", "-c", "echo $VAR1 $VAR2"}, "", {{"VAR1", "value1"}, {"VAR2", "value2"}}, [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 13) == "value1 value2");
    output->clear();
  }

  {
    Process process("while true; do sleep 10000; done");
    int exit_status;
    assert(!process.try_get_exit_status(exit_status));
    process.kill();
    assert(process.get_exit_status() != 0);
  }

  {
    Process process(
        [] {
          cout << "Test" << endl;
          exit(0);
        },
        [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }
  {
    Config config;
    config.buffer_size = 4;
    Process process(
        std::vector<string>{"printf", "Hello, world!\nHi, there!"}, {}, [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        nullptr, false, config);
    assert(process.get_exit_status() == 0);
    assert(*output == "Hello, world!\nHi, there!");
    output->clear();
  }
#endif

  {
    Process process("ls an_incorrect_path", "", nullptr, [error](const char *bytes, size_t n) {
      *error += string(bytes, n);
    });
    assert(process.get_exit_status() > 0);
    assert(!error->empty());
    error->clear();
  }

  {
    Process process(
        "echo Test && ls an_incorrect_path", "",
        [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        [error](const char *bytes, size_t n) {
          *error += string(bytes, n);
        });
    assert(process.get_exit_status() > 0);
    assert(output->substr(0, 4) == "Test");
    assert(!error->empty());
    output->clear();
    error->clear();
  }

  {
    Config config;
    config.on_stdout_close = [eof]() {
      ++*eof;
    };
    Process process(
        std::vector<string>{"/bin/echo", "Test"}, "", [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        nullptr, false, config);
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    assert(*eof == 1);
    output->clear();
    *eof = 0;
  }

  {
    Config config;
    config.on_stderr_close = [eof]() {
      ++*eof;
    };
    Process process(
        "ls an_incorrect_path", "", nullptr, [error](const char *bytes, size_t n) {
          *error += string(bytes, n);
        },
        false, config);
    assert(process.get_exit_status() > 0);
    assert(!error->empty());
    assert(*eof == 1);
    error->clear();
    *eof = 0;
  }

  {
    Config config;
    config.on_stdout_close = [eof]() {
      ++*eof;
    };
    config.on_stderr_close = [eof]() {
      ++*eof;
    };
    Process process(
        "echo Test && ls an_incorrect_path", "",
        [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        [error](const char *bytes, size_t n) {
          *error += string(bytes, n);
        },
        false, config);
    assert(process.get_exit_status() > 0);
    assert(output->substr(0, 4) == "Test");
    assert(!error->empty());
    assert(*eof == 2);
    output->clear();
    error->clear();
    *eof = 0;
  }

  {
    Process process(
        "bash", "",
        [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        nullptr, true);
    process.write("echo Test\n");
    process.write("exit\n");
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }

  {
    Process process(
        "cat", "",
        [output](const char *bytes, size_t n) {
          *output += string(bytes, n);
        },
        nullptr, true);
    process.write("Test\n");
    process.close_stdin();
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 4) == "Test");
    output->clear();
  }

  {
    Process process("sleep 5");
    int exit_status = -2;
    assert(!process.try_get_exit_status(exit_status));
    assert(exit_status == -2);
    this_thread::sleep_for(chrono::seconds(3));
    assert(!process.try_get_exit_status(exit_status));
    assert(exit_status == -2);
    this_thread::sleep_for(chrono::seconds(5));
    assert(process.try_get_exit_status(exit_status));
    assert(exit_status == 0);
    assert(process.try_get_exit_status(exit_status));
    assert(exit_status == 0);
  }

  {
    Process process("echo $VAR1 $VAR2", "", {{"VAR1", "value1"}, {"VAR2", "value2"}}, [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 13) == "value1 value2");
    output->clear();
  }

  {
    Process process("echo $VAR1 $VAR2", "", {{"VAR1", "value1 value2"}, {"VAR2", "\"value3 value 4\""}}, [output](const char *bytes, size_t n) {
      *output += string(bytes, n);
    });
    assert(process.get_exit_status() == 0);
    assert(output->substr(0, 30) == "value1 value2 \"value3 value 4\"");
    output->clear();
  }
}
