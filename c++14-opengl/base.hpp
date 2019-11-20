#ifndef INCLUDED_BASE_HPP
#define INCLUDED_BASE_HPP

#include "defines.hpp"

namespace nekolib {
  // 他のクラスを実装する際に役に立つ基底クラス
  namespace base {
    // 参照カウント持ち基底
    class Refered {
    private:
      int count_;
    protected:
      Refered() : count_(1) {}
      ~Refered() {}
    public:
      void refer() { ++count_; }
      void release() { --count_; }
      int count() const { return count_; }
    };
  }
}

#endif // INCLUDED_BASE_HPP
