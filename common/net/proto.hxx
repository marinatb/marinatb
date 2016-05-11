#ifndef MARINA_COMMON_NET_PROTO
#define MARINA_COMMON_NET_PROTO

#include <proxygen/lib/http/HTTPMessage.h>
#include <folly/io/IOBuf.h>
#include <3p/json/src/json.hpp>

namespace marina 
{ 
  using Json = nlohmann::json;
  namespace http 
  {
    struct Message
    {
      std::unique_ptr<proxygen::HTTPMessage> msg;
      std::unique_ptr<folly::IOBuf> content;

      std::string bodyAsString();
      Json bodyAsJson();
    };

    struct Status
    {
      static Status OK() { return Status{200, "OK"}; }
      static Status NotFound() { return Status{404, "Not Found"}; }
      static Status BadRequest() { return Status{400, "Bad Request"}; }
      static Status InternalServerError()
      {
        return Status{500, "Internal Server Error"};
      }
      unsigned short code;
      std::string message;
    };

    struct Response
    {
      Response(Status status, std::unique_ptr<folly::IOBuf> content);
      Response(Status status, std::string content);

      Status status;
      std::unique_ptr<folly::IOBuf> content;
    };
  }
}

#endif
