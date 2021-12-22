
#include "base/logging.h"

#include "demo/demo_mojo/mojom/test.mojom.h"



#include "mojo/public/cpp/system/invitation.h"

#include "mojo/public/cpp/platform/platform_channel.h"

#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr_info.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"


#pragma region Test
class TestImpl : public demo::demo_mojo::mojom::Test {
 public:
  void Hello(const std::string& caller) override {
    caller_ = caller;
    LOG(INFO) << "Test1 run: Hello " << caller;
  }

  void Hi(HiCallback callback) override {
    LOG(INFO) << "Test1 run: Hi " << caller_;
    std::move(callback).Run(caller_);
  }

 private:
  std::string caller_;
};
#pragma endregion


void MojoProducer(){
    mojo::PlatformChannel channel;


    mojo::OutgoingInvitation invitation;


}