// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file HelloWorldPublisher.cpp
 *
 */

#include "HelloWorldPublisher.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/utils/IPLocator.h>
#include <sys/time.h>
#include <thread>
#include <fstream>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using IPLocator = eprosima::fastrtps::rtps::IPLocator;//+

HelloWorldPublisher::HelloWorldPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new HelloWorldPubSubType())
{
}

bool HelloWorldPublisher::init(
        const std::string& wan_ip,
        unsigned short port,
        bool use_tls,
        const std::vector<std::string>& whitelist,
        std::vector<uint8_t>& readSeq)
{
    stop_ = false;
    // hello_.index(0);
    struct timeval tv;    
    gettimeofday(&tv, NULL);    //该函数在sys/time.h头文件中
    double timestamp = tv.tv_sec+double(tv.tv_usec)/1000000.0;
    hello_.index(timestamp);
    // hello_.index(0);
    // hello_.message("HelloWorld");


    // hello_.message("HelloWorld");
    hello_.message(readSeq);

    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = eprosima::fastrtps::c_TimeInfinite;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod =
            eprosima::fastrtps::Duration_t(5, 0);
    pqos.name("Participant_pub");

    pqos.transport().use_builtin_transports = false;
    
    int32_t kind = LOCATOR_KIND_TCPv4;//+

    Locator initial_peer_locator;//+
    initial_peer_locator.kind = kind;//+
    
    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();

    for (std::string ip : whitelist)
    {
        descriptor->interfaceWhiteList.push_back(ip);
        std::cout << "Whitelisted " << ip << std::endl;
    }

    if (use_tls)
    {
        using TLSOptions = TCPTransportDescriptor::TLSConfig::TLSOptions;
        descriptor->apply_security = true;
        descriptor->tls_config.password = "test";
        descriptor->tls_config.cert_chain_file = "servercert.pem";
        descriptor->tls_config.private_key_file = "serverkey.pem";
        descriptor->tls_config.tmp_dh_file = "dh2048.pem";
        descriptor->tls_config.add_option(TLSOptions::DEFAULT_WORKAROUNDS);
        descriptor->tls_config.add_option(TLSOptions::SINGLE_DH_USE);
        descriptor->tls_config.add_option(TLSOptions::NO_SSLV2);
    }

    descriptor->sendBufferSize = 0;
    descriptor->receiveBufferSize = 0;

    // if (!wan_ip.empty())
    // {
    //     descriptor->set_WAN_address(wan_ip);
    //     std::cout << wan_ip << ":" << port << std::endl;
    // }
    if (!wan_ip.empty())//+
    {
        IPLocator::setIPv4(initial_peer_locator, wan_ip);
        std::cout << wan_ip << ":" << port << std::endl;
    }
    else
    {
        IPLocator::setIPv4(initial_peer_locator, "127.0.0.1");
    }
    initial_peer_locator.port = port;//+
    // descriptor->add_listener_port(port);
    pqos.wire_protocol().builtin.initialPeersList.push_back(initial_peer_locator); // Publisher's meta channel //+
    pqos.transport().user_transports.push_back(descriptor);
    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);

    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE PUBLISHER
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);

    if (publisher_ == nullptr)
    {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic("HelloWorldTopicTCP", "HelloWorld", TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return false;
    }

    //CREATE THE DATAWRITER
    DataWriterQos wqos;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 30;
    wqos.resource_limits().max_samples = 50;
    wqos.resource_limits().allocated_samples = 20;
    wqos.reliable_writer_qos().times.heartbeatPeriod.seconds = 2;
    wqos.reliable_writer_qos().times.heartbeatPeriod.nanosec = 200 * 1000 * 1000;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    writer_ = publisher_->create_datawriter(topic_, wqos, &listener_);

    if (writer_ == nullptr)
    {
        return false;
    }

    return true;
}

HelloWorldPublisher::~HelloWorldPublisher()
{
    if (writer_ != nullptr)
    {
        publisher_->delete_datawriter(writer_);
    }
    if (publisher_ != nullptr)
    {
        participant_->delete_publisher(publisher_);
    }
    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

void HelloWorldPublisher::PubListener::on_publication_matched(
        eprosima::fastdds::dds::DataWriter*,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        first_connected_ = true;
        std::cout << "[RTCP] Publisher matched" << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "[RTCP] Publisher unmatched" << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void HelloWorldPublisher::runThread(
        uint32_t samples,
        long sleep_ms)
{
    if (samples == 0)
    {
        while (!stop_)
        {
            if (publish(false))
            {
                //logError(HW, "SENT " <<  hello_.index());
                // std::cout << "[RTCP] Message: " << hello_.message() << " with index: "
                //           << hello_.index() << " SENT" << std::endl;
                printf("--------------------------------------------------------\n");
                printf("[RTCP1] Message length:%ld and with index:%lf SENT\n",hello_.message().size(),hello_.index());
                int size = hello_.getCdrSerializedSize(hello_);
                printf("cdr size:%d\n",size);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
    else
    {
        for (uint32_t i = 0; i < samples; ++i)
        {
            if (!publish())
            {
                --i;
            }
            else
            {
                // std::cout << "[RTCP] Message: " << hello_.message() << " with index: "
                //           << hello_.index() << " SENT" << std::endl;
                printf("[RTCP2] Message length:%ld and with index:%lf SENT\n",hello_.message().size(),hello_.index());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
}

void HelloWorldPublisher::run(
        uint32_t samples,
        long sleep_ms)
{
    std::thread thread(&HelloWorldPublisher::runThread, this, samples, sleep_ms);
    if (samples == 0)
    {
        std::cout << "Publisher running. Please press enter to stop_ the Publisher at any time." << std::endl;
        std::cin.ignore();
        stop_ = true;
    }
    else
    {
        std::cout << "Publisher running " << samples << " samples." << std::endl;
    }
    thread.join();
}

bool HelloWorldPublisher::publish(
        bool waitForListener)
{
    if (listener_.first_connected_ || !waitForListener || listener_.matched_ > 0)
    {
        // hello_.index(hello_.index() + 1);
        struct timeval tv;    
        gettimeofday(&tv, NULL);    //该函数在sys/time.h头文件中
        double timestamp = tv.tv_sec+double(tv.tv_usec)/1000000.0;
        // printf("publish timestamp: %lf\n",timestamp);
        hello_.index(timestamp);
        writer_->write((void*)&hello_);
        return true;
    }
    return false;
}
