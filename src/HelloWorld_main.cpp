/*
 * @Author: your name
 * @Date: 2021-09-30 02:21:34
 * @LastEditTime: 2021-10-09 09:38:49
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /wangs/test/dds_rslidar/src/HelloWorld_main.cpp
 */

#include "HelloWorldPublisher.h"
#include "HelloWorldSubscriber.h"

#include <fastrtps/Domain.h>
#include <fastrtps/log/Log.h>
#include <fstream>
#include <string>

#include "optionparser.h"

using namespace eprosima;
using namespace fastrtps;
using namespace rtps;
int main(int argc, char** argv)
{
    std::cout << "Starting " << std::endl;
    int type = 1;
    int count = 0;
    long sleep = 100;
    std::string wan_ip= "10.3.1.241";
    int port = 5200;
    bool use_tls = false;
    // std::string wan_ip = "172.30.1.124";
    std::vector<std::string> whitelist;
    
    std::fstream readfile;
    readfile.open("1624329432.606174.string",std::ios::in|std::ios::binary);
    // std::vector<char> readSeq((std::istreambuf_iterator<char>(readfile)),  std::istreambuf_iterator<char>());
    std::vector<uint8_t> readSeq((std::istreambuf_iterator<char>(readfile)),  std::istreambuf_iterator<char>());

    HelloWorldPublisher mypub;
    if (mypub.init(wan_ip, static_cast<uint16_t>(port), use_tls, whitelist,readSeq))
       {
           mypub.run(count, sleep);
       }
            
   
    // HelloWorldSubscriber mysub;
    // if (mysub.init(wan_ip, static_cast<uint16_t>(port), use_tls, whitelist))
    //     {
    //         mysub.run();
    //     }
    Domain::stopAll();
    return 0;
}
