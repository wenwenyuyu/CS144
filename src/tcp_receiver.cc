#include "tcp_receiver.hh"

using namespace std;

// 给相对索引，压入绝对索引
void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if(!isn_.has_value()){
    if(!message.SYN){
     return;
    }
    isn_ = message.seqno;
  }

  uint64_t checkpoint = reassembler.first_unassembled_index_;
  uint64_t abs_seqno = message.seqno.unwrap(isn_.value(), checkpoint);
  uint64_t first_index = message.SYN ? 0 : abs_seqno - 1;
  reassembler.insert(first_index, message.payload.release(), message.FIN, inbound_stream);
}

//stream_index to abs_index
TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage msg{};
  uint64_t win_sz = inbound_stream.available_capacity();
  msg.window_size = win_sz < UINT16_MAX ? win_sz : UINT16_MAX;
  if(isn_.has_value()){
    uint64_t abs_seqno = inbound_stream.bytes_pushed() + 1 + inbound_stream.is_closed();
    msg.ackno = Wrap32::wrap(abs_seqno, isn_.value());
  }
  return msg;
}
