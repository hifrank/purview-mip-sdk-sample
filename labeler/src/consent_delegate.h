#pragma once

#include "mip/common_types.h"
#include <string>

class ConsentDelegateImpl final : public mip::ConsentDelegate {
public:
    ConsentDelegateImpl() = default;
    mip::Consent GetUserConsent(const std::string& url) override;
};
