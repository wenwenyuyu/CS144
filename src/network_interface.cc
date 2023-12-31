#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t target_ip = next_hop.ipv4_numeric();
  if(ip2ethernet_.count(target_ip)){
    EthernetFrame frame {{ip2ethernet_[target_ip].first, ethernet_address_, EthernetHeader::TYPE_IPv4}, serialize(dgram)};
    frames_.push(std::move(frame));
  }else{
    if(!arp_timer_.count(target_ip)){
      ARPMessage request_msg = {
        .opcode = ARPMessage::OPCODE_REQUEST,
        .sender_ethernet_address = ethernet_address_,
        .sender_ip_address = ip_address_.ipv4_numeric(),
        .target_ip_address = target_ip
      };
      EthernetFrame frame{{ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(request_msg)};
      frames_.push(std::move(frame));
      waited_dram_.insert({target_ip, {dgram}});
      arp_timer_.insert({target_ip, 0});
    }else{
      waited_dram_[target_ip].push_back(dgram);
    }
  }

}

// frame: the incoming Ethernet frame
// If type is IPv4, returns the datagram.
// If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
// If type is ARP reply, learn a mapping from the "sender" fields.
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return {};
  }

  if(frame.header.type == frame.header.TYPE_IPv4){
    InternetDatagram internetDatagram;
    if(parse(internetDatagram, frame.payload)){
      return internetDatagram;
    }
  }
  else if(frame.header.type == frame.header.TYPE_ARP){
    ARPMessage msg;
    if(parse(msg, frame.payload)){
      ip2ethernet_.insert({msg.sender_ip_address, {msg.sender_ethernet_address, 0}});
      if(msg.opcode == msg.OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric()){
        ARPMessage reply_msg = {
          .opcode = ARPMessage::OPCODE_REPLY,
          .sender_ethernet_address = ethernet_address_,
          .sender_ip_address = ip_address_.ipv4_numeric(),
          .target_ethernet_address = msg.sender_ethernet_address,
          .target_ip_address = msg.sender_ip_address
        };
        EthernetFrame reply_frame {{msg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(reply_msg)};
        frames_.push(std::move(reply_frame));
      }
      else if(msg.opcode == msg.OPCODE_REPLY){
        auto &dgrams = waited_dram_[msg.sender_ip_address];
        for(auto &dgram : dgrams){
          send_datagram(dgram, Address::from_ipv4_numeric(msg.sender_ip_address));
        }
        waited_dram_.erase(msg.sender_ip_address);
      }
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  static constexpr size_t IP_MAP_TTL = 30000;     // 30s
  static constexpr size_t ARP_REQUEST_TTL = 5000; // 5s

  for ( auto it = ip2ethernet_.begin(); it != ip2ethernet_.end(); ) {
    it->second.second += ms_since_last_tick;
    if ( it->second.second >= IP_MAP_TTL ) {
      it = ip2ethernet_.erase( it );
    } else {
      ++it;
    }
  }

  for ( auto it = arp_timer_.begin(); it != arp_timer_.end(); ) {
    it->second += ms_since_last_tick;
    if ( it->second >= ARP_REQUEST_TTL ) {
      it = arp_timer_.erase( it );
    } else {
      ++it;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(frames_.empty()){
    return {};
  }
  auto frame = frames_.front();
  frames_.pop();
  return frame;
}
