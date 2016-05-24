#include <iostream>
#include <fmt/format.h>
#include "core/blueprint.hxx"
#include "core/util.hxx"
#include "../catch.hpp"

using std::cout;
using std::endl;
using namespace marina;

/*
 *    system command execution tests
 */

TEST_CASE("ls", "[cmd-exec]")
{
  CmdResult cr = exec("ls -lh");  

  REQUIRE(cr.code == 0);

  cout << cr.output << endl;
}

TEST_CASE("multiout", "[cmd-exec]")
{
  CmdResult cr = exec("./multiout.sh");

  cout << "code: " << cr.code << endl;
  cout << cr.output;
}

