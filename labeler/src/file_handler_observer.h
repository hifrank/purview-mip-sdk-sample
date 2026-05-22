#pragma once

#include <memory>
#include "mip/file/file_handler.h"

class FileHandlerObserver final : public mip::FileHandler::Observer {
public:
    FileHandlerObserver() = default;

    void OnCreateFileHandlerSuccess(
        const std::shared_ptr<mip::FileHandler>& fileHandler,
        const std::shared_ptr<void>& context) override;

    void OnCreateFileHandlerFailure(
        const std::exception_ptr& error,
        const std::shared_ptr<void>& context) override;

    void OnCommitSuccess(
        bool committed,
        const std::shared_ptr<void>& context) override;

    void OnCommitFailure(
        const std::exception_ptr& error,
        const std::shared_ptr<void>& context) override;
};
