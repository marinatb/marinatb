#include <iostream>
#include <proxygen/lib/utils/URL.h>
#include "common/net/http_request.hxx"
#include "../../catch.hpp"

using namespace proxygen;
using namespace folly;
using namespace std;
using namespace std::chrono;
using namespace marina;

TEST_CASE("http-client-remote-test", "[net]")
{
  HttpRequest req{
    HTTPMethod::POST, 
    "https://marina.deterlab.net",
    "Do you know the muffin man?"
  };
  REQUIRE( req.response().get().bodyAsString() == "The muffin man is me!" );

  req = HttpRequest{
    HTTPMethod::POST,
    "https://marina.deterlab.net",
    IOBuf::copyBuffer("Give me muffins")
  };
  REQUIRE( req.response().get().bodyAsString() == "Do I know you?" );
}

TEST_CASE("http-client-local-test", "[net]")
{
  HttpRequest req{
    HTTPMethod::GET,
    "https://localhost:4433/"
  };
  REQUIRE( req.response().get().bodyAsString() == "well, hello there!" );

  req = HttpRequest{
    HTTPMethod::GET,
    "https://localhost:4433/relay"
  };
  REQUIRE( req.response().get().bodyAsString() == "Do I know you?" );
}
