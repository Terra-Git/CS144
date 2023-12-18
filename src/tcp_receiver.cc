#include "tcp_receiver.hh"
#include <iostream>

using namespace std;

/**
 * 保存初始 seqno 
 * 数据的第一个字节将具有 ISN + 1（ mod 2 >> 32）的序列号，初始化时需要＋1 
 * 逻辑开始和结束各占用一个序列号：除了确保接收所有数据字节外，TCP还确保流的开始和结束可靠接收
 * 绝对序列号和流索引之间进行转换相对容易——只需加或减一 
*/
void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if(message.SYN){SYN = true; ISN = message.seqno;}
  if(!SYN) {return ;}
  reassembler.insert((message.seqno).unwrap(ISN, reassembler.bytes_pending()) + message.SYN - 1, message.payload,message.FIN, inbound_stream);
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  std::optional<Wrap32> ackno {};
  if(SYN){ ackno = Wrap32::wrap(SYN + inbound_stream.is_closed() + inbound_stream.bytes_pushed(), ISN); }
  return { ackno, (uint16_t)std::min(inbound_stream.available_capacity(), (uint64_t)UINT16_MAX) };
}
