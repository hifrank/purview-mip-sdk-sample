#pragma once

#include <string>
#include "mip/common_types.h"

class AuthDelegateImpl final : public mip::AuthDelegate {
public:
    AuthDelegateImpl() = delete;

    // Interactive mode: prompts user for token on stdin
    explicit AuthDelegateImpl(const std::string& appId)
        : mAppId(appId) {}

    // Client credentials mode: acquires token automatically via OAuth2
    AuthDelegateImpl(const std::string& appId,
                     const std::string& tenantId,
                     const std::string& clientSecret)
        : mAppId(appId), mTenantId(tenantId), mClientSecret(clientSecret) {}

    bool AcquireOAuth2Token(
        const mip::Identity& identity,
        const OAuth2Challenge& challenge,
        OAuth2Token& token) override;

private:
    bool AcquireTokenClientCredentials(const std::string& authority,
                                       const std::string& resource,
                                       std::string& outToken);

    std::string mAppId;
    std::string mTenantId;
    std::string mClientSecret;
    std::string mToken;
    std::string mAuthority;
    std::string mResource;
};
