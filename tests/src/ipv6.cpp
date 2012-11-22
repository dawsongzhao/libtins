#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <stdint.h>
#include "ipv6.h"
#include "tcp.h"
#include "udp.h"
#include "icmp.h"
#include "rawpdu.h"
#include "ipv6_address.h"
#include "utils.h"

using namespace std;
using namespace Tins;

class IPv6Test : public testing::Test {
public:
    static const uint8_t expected_packet1[], expected_packet2[];
    
    void test_equals(IPv6 &ip1, IPv6 &ip2);
};

const uint8_t IPv6Test::expected_packet1[] = {
    'i', '\xa8', '\'', '4', '\x00', '(', '\x06', '@', '\x00', '\x00', 
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 
    '\x00', '\x00', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x00', 
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 
    '\x00', '\x00', '\x01', '\xc6', '\x8c', '\x00', 'P', 'h', 'H', '\x03', 
    '\x0c', '\x00', '\x00', '\x00', '\x00', '\xa0', '\x02', '\x7f', '\xf0', 
    '\x00', '0', '\x00', '\x00', '\x02', '\x04', '?', '\xf8', '\x04', 
    '\x02', '\x08', '\n', '\x00', '\x84', '\xa3', '\x9c', '\x00', '\x00', 
    '\x00', '\x00', '\x01', '\x03', '\x03', '\x07'
};

const uint8_t IPv6Test::expected_packet2[] = {
    '`', '\x00', '\x00', '\x00', '\x00', '$', '\x00', '\x01', '\xfe', 
    '\x80', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x02', 
    '\xd0', '\t', '\xff', '\xfe', '\xe3', '\xe8', '\xde', '\xff', '\x02', 
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 
    '\x00', '\x00', '\x00', '\x00', '\x16', ':', '\x00', '\x05', '\x02', 
    '\x00', '\x00', '\x01', '\x00', '\x8f', '\x00', 't', '\xfe', '\x00', 
    '\x00', '\x00', '\x01', '\x04', '\x00', '\x00', '\x00', '\xff', '\x02', 
    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', 
    '\x01', '\xff', '\x98', '\x06', '\xe1'
};

void IPv6Test::test_equals(IPv6 &ip1, IPv6 &ip2) {
    EXPECT_EQ(ip1.version(), ip2.version());
    EXPECT_EQ(ip1.traffic_class(), ip2.traffic_class());
    EXPECT_EQ(ip1.flow_label(), ip2.flow_label());
    EXPECT_EQ(ip1.payload_length(), ip2.payload_length());
    EXPECT_EQ(ip1.next_header(), ip2.next_header());
    EXPECT_EQ(ip1.hop_limit(), ip2.hop_limit());
    EXPECT_EQ(ip1.dst_addr(), ip2.dst_addr());
    EXPECT_EQ(ip1.src_addr(), ip2.src_addr());
    
    EXPECT_EQ(bool(ip1.search_option(IPv6::HOP_BY_HOP)), bool(ip2.search_option(IPv6::HOP_BY_HOP)));
    const IPv6::ipv6_ext_header *header1 = ip1.search_option(IPv6::HOP_BY_HOP),
                                    *header2 = ip2.search_option(IPv6::HOP_BY_HOP);
    if(header1 && header2) {
        EXPECT_EQ(header1->data_size(), header2->data_size());
    }
    
    EXPECT_EQ(bool(ip1.inner_pdu()), bool(ip2.inner_pdu()));
    
    const RawPDU *raw1 = ip1.find_pdu<RawPDU>(), *raw2 = ip2.find_pdu<RawPDU>();
    ASSERT_EQ(bool(raw1), bool(raw2));
    
    if(raw1) {
        EXPECT_EQ(raw1->payload(), raw2->payload());
    }
}

TEST_F(IPv6Test, Constructor) {
    IPv6 ipv6("::1:2:3", "f0aa:beef::1");
    EXPECT_EQ(ipv6.version(), 6);
    EXPECT_EQ(ipv6.traffic_class(), 0);
    EXPECT_EQ(ipv6.flow_label(), 0);
    EXPECT_EQ(ipv6.payload_length(), 0);
    EXPECT_EQ(ipv6.next_header(), 0);
    EXPECT_EQ(ipv6.hop_limit(), 0);
    EXPECT_EQ(ipv6.dst_addr(), "::1:2:3");
    EXPECT_EQ(ipv6.src_addr(), "f0aa:beef::1");
}

