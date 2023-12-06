#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return seq_outstanding;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return con_retransmissions_cnt;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if(queued_segment.empty()){
    return {};
  }
  if(!timer_.isRunning()){
    timer_.start();
  }
  auto msg = queued_segment.front();
  queued_segment.pop_front();
  return msg;
}

//尽可能从outbound_stream中读数据
//syn如果没有设置，要设置syn
//把outbound_stream读完了要设置fin_
//尽可能多读数据，取决于对方的窗口大小和自身已经存在outstanding队列中的数量
//读完数据存储在两个队列中
// void TCPSender::push( Reader& outbound_stream )
// {
//   size_t curr_window_size = win_sz != 0 ? win_sz : 1;
//   while ( seq_outstanding < curr_window_size ) {
//     TCPSenderMessage msg;
//     if ( !syn_ ) {
//       syn_ = msg.SYN = true;
//       seq_outstanding += 1;
//     }
//     msg.seqno = Wrap32::wrap( next_seqno, isn_ );

//     auto const payload_size = min( TCPConfig::MAX_PAYLOAD_SIZE, curr_window_size - seq_outstanding );
//     read( outbound_stream, payload_size, msg.payload );
//     seq_outstanding += msg.payload.size();

//     if ( !fin_ && outbound_stream.is_finished() && seq_outstanding < curr_window_size ) {
//       fin_ = msg.FIN = true;
//       seq_outstanding += 1;
//     }

//     if ( msg.sequence_length() == 0 ) {
//       break;
//     }

//     queued_segment.push_back( msg );
//     next_seqno += msg.sequence_length();
//     outstanding_segment.push_back( msg );

//     if ( msg.FIN || outbound_stream.bytes_buffered() == 0 ) {
//       break;
//     }
//   }
// }
void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  uint16_t target_win_sz = win_sz != 0 ? win_sz : 1;
  while(seq_outstanding < target_win_sz){
    TCPSenderMessage msg;
    if(!syn_){
      msg.SYN = true;
      syn_ = true;
      seq_outstanding += 1;
    }
    msg.seqno = Wrap32::wrap(next_seqno, isn_);
    uint64_t size = min(TCPConfig::MAX_PAYLOAD_SIZE, target_win_sz - seq_outstanding);
    read(outbound_stream, size, msg.payload);
    seq_outstanding += msg.payload.size();

    if(!fin_ && outbound_stream.is_finished() && seq_outstanding < target_win_sz){
      msg.FIN = true;
      fin_ = true;
      seq_outstanding += 1;
    }

    if(msg.sequence_length() == 0)
      break;
    
    queued_segment.push_back(msg);
    next_seqno += msg.sequence_length();
    outstanding_segment.push_back(msg);

    if(msg.FIN || outbound_stream.bytes_buffered() == 0){
      break;
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  Wrap32 wrap = Wrap32::wrap(next_seqno, isn_);
  return {wrap, false, {}, false};
}

//接受对方窗口大小和下一个序列号
//将队列中序号小于确认序号的消息删除
void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  win_sz = msg.window_size;
  if(msg.ackno.has_value()){
    auto ackno = msg.ackno.value().unwrap(isn_, next_seqno);
    if(ackno > next_seqno){
      return;
    }

    acked_seqno = ackno;

    while(!outstanding_segment.empty()){
      auto &target = outstanding_segment.front();
      if(target.seqno.unwrap(isn_, next_seqno) + target.sequence_length() <= acked_seqno){
        seq_outstanding -= target.sequence_length();
        outstanding_segment.pop_front();

        timer_.reset();
        if(!outstanding_segment.empty()){
          timer_.start();
        }
        con_retransmissions_cnt = 0;
      }else{
        break;
      }
    }

    if(outstanding_segment.empty()){
      timer_.stop();
    }
  }
 
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer_.tick(ms_since_last_tick);
  if(timer_.isExpired()){
    queued_segment.push_back(outstanding_segment.front());
    if(win_sz != 0){
      con_retransmissions_cnt++;
      timer_.double_RTO();
    }
    timer_.start();
  }
}
