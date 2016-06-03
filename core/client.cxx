/*
 * marina client implementation
 */

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <fmt/format.h>
#include "common/net/http_request.hxx"
#include "core/util.hxx"
#include "core/blueprint.hxx"

using std::string;
using std::vector;
using std::ofstream;
using std::ifstream;
using std::istreambuf_iterator;
using std::cout;
using std::cerr;
using std::endl;
using std::out_of_range;
using proxygen::HTTPMethod;
using namespace marina;

static const string COMMENT {"\e[38;5;242m"};
static const string PARAM {"\e[38;5;74m"};
static const string CMD {"\e[38;5;47m"};
static const string NORMAL {"\e[39m"};
static const string ATTN {"\e[38;5;214m"};

static string usage = fmt::format(
R"({A}usage: {X}marina {P}cmd{N}
  {P}cmd{N}:
   {C}general{N}
   {X}bps{N} {P}project        {C}show blueprints in a project
   {X}mzs{N} {P}project        {C}show materializations in a project

   {C}experiment control{N}
   {X}cc {P}source{N}                {C}compile a blueprint{N}
   {X}save {P}blueprint project{N}   {C}save a compiled blueprint{N}
   {X}rm {P}blueprint project{N}     {C}remove a saved blueprint{N}
   {X}up {P}project blueprint{N}     {C}materialize a blueprint{N}
   {X}down {P}project blueprint{N}   {C}dematerialize a blueprint{N}

   {C}experiment access
   {X}console {P}mzn node         {C}get a vnc console on a materialized node
   {X}ssh {P}mzn node             {C}get a secure shell on a materialized node
{N})",
  fmt::arg("A", ATTN),
  fmt::arg("C", COMMENT),
  fmt::arg("N", NORMAL),
  fmt::arg("P", PARAM),
  fmt::arg("X", CMD)
);

static string url{"https://api"};


void fail()
{
  cerr << usage << endl;
  exit(1);
}

void bps(string pid)
{
  Json msg;
  msg["project"] = pid;

  HttpRequest rq{
    HTTPMethod::POST,
    url+"/blueprint/list",
    msg.dump(2)
  };
  auto res = rq.response().get();
  if(res.msg->getStatusCode() != 200)
  {
    cerr << "fetching blueprints failed" << endl;
    cerr << "code: " << res.msg->getStatusCode() << endl;
    exit(1);
  }

  Json j = res.bodyAsJson();
  try
  {
    Json js = j.at("blueprints");
    for(const Json x : js)
    {
      Blueprint b = Blueprint::fromJson(x);
      cout << b.name() << endl;
    }
  }
  catch(out_of_range &)
  {
    cerr << "fetching blueprints failed" << endl;
    cerr << "marinatb returned:" << endl;
    cerr << j << endl;
  }
}

void mzs(string pid)
{
  Json msg;
  msg["project"] = pid;

  HttpRequest rq{
    HTTPMethod::POST,
    url+"/materialization/list",
    msg.dump(2)
  };

  auto res = rq.response().get();
  if(res.msg->getStatusCode() != 200)
  {
    cerr << "fetching materializations failed" << endl;
    cerr << "code: " << res.msg->getStatusCode() << endl;
    exit(1);
  }

  Json j = res.bodyAsJson();
  try
  {
    Json js = j.at("materializations");
    for(const Json x : js)
    {
      Blueprint b = Blueprint::fromJson(x);
      cout << b.name() << endl;
    }
  }
  catch(out_of_range &)
  {
    cerr << "fetching materializations failed" << endl;
    cerr << "marinatb returned:" << endl;
    cerr << j << endl;
  }

}

string get_home()
{
  char *home = getenv("HOME");
  if(home == nullptr)
  {
    cerr
      << "are you lost? where is $HOME?" << endl;
    exit(1);
  }
  return string{home};
}

