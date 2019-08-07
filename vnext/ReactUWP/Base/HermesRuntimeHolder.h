#pragma once

#include <jsi/RuntimeHolder.h>
#include <jsi/jsi.h>

namespace react {
namespace uwp {

class HermesRuntimeHolder : public facebook::jsi::RuntimeHolderLazyInit {
 public:
  std::shared_ptr<facebook::jsi::Runtime> getRuntime() noexcept override;

 private:
  void initRuntime() noexcept;
  std::shared_ptr<facebook::jsi::Runtime> runtime_;

  std::once_flag once_flag_;
  std::thread::id own_thread_id_;
};

} // namespace uwp
} // namespace react
