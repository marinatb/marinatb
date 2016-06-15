#include <sys/socket.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include "core/util.hxx"

using std::string;
using std::endl;
using std::out_of_range;
using namespace marina;

string marina::mac_2_ifname(string mac)
{
  static size_t seq_num{0};

  /*
    * send netlink request
    */

  //prepare request
  RtReq req;
  memset(&req, 0, sizeof(req));
  req.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
  req.header.nlmsg_type = RTM_GETLINK;
  req.header.nlmsg_seq = ++seq_num;
  req.msg.ifi_family = AF_UNSPEC;
  req.msg.ifi_change = 0xffffffff;
  
  //prepare message
  sockaddr_nl sa;
  iovec iov = {&req, req.header.nlmsg_len};
  msghdr msg = {&sa, sizeof(sa), &iov, 1, nullptr, 0, 0};
  memset(&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;

  //open socket
  int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

  //send message
  sendmsg(fd, &msg, 0);

  /*
    * get netlink response
    */

  char buf[16192];
  iov = {buf, sizeof(buf) };
  msg = {&sa, sizeof(sa), &iov, 1, nullptr, 0, 0};
  nlmsghdr *nh;

  bool over{false};
  while(!over)
  {
    size_t len = recvmsg(fd, &msg, 0);
    for(nh = (nlmsghdr*)buf; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len))
    {
      if(nh->nlmsg_type == NLMSG_DONE) 
      {
        over = true;
        break;
      }

      if(nh->nlmsg_type != RTM_BASE) continue;

      // get message payload
      ifinfomsg *msg = (ifinfomsg*)NLMSG_DATA(nh);
      if(msg->ifi_type != ARPHRD_ETHER) continue;

      //get message attributes
      rtattr *rta = IFLA_RTA(msg);
      int alen = nh->nlmsg_len - NLMSG_LENGTH(sizeof(*msg));

      //store the interface name and mac addr here
      string name, addr;

      //iterate through attributes
      for(; RTA_OK(rta, alen); rta = RTA_NEXT(rta, alen))
      {
        //grab the mac address attribute
        if(rta->rta_type == IFLA_ADDRESS)
        {
          char addr_buf[64];
          unsigned char * c = (unsigned char*)RTA_DATA(rta);

          snprintf(addr_buf, 64, "%02x:%02x:%02x:%02x:%02x:%02x", 
              c[0], c[1], c[2], c[3], c[4], c[5]);

          addr = string{addr_buf};
        }

        //grab the interface name attribute
        if(rta->rta_type == IFLA_IFNAME)
        {
          name = string{(char*)RTA_DATA(rta)};
        }
      }
      if(mac == addr) return name;
    }
  }
  throw out_of_range{mac + " not found"};
}