TEST_F(IPv6Test, ConstructorFromBuffer) {
    IPv6 ipv6(expected_packet1, sizeof(expected_packet1));
    EXPECT_EQ(ipv6.version(), 6);
    EXPECT_EQ(ipv6.traffic_class(), 0x9a);
    EXPECT_EQ(ipv6.flow_label(), 0x82734);
    EXPECT_EQ(ipv6.payload_length(), 40);
    EXPECT_EQ(ipv6.next_header(), 6);
    EXPECT_EQ(ipv6.hop_limit(), 64);
    EXPECT_EQ(ipv6.dst_addr(), "::1");
    EXPECT_EQ(ipv6.src_addr(), "::1");
    ASSERT_TRUE(ipv6.inner_pdu());
    TCP *tcp = ipv6.find_pdu<TCP>();
    ASSERT_TRUE(tcp);
    EXPECT_EQ(tcp->sport(), 50828);
    EXPECT_EQ(tcp->dport(), 80);
}

// This one has a hop-by-hop ext header
TEST_F(IPv6Test, ConstructorFromBuffer2) {
    IPv6 ipv6(expected_packet2, sizeof(expected_packet2));
    EXPECT_EQ(ipv6.version(), 6);
    EXPECT_EQ(ipv6.traffic_class(), 0);
    EXPECT_EQ(ipv6.flow_label(), 0);
    EXPECT_EQ(ipv6.payload_length(), 36);
    EXPECT_EQ(ipv6.next_header(), IPv6::HOP_BY_HOP);
    EXPECT_EQ(ipv6.hop_limit(), 1);
    EXPECT_EQ(ipv6.dst_addr(), "ff02::16");
    EXPECT_EQ(ipv6.src_addr(), "fe80::2d0:9ff:fee3:e8de");
    // This will have to be changed when ICMPv6 is implemented
    RawPDU *pdu = ipv6.find_pdu<RawPDU>();
    ASSERT_TRUE(pdu);
    EXPECT_EQ(pdu->payload_size(), 28);
    
    const IPv6::ipv6_ext_header *header = ipv6.search_option(IPv6::HOP_BY_HOP);
    ASSERT_TRUE(header);
    EXPECT_EQ(header->data_size(), 6);
}

TEST_F(IPv6Test, Serialize) {
    IPv6 ip1(expected_packet2, sizeof(expected_packet2));
    IPv6::serialization_type buffer = ip1.serialize();
    ASSERT_EQ(buffer.size(), sizeof(expected_packet2));
    EXPECT_TRUE(std::equal(buffer.begin(), buffer.end(), expected_packet2));
    IPv6 ip2(&buffer[0], buffer.size());
    test_equals(ip1, ip2);
}

TEST_F(IPv6Test, Version) {
    IPv6 ipv6;
    ipv6.version(3);
    EXPECT_EQ(ipv6.version(), 3);
}

TEST_F(IPv6Test, TrafficClass) {
    IPv6 ipv6;
    ipv6.traffic_class(0x7a);
    EXPECT_EQ(ipv6.traffic_class(), 0x7a);
}

TEST_F(IPv6Test, FlowLabel) {
    IPv6 ipv6;
    ipv6.flow_label(0x918d7);
    EXPECT_EQ(ipv6.flow_label(), 0x918d7);
}

TEST_F(IPv6Test, PayloadLength) {
    IPv6 ipv6;
    ipv6.payload_length(0xaf71);
    EXPECT_EQ(ipv6.payload_length(), 0xaf71);
}

TEST_F(IPv6Test, NextHeader) {
    IPv6 ipv6;
    ipv6.next_header(0x7a);
    EXPECT_EQ(ipv6.next_header(), 0x7a);
}

TEST_F(IPv6Test, HopLimit) {
    IPv6 ipv6;
    ipv6.hop_limit(0x7a);
    EXPECT_EQ(ipv6.hop_limit(), 0x7a);
}

TEST_F(IPv6Test, SourceAddress) {
    IPv6 ipv6;
    ipv6.src_addr("99af:1293::1");
    EXPECT_EQ(ipv6.src_addr(), "99af:1293::1");
}

TEST_F(IPv6Test, DestinationAddress) {
    IPv6 ipv6;
    ipv6.dst_addr("99af:1293::1");
    EXPECT_EQ(ipv6.dst_addr(), "99af:1293::1");
}
