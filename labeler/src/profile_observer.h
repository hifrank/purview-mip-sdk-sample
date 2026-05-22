#pragma once

#include <memory>
#include "mip/file/file_profile.h"

class ProfileObserver final : public mip::FileProfile::Observer {
public:
    ProfileObserver() = default;

    void OnLoadSuccess(
        const std::shared_ptr<mip::FileProfile>& profile,
        const std::shared_ptr<void>& context) override;

    void OnLoadFailure(
        const std::exception_ptr& error,
        const std::shared_ptr<void>& context) override;

    void OnAddEngineSuccess(
        const std::shared_ptr<mip::FileEngine>& engine,
        const std::shared_ptr<void>& context) override;

    void OnAddEngineFailure(
        const std::exception_ptr& error,
        const std::shared_ptr<void>& context) override;
};
