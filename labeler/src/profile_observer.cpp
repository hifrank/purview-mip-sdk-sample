#include "profile_observer.h"
#include <future>

using std::shared_ptr;
using std::static_pointer_cast;

void ProfileObserver::OnLoadSuccess(
    const shared_ptr<mip::FileProfile>& profile,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<shared_ptr<mip::FileProfile>>>(context);
    promise->set_value(profile);
}

void ProfileObserver::OnLoadFailure(
    const std::exception_ptr& error,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<shared_ptr<mip::FileProfile>>>(context);
    promise->set_exception(error);
}

void ProfileObserver::OnAddEngineSuccess(
    const shared_ptr<mip::FileEngine>& engine,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<shared_ptr<mip::FileEngine>>>(context);
    promise->set_value(engine);
}

void ProfileObserver::OnAddEngineFailure(
    const std::exception_ptr& error,
    const shared_ptr<void>& context)
{
    auto promise = static_pointer_cast<std::promise<shared_ptr<mip::FileEngine>>>(context);
    promise->set_exception(error);
}
