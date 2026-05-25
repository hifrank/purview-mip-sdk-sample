#include "auth_delegate.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <curl/curl.h>

using std::cout;
using std::cin;
using std::string;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

bool AuthDelegateImpl::AcquireTokenClientCredentials(
    const string& authority, const string& resource, string& outToken)
{
    // Build token endpoint. The MIP SDK challenge may give authority as
    // "https://login.microsoftonline.com/common" — replace with actual tenant.
    string tokenUrl = authority;
    if (!mTenantId.empty()) {
        auto pos = tokenUrl.find("/common");
        if (pos != string::npos) {
            tokenUrl.replace(pos, 7, "/" + mTenantId);
        }
    }
    if (tokenUrl.back() != '/') tokenUrl += '/';
    tokenUrl += "oauth2/v2.0/token";

    // Build scope from resource (e.g. "https://syncservice.o365syncservice.com/.default")
    string scope = resource;
    if (scope.find("/.default") == string::npos) {
        if (scope.back() != '/') scope += '/';
        scope += ".default";
    }

    string postData = "grant_type=client_credentials"
                      "&client_id=" + mAppId +
                      "&client_secret=" + mClientSecret +
                      "&scope=" + scope;

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, tokenUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Token request failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    // Simple JSON parsing for "access_token":"..."
    auto pos = responseBody.find("\"access_token\"");
    if (pos == string::npos) {
        std::cerr << "Token response error: " << responseBody << std::endl;
        return false;
    }
    pos = responseBody.find(':', pos);
    auto start = responseBody.find('"', pos + 1) + 1;
    auto end = responseBody.find('"', start);
    outToken = responseBody.substr(start, end - start);
    return true;
}

bool AuthDelegateImpl::AcquireOAuth2Token(
    const mip::Identity& identity,
    const OAuth2Challenge& challenge,
    OAuth2Token& token)
{
    string authority = challenge.GetAuthority();
    string resource = challenge.GetResource();

    // Reuse cached token if same authority/resource
    if (!mToken.empty() && authority == mAuthority && resource == mResource) {
        token.SetAccessToken(mToken);
        return true;
    }

    // Client credentials mode
    if (!mClientSecret.empty() && !mTenantId.empty()) {
        string newToken;
        std::cerr << "  Auth challenge:\n"
             << "    Authority: " << authority << "\n"
             << "    Resource : " << resource << "\n";
        if (AcquireTokenClientCredentials(authority, resource, newToken)) {
            mToken = newToken;
            mAuthority = authority;
            mResource = resource;
            token.SetAccessToken(mToken);
            std::cerr << "  Token acquired via client credentials.\n";
            return true;
        }
        std::cerr << "Client credentials token acquisition failed.\n";
        return false;
    }

    // Interactive fallback: prompt for token on stdin
    cout << "\n========================================\n";
    cout << "Token required. Generate an access token with:\n";
    cout << "  Authority : " << authority << "\n";
    cout << "  Resource  : " << resource << "\n";
    cout << "  User      : " << identity.GetEmail() << "\n";
    cout << "  App ID    : " << mAppId << "\n";
    cout << "========================================\n";
    cout << "Enter access token: ";
    cin >> mToken;
    mAuthority = authority;
    mResource = resource;

    token.SetAccessToken(mToken);
    return true;
}
