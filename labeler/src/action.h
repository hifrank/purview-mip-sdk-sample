#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mip/mip_context.h"
#include "mip/file/file_profile.h"
#include "mip/file/file_engine.h"
#include "mip/file/file_handler.h"

class Action {
public:
    Action(const std::string& appId,
           const std::string& appName,
           const std::string& appVersion,
           const std::string& userName,
           const std::string& tenantId = "",
           const std::string& clientSecret = "");

    ~Action();

    void ListLabels();

    void ListPolicies();

    void SetLabel(const std::string& inputFilePath,
                  const std::string& outputFilePath,
                  const std::string& labelId);

    void GetLabel(const std::string& filePath);

    void RemoveLabel(const std::string& inputFilePath,
                     const std::string& outputFilePath);

    /// Decrypt a protected file and write raw bytes to stdout (in-memory only).
    void DecryptToStdout(const std::string& filePath);

private:
    std::shared_ptr<mip::FileHandler> CreateFileHandler(const std::string& filePath);

    std::shared_ptr<mip::MipContext> mMipContext;
    std::shared_ptr<mip::FileProfile> mProfile;
    std::shared_ptr<mip::FileEngine> mEngine;
    std::string mAppId;
    std::string mUserName;
};
