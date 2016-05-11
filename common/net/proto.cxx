#include "proto.hxx"

using std::string;
using std::unique_ptr;
using namespace marina;
using namespace marina::http;
using folly::IOBuf;

string http::Message::bodyAsString()
{
  IOBuf *p = content.get();
  if(p == nullptr) { return ""; }
  std::string s;
  do {
    s += string(reinterpret_cast<const char*>(p->data()), p->length());
    p = p->next();
  } while( p != content.get() );
  return s;
}

Json http::Message::bodyAsJson()
{
  return Json::parse(bodyAsString());
}

http::Response::Response(Status status, unique_ptr<IOBuf> content)
  : status(status),
    content{move(content)}
{ } 

http::Response::Response(Status status, string content)
  : status(status),
    content{IOBuf::copyBuffer(content)}
{ }
