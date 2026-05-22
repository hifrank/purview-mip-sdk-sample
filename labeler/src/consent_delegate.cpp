#include "consent_delegate.h"
#include <iostream>

mip::Consent ConsentDelegateImpl::GetUserConsent(const std::string& url)
{
    std::cout << "SDK will connect to: " << url << std::endl;
    return mip::Consent::AcceptAlways;
}
