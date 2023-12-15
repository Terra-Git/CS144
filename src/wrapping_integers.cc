#include "wrapping_integers.hh"

#include <cmath>
#include <iostream>

using namespace std;

// 尽量避免隐式的类型转换
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) noexcept
{
  return zero_point + static_cast<uint32_t>( n % static_cast<uint64_t>( 1UL << 32 ) );
}

/**
 * 先计算 check_point 相对 zero_point 的位置
 * left：raw + 多少可以到 check，此时假定 raw 在 check 左边。
 * right：check + 多少可以到 raw，此时假定 raw 在 check 右边。
 * 比较左右距离，小的那个是最近距离
 * 同时需要保证 check_point 比 left 大的情况，raw 才能在 check_point 的左边
*/
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const noexcept
{
  Wrap32 check = zero_point + (uint32_t)(checkpoint % ( 1UL<<32 ));
  uint64_t left = (uint64_t)(check.raw_value_ + (uint32_t)( 1UL<<32 ) - raw_value_) % (uint64_t)( 1UL<<32 );
  uint64_t right = (uint64_t)(raw_value_ + (uint32_t)( 1UL<<32 ) - check.raw_value_) % (uint64_t)( 1UL<<32 );
  return (left < right && checkpoint >= left) ? checkpoint - left : checkpoint + right;
}
