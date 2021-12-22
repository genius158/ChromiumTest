#include "base/command_line.h"
#include "base/logging.h"
#include "base/task/single_thread_task_executor.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/c/system/buffer.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "mojo/public/cpp/system/wait.h"

#include "demo/mojotest/mojom/test.mojom.h"

#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr_info.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"

using namespace demo::mojotest::mojom;


#pragma region Test
class TestImpl : public Test {
// public:
//  void Hello(const std::string& caller) override {
//    caller_ = caller;
//    LOG(INFO) << "Test1 run: Hello " << caller;
//  }
//  void Hi(HiCallback callback) override {
//    LOG(INFO) << "Test1 run: Hi " << caller_;
//    std::move(callback).Run(caller_);
//  }
// private:
//  std::string caller_;

/* ------------------------------------------- */
    private:
    mojo::Receiver<Test> receiver_;

    public:
        explicit TestImpl(mojo::PendingReceiver<Test> receiver):receiver_(this,std::move(receiver)){}

        void SendHandle(mojo::ScopedMessagePipeHandle pipe_handle) override {
            LOG(INFO) <<"SendHandle: " << pipe_handle->value();
        }


};
#pragma endregion


void MojoProducer(){
    mojo::PlatformChannel channel;


    mojo::OutgoingInvitation invitation;

    mojo::ScopedMessagePipeHandle pipe = invitation.AttachMessagePipe("test");
    LOG(INFO) << "pipe value : " << pipe->value();

    base::LaunchOptions options;
    base::CommandLine command_line(base::CommandLine::ForCurrentProcess()->GetProgram());
    channel.PrepareToPassRemoteEndpoint(&options, &command_line);
    base::Process child_process = base::LaunchProcess(command_line, options);
    channel.RemoteProcessLaunchAttempted();
    mojo::OutgoingInvitation::Send(std::move(invitation),child_process.Handle(),channel.TakeLocalEndpoint(),base::BindRepeating([](const std::string& error){LOG(ERROR) << error;}));



    mojo::InterfacePtr<Test> test;
    test.Bind(mojo::InterfacePtrInfo<Test>(std::move(pipe), 0));
    test->SendHandle(std::move(pipe));
    LOG(INFO) << "Test call: SendHandle";

}

void MojoConsumer(){
    mojo::IncomingInvitation invitation = mojo::IncomingInvitation::Accept(mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(*base::CommandLine::ForCurrentProcess()));

    mojo::ScopedMessagePipeHandle pipe = invitation.ExtractMessagePipe("test");
    LOG(INFO) << "pipe value : " << pipe->value();

    new TestImpl(mojo::PendingReceiver<Test>(std::move(pipe)));

}

int main(int argc,char** argv){
    base::CommandLine::Init(argc,argv);

    base::SingleThreadTaskExecutor main_task_executor;
    base::RunLoop run_loop;

    mojo::core::Init();
    base::Thread ipc_thread("ipc");
    ipc_thread.StartWithOptions(base::Thread::Options(base::MessagePumpType::IO,0));
    mojo::core::ScopedIPCSupport ipc_support(ipc_thread.task_runner(),mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

    if(argc < 2){
        MojoProducer();
    }else{
        MojoConsumer();
    }

    LOG(INFO) << "running...";
    run_loop.Run();
    return 0;

}