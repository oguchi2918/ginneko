// これはクラスを書く人向けで、ユーザがインクルードすることはない。
// 参照カウント型のクラスでは必ず持っている関数というものがあり、
// これを書くのを少しだけ楽にする。
// ActualTypeが本当の型の名前だとしてクラス実装ファイルに以下の二行を追加する。
// #define TYPE ActualType
// #include "rctype_template.hpp"
// クラス宣言ファイルには別途宣言を手で追加してやる必要がある。
// 面倒だが継承するより10%位速い。

TYPE::TYPE() noexcept : impl_(nullptr) {
}

TYPE::~TYPE() noexcept {
  release();
}

TYPE::TYPE(const TYPE& o) noexcept : impl_(o.impl_) {
  if (impl_) {
    impl_->refer();
  }
}

TYPE::TYPE(TYPE&& o) noexcept : impl_(o.impl_) {
  o.impl_ = nullptr;
}

void TYPE::release() noexcept {
  if (impl_) {
    impl_->release();
    if (impl_->count() == 0) {
      SAFE_DELETE(impl_);
    }
  }
  impl_ = nullptr;
}

TYPE& TYPE::operator=(const TYPE& o) noexcept {
  release();
  impl_ = o.impl_;
  if (impl_) {
    impl_->refer();
  }
  return *this;
}

TYPE& TYPE::operator=(TYPE&& o) noexcept {
  release();
  impl_ = o.impl_; o.impl_ = nullptr;
  return *this;
}

bool TYPE::operator==(const TYPE& o) const noexcept {
  return impl_ == o.impl_;
}

bool TYPE::operator!=(const TYPE& o) const noexcept {
  return impl_ != o.impl_;
}

TYPE::operator bool() const noexcept {
  return impl_ != nullptr;
}

#undef TYPE
			    