void cc(string src)
{
  char *mrsrc = getenv("MARINA_SRC");

  if(mrsrc == nullptr)
  {
    cerr 
      << "you must set the environment variable MARINA_SRC to use cc" << endl;
    exit(1);
  }
  
  string home = get_home();

  exec(fmt::format("mkdir -p {}/.marina/cc", home));
  string lib = fmt::format("{}/.marina/cc/{}.so", home, basename(src.c_str()));
  CmdResult cr = 
    exec(
      fmt::format(
        "clang++ "
        "-stdlib=libc++ -std=c++14 -fPIC -shared "
        "-I{mrsrc} -I/usr/local/include/c++/v1 "
        "{src} "
        "-o {out}",
        fmt::arg("src", src),
        fmt::arg("out", lib),
        fmt::arg("mrsrc", mrsrc)
      )
    );
  
  if(cr.code != 0)
  {
    cerr << "failed to compile " << src << endl;
    cerr << cr.output << endl;
    exit(1);
  }

  //load the compiled user code and get the blueprint
  void *dh = dlopen(lib.c_str(), RTLD_LAZY);

  if(dh == nullptr)
  {
    cerr << "error loading user model code" << endl;
    cerr << dlerror() << endl;
    exit(1);
  }

  Blueprint (*bpf)();
  bpf = (Blueprint (*)())dlsym(dh, "bp");
  Blueprint bp = bpf();
  cout << "compiled blueprint " << bp.name() << endl;
  cout << "found " << bp.computers().size() << " computers" << endl;
  cout << "found " << bp.networks().size() << " networks" << endl;

  string ir = fmt::format("{}/.marina/cc/{}.json", 
      home, 
      bp.name()
  );

  ofstream ofs{ir};
  ofs << bp.json().dump(2);
  ofs.close();
}

void save(string bid, string pid)
{
  string home = get_home();
  string ir = fmt::format("{}/.marina/cc/{}.json", home, bid);

  ifstream ifs{ir, std::ios::binary};
  if(!ifs.good())
  {
    cerr << "could not open find blueprint " << bid << endl;
    exit(1);
  }

  string ir_text;
  ifs.seekg(0, std::ios::end);
  ir_text.reserve(ifs.tellg());
  ifs.seekg(0, std::ios::beg);

  ir_text.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());

  Json msg;
  msg["project"] = pid;
  msg["source"] = Json::parse(ir_text);

  HttpRequest rq{
    HTTPMethod::POST,
    "https://api/blueprint/save",
    msg.dump(2)
  };
  auto res = rq.response().get();
  if(res.msg->getStatusCode() != 200)
  {
    cerr << "failed to save blueprint" << endl;
    exit(1);
  }
}

void rm(string bid, string pid)
{
  Json msg;
  msg["project"] = pid;
  msg["bpid"] = bid;

  HttpRequest rq{
    HTTPMethod::POST,
    "https://api/blueprint/delete",
    msg.dump(2)
  };
  auto res = rq.response().get();
  if(res.msg->getStatusCode() != 200)
  {
    cerr << "failed to remove blueprint" << endl;
    exit(1);
  }
}

void up(string pid, string bid)
{
  cout << __func__ << endl;
}

void down(string pid, string bid)
{
  cout << __func__ << endl;
}

void console(string mzn, string node)
{
  cout << __func__ << endl;
}

void ssh(string mzn, string console)
{
  cout << __func__ << endl;
}


int main(int argc, char **argv)
{
  if(argc < 2) fail();

  string cmd{argv[1]};
  if(cmd == "bps") 
  {
    if(argc != 3) fail();
    bps(argv[2]);
  }
  else if(cmd == "mzs") 
  {
    if(argc != 3) fail();
    mzs(argv[2]);
  }
  else if(cmd == "cc")
  {
    if(argc != 3) fail();
    cc(argv[2]);
  }
  else if(cmd == "save")
  {
    if(argc != 4) fail();
    save(argv[2], argv[3]);
  }
  else if(cmd == "rm")
  {
    if(argc != 4) fail();
    rm(argv[2], argv[3]);
  }
  else if(cmd == "up")
  {
    if(argc != 4) fail();
    up(argv[2], argv[3]);
  }
  else if(cmd == "down")
  {
    if(argc != 4) fail();
    down(argv[2], argv[3]);
  }
  else if(cmd == "console")
  {
    if(argc != 4) fail();
    console(argv[2], argv[3]);
  }
  else if(cmd == "ssh")
  {
    if(argc != 4) fail();
    ssh(argv[2], argv[3]);
  }
  else fail();
}

