#include "wrapping_integers.hh"

#include <cmath>
#include <iostream>

using namespace std;

// 尽量避免隐式的类型转换
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) noexcept
{
  return zero_point + static_cast<uint32_t>( n % static_cast<uint64_t>( 1UL << 32 ) );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const noexcept
{
  // check_point 相对 zero_point 的位置
  Wrap32 check = zero_point + static_cast<uint32_t>(checkpoint % ( 1UL<<32 ));
  // 区分两种情况，在左边或者右边
  uint64_t left{},right{};
  // check 在left左边
  if( check.raw_value_ < raw_value_) {
    left = (uint64_t)(check.raw_value_ + (uint32_t)( 1UL<<32 ) - raw_value_);
    right = (uint64_t)(raw_value_ - check.raw_value_);
  }else {
    left = (uint64_t)(check.raw_value_ - raw_value_);
    right = (uint64_t)(raw_value_ + (uint32_t)( 1UL<<32 ) - check.raw_value_);
  }
  return (left < right && checkpoint >= left) ? checkpoint - left : checkpoint + right;
}
