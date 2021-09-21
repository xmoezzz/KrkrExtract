#pragma once

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <client.grpc.pb.h>
#include <Stubs.h>
#include <Utf.Convert.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using krclient::KrCoreApi;
using krclient::CancelTaskRequest;
using krclient::CancelTaskResponse;
using krclient::EmitCommandRequest;
using krclient::EmitCommandResponse;
using krclient::PackerRequest;
using krclient::PackerResponse;
using krclient::TaskCloseWindowRequest;
using krclient::TaskCloseWindowResponse;
using krclient::TaskDumpStartRequest;
using krclient::TaskDumpStartResponse;
using krclient::UniversalDumperRequest;
using krclient::UniversalDumperResponse;
using krclient::UniversalPatchRequest;
using krclient::UniversalPatchResponse;

class CoreApiService final : public KrCoreApi::Service
{
public:
    CoreApiService(ClientStub* Client) : Stub(Client) {}
    ~CoreApiService() { Stub = nullptr; }

    grpc::Status CancelTask(
        ServerContext* Context, 
        const CancelTaskRequest* Request, 
        CancelTaskResponse* Response
    ) override 
    {
        auto Success = Stub->NotifyClientCancelTask();
        if (!Success) {
            Response->set_success(false);
            Response->set_message("failed to call Stub->NotifyClientCancelTask");
            return grpc::Status::OK;
        }

        Response->set_success(true);
        return grpc::Status::OK;
    }
    
    grpc::Status EmitCommand(
        ServerContext* Context, 
        const EmitCommandRequest* Request, 
        EmitCommandResponse* Response
    ) override 
    {
        auto Success = Stub->NotifyClientCommandEmitted(Utf8ToUtf16(Request->command()).c_str());
        if (!Success) {
            Response->set_success(false);
            Response->set_message("failed to call Stub->NotifyClientCommandEmitted");
            return grpc::Status::OK;
        }

        Response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status Packer(
        ServerContext* Context,
        const PackerRequest* Request,
        PackerResponse* Response
    ) override
    {
        auto Success = Stub->NotifyClientPackerChecked(
            Utf8ToUtf16(Request->basedir()).c_str(),
            Utf8ToUtf16(Request->originalarchivename()).c_str(),
            Utf8ToUtf16(Request->outputarchivename()).c_str()
        );
        if (!Success) {
            Response->set_success(false);
            Response->set_message("failed to call Stub->NotifyClientPackerChecked");
            return grpc::Status::OK;
        }

        Response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status TaskCloseWindow(
        ServerContext* Context,
        const TaskCloseWindowRequest* Request,
        TaskCloseWindowResponse* Response
    ) override
    {
        // push event
        auto Success = Stub->NotifyClientTaskCloseWindow();
        if (!Success) {
            return grpc::Status::OK;
        }

        return grpc::Status::OK;
    }

    grpc::Status TaskDumpStart(
        ServerContext* Context,
        const TaskDumpStartRequest* Request,
        TaskDumpStartResponse* Response
    ) override
    {
        auto Success = Stub->NotifyClientTaskDumpStart(
            static_cast<KrkrPsbMode>(Request->psbmode()),
            static_cast<KrkrTextMode>(Request->textmode()),
            static_cast<KrkrPngMode>(Request->pngmode()),
            static_cast<KrkrTjs2Mode>(Request->tjs2mode()),
            static_cast<KrkrTlgMode>(Request->tlgmode()),
            static_cast<KrkrAmvMode>(Request->amvmode()),
            static_cast<KrkrPbdMode>(Request->pdbmode()),
            Utf8ToUtf16(Request->file()).c_str()
        );
        if (!Success) {
            Response->set_success(false);
            Response->set_message("failed to call Stub->TaskDumpStart");
            return grpc::Status::OK;
        }

        Response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status UniversalDumper(
        ServerContext* Context,
        const UniversalDumperRequest* Request,
        UniversalDumperResponse* Response
    ) override
    {
        auto Success = Stub->NotifyClientUniversalDumperModeChecked(
            static_cast<KrkrPsbMode>(Request->psbmode()),
            static_cast<KrkrTextMode>(Request->textmode()),
            static_cast<KrkrPngMode>(Request->pngmode()),
            static_cast<KrkrTjs2Mode>(Request->tjs2mode()),
            static_cast<KrkrTlgMode>(Request->tlgmode()),
            static_cast<KrkrAmvMode>(Request->amvmode()),
            static_cast<KrkrPbdMode>(Request->pdbmode())
        );

        if (!Success) {
            Response->set_success(false);
            Response->set_message("failed to call Stub->UniversalDumper");
            return grpc::Status::OK;
        }

        Response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status UniversalPatch(
        ServerContext* context,
        const UniversalPatchRequest* Request,
        UniversalPatchResponse* Response
    ) override
    {
        auto Success = Stub->NotifyClientUniversalPatchMakeChecked(
            Request->protect(),
            Request->icon()
        );

        if (!Success) {
            Response->set_success(false);
            Response->set_message("failed to call Stub->UniversalPatch");
            return grpc::Status::OK;
        }

        Response->set_success(true);
        return grpc::Status::OK;
    }

private:
    ClientStub* Stub = nullptr;
};


