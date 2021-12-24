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
private:
    std::string caller_;

public:
    explicit TestImpl(mojo::PendingReceiver<Test> receiver):receiver_(this,std::move(receiver)){}

    void Hello(const std::string& caller) override {
        caller_ = caller;
        LOG(INFO) << "TestImpl run: Hello From " << caller << " " << getpid();;
    }
    void Hi(HiCallback callback) override {
        std::string txt = "TestImpl run: TestImpl Hi To ";

        LOG(INFO) << txt << caller_;
//        std::move(callback).Run(txt.append(caller_));
        std::move(callback).Run(txt);
        LOG(INFO) << "std::move(callback).Run(txt); " << getpid();;

    }

/* ------------------------------------------- */

private:
    mojo::Receiver<Test> receiver_;

public:
    void SendHandle(mojo::ScopedMessagePipeHandle pipe_handle) override {
        LOG(INFO) <<"SendHandle: " << pipe_handle->value()<< "  "<<getpid();

        //if(!pipe->QuerySignalsState().readable()){
            // msg form client
            MojoResult result;
            result = mojo::Wait(pipe_handle.get(),MOJO_HANDLE_SIGNAL_READABLE);
            std::vector<uint8_t> data;
            result = mojo::ReadMessageRaw(pipe_handle.get(),&data,nullptr,MOJO_READ_MESSAGE_FLAG_NONE);
            LOG(INFO) << "MSG "<< (char*)&data[0];


            // to client
            const char KMessage[] = "HI";
            mojo::WriteMessageRaw(pipe_handle.get(), KMessage,sizeof(KMessage),nullptr,0,MOJO_WRITE_MESSAGE_FLAG_NONE);


      //  }
        LOG(INFO) <<"SendHandle End: " << pipe_handle->value()<< "  "<<getpid();

    }
};
#pragma endregion


void OnBack(const std::string& msg){
    LOG(INFO) << "test response: Hi " << msg << "---" << getpid();
}

void MojoProducer(){
    mojo::PlatformChannel channel;


    mojo::OutgoingInvitation invitation;

    mojo::ScopedMessagePipeHandle pipe = invitation.AttachMessagePipe("test");
    LOG(INFO) << "MojoProducer pipe value : " << pipe->value() << "  " <<getpid();

    base::LaunchOptions options;
    base::CommandLine command_line(base::CommandLine::ForCurrentProcess()->GetProgram());
    channel.PrepareToPassRemoteEndpoint(&options, &command_line);
    base::Process child_process = base::LaunchProcess(command_line, options);
    channel.RemoteProcessLaunchAttempted();
    mojo::OutgoingInvitation::Send(std::move(invitation),child_process.Handle(),channel.TakeLocalEndpoint(),base::BindRepeating([](const std::string& error){LOG(ERROR) << error;}));



    mojo::InterfacePtr<Test> test;
    test.Bind(mojo::InterfacePtrInfo<Test>(std::move(pipe), 0));
    LOG(INFO) << "Test call: SendHandle  " << getpid();
    test->Hello("MojoProducer");

    test->Hi(base::BindOnce(&OnBack));

    mojo::MessagePipe msgPipe;
    LOG(INFO) << "pipe0: "<<msgPipe.handle0->value() <<" pipe1: "<<msgPipe.handle1->value()<<"  " << getpid();
    test->SendHandle(std::move(msgPipe.handle1));

    // send msg to remote
    const char KMessage[] = "Hello";
    mojo::WriteMessageRaw(msgPipe.handle0.get(), KMessage,sizeof(KMessage),nullptr,0,MOJO_WRITE_MESSAGE_FLAG_NONE);

    // get msg from remote
    MojoResult result;
    result = mojo::Wait(msgPipe.handle0.get(),MOJO_HANDLE_SIGNAL_READABLE);
    std::vector<uint8_t> data;
    result = mojo::ReadMessageRaw(msgPipe.handle0.get(),&data,nullptr,MOJO_READ_MESSAGE_FLAG_NONE);
    LOG(INFO) << "MSG handle0 "<< (char*)&data[0];

    LOG(INFO) << "test->Hi(base::BindOnce([](const std::string& msg) end" << getpid();
}


void MojoConsumer(){
    mojo::IncomingInvitation invitation = mojo::IncomingInvitation::Accept(mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(*base::CommandLine::ForCurrentProcess()));

    mojo::ScopedMessagePipeHandle pipe = invitation.ExtractMessagePipe("test");
    LOG(INFO) << "MojoConsumer pipe value : " << pipe->value()<< " " << getpid();

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

    LOG(INFO) << "running... "<< getpid();
    run_loop.Run();
    return 0;

}