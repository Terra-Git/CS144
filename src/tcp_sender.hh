#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <queue>
#include <memory>

enum class TimerStatus{ start, stop, error};

class Timer
{
public:
  Timer(uint64_t time):initial_RTO_ms_(time){}
  void reset() noexcept {
    RTO_ = initial_RTO_ms_;
    is_start_ = true;
  }

  bool     is_start()const noexcept { return is_start_;}
  void     time_start() noexcept { is_start_ = true; }
  void     time_stop() noexcept { is_start_ = false; }
  bool     is_expire()const noexcept { return RTO_ <= 0; }
  void     tick(uint64_t time) noexcept { RTO_ -= time; }
  uint64_t get_RTO() const {return RTO_; }
  void     double_RTO() noexcept { RTO_ *= 2; }

private:
  const uint64_t   initial_RTO_ms_;
  uint64_t         RTO_{0};
  bool             is_start_{false};
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
private:
  using MsgQueue =  std::queue<TCPSenderMessage>;

  Wrap32    isn_;
  uint64_t  initial_RTO_ms_;
  bool      SYN_{false};
  bool      FIN_{false};
  uint64_t  receive_seqno_{};         // 已经确认的数据下标
  uint64_t  next_seqno_{};            // 下一个发送的数据下标
  uint64_t  retransmission_count{};  // 重传的message数量
  uint16_t  windows_size_{};          // 窗口大小
  MsgQueue  send_queue_{};            // 待发送队列
  MsgQueue  outstanding_queue_{};     // 已发送待确认的队列
};
