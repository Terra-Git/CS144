#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_rto_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_rto_ms_( initial_rto_ms ),timer_(initial_rto_ms)
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return next_seqno_ - receive_seqno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmission_count;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if( !syn_ ){
    return {};
  }else if(!send_queue_.empty()){
    if( !timer_.is_start()){
      timer_.reset();
      timer_.timer_start();
    }
    outstanding_queue_.push(std::move(send_queue_.front()));
    send_queue_.pop();
    return outstanding_queue_.back();
  }
  return {};
}

// windows size 初始值为 1， 保证了发出去的第一个数据为 syn
// reader 中取数据的时候，需要注意，如果stream关闭了，是存在取不到数据的情况的
// 此时存在一个fin
void TCPSender::push( Reader& outbound_stream )
{
  if(!fin_){return ;}
  uint64_t avilvble_size = windows_size_ - sequence_numbers_in_flight();
  while( avilvble_size && !fin_ ){
    TCPSenderMessage message;
    auto& buffer = message.payload;
    read(outbound_stream, min(avilvble_size ,TCPConfig::MAX_PAYLOAD_SIZE), buffer);
    avilvble_size -= message.payload.size();
    message.seqno = Wrap32::wrap(next_seqno_,isn_);
    if(!syn_){
      message.SYN = true;
      syn_ = true;
    }
    if( !fin_ and outbound_stream.is_finished() and avilvble_size ){
      message.FIN = true;
      fin_ = true;
    }
    if( !syn_ and !fin_ and message.payload.empty()){
      return ;
    }
    send_queue_.push(std::move(message));
    next_seqno_ += send_queue_.back().sequence_length();
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  TCPSenderMessage messages;
  messages.seqno = Wrap32::wrap(next_seqno_, isn_);
  return messages ;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if( ! msg.ackno.has_value()) { return ;}
  uint64_t receive_seqno = msg.ackno->unwrap(isn_, receive_seqno_);
  if( receive_seqno < receive_seqno_ && receive_seqno > next_seqno_ ){ 
    return ;
  }else{
    receive_seqno_ = receive_seqno;
  }

  windows_size_ = msg.window_size;
  retransmission_count = 0;
  timer_.reset();

  while(!outstanding_queue_.empty()){
    auto& message = outstanding_queue_.front();
    if((message.seqno.unwrap(isn_, receive_seqno_) + message.sequence_length() ) <= receive_seqno){
      outstanding_queue_.pop();
    }else{
      break;
    }
  }
  if( !outstanding_queue_.empty() ){
    timer_.timer_start();
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  timer_.tick(ms_since_last_tick);
  if( timer_.is_start() && timer_.is_expire()){
    while( !outstanding_queue_.empty() ){
      auto& message = outstanding_queue_.front();
      if( message.seqno.unwrap(isn_,receive_seqno_) <= receive_seqno_ ){
        outstanding_queue_.pop();
      }else{
        break;
      }
      if( !outstanding_queue_.empty()){
        auto& re_message = outstanding_queue_.front();
        send_queue_.push(re_message);
        retransmission_count++;
        if( !receive_seqno_  and windows_size_ ){ timer_.double_RTO();}
        else{ timer_.reset();timer_.timer_start();} 
      }else{
        timer_.timer_stop();
      }
    }
  }
}
