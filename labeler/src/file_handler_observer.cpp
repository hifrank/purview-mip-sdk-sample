#include "file_handler_observer.h"
#include <future>

using std::shared_ptr;
using std::static_pointer_cast;

void FileHandlerObserver::OnCreateFileHandlerSuccess(
    const shared_ptr<mip::FileHandler>& fileHandler,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<shared_ptr<mip::FileHandler>>>(context);
    promise->set_value(fileHandler);
}

void FileHandlerObserver::OnCreateFileHandlerFailure(
    const std::exception_ptr& error,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<shared_ptr<mip::FileHandler>>>(context);
    promise->set_exception(error);
}

void FileHandlerObserver::OnCommitSuccess(
    bool committed,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<bool>>(context);
    promise->set_value(committed);
}

void FileHandlerObserver::OnCommitFailure(
    const std::exception_ptr& error,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<bool>>(context);
    promise->set_exception(error);
}
