#include "action.h"
#include "auth_delegate.h"
#include "consent_delegate.h"
#include "profile_observer.h"
#include "file_handler_observer.h"

#include "mip/mip_configuration.h"
#include "mip/file/labeling_options.h"
#include "mip/file/protection_settings.h"

#include <future>
#include <iostream>

using std::cout;
using std::endl;
using std::make_shared;
using std::shared_ptr;
using std::string;

Action::Action(const string& appId,
               const string& appName,
               const string& appVersion,
               const string& userName,
               const string& tenantId,
               const string& clientSecret)
    : mAppId(appId), mUserName(userName)
{
    // 1. Create MipContext
    mip::ApplicationInfo appInfo{appId, appName, appVersion};

    auto mipConfiguration = make_shared<mip::MipConfiguration>(
        appInfo,
        "mip_data",
        mip::LogLevel::Trace,
        false);

    mMipContext = mip::MipContext::Create(mipConfiguration);

    // 2. Load FileProfile
    auto profileObserver = make_shared<ProfileObserver>();
    auto consentDelegate = make_shared<ConsentDelegateImpl>();

    mip::FileProfile::Settings profileSettings(
        mMipContext,
        mip::CacheStorageType::OnDisk,
        consentDelegate,
        profileObserver);

    auto profilePromise = make_shared<std::promise<shared_ptr<mip::FileProfile>>>();
    auto profileFuture = profilePromise->get_future();

    mip::FileProfile::LoadAsync(profileSettings, profilePromise);
    mProfile = profileFuture.get();

    // 3. Add FileEngine for the specified user
    shared_ptr<AuthDelegateImpl> authDelegate;
    if (!tenantId.empty() && !clientSecret.empty()) {
        authDelegate = make_shared<AuthDelegateImpl>(appId, tenantId, clientSecret);
    } else {
        authDelegate = make_shared<AuthDelegateImpl>(appId);
    }

    mip::FileEngine::Settings engineSettings(
        mip::Identity(userName),
        authDelegate,
        "",
        "en-US",
        false);

    engineSettings.SetEngineId(userName);

    auto enginePromise = make_shared<std::promise<shared_ptr<mip::FileEngine>>>();
    auto engineFuture = enginePromise->get_future();

    mProfile->AddEngineAsync(engineSettings, enginePromise);
    mEngine = engineFuture.get();

    cout << "MIP SDK initialized successfully for user: " << userName << endl;
}

Action::~Action()
{
    mEngine = nullptr;
    mProfile = nullptr;
    if (mMipContext) {
        mMipContext->ShutDown();
        mMipContext = nullptr;
    }
}

void Action::ListLabels()
{
    auto labels = mEngine->ListSensitivityLabels();

    cout << "\nSensitivity labels for your organization:\n";
    cout << "==========================================\n";

    for (const auto& label : labels) {
        cout << label->GetName() << " : " << label->GetId() << endl;
        for (const auto& child : label->GetChildren()) {
            cout << "  -> " << child->GetName() << " : " << child->GetId() << endl;
        }
    }
    cout << endl;
}

std::shared_ptr<mip::FileHandler> Action::CreateFileHandler(const string& filePath)
{
    auto handlerPromise = make_shared<std::promise<shared_ptr<mip::FileHandler>>>();
    auto handlerFuture = handlerPromise->get_future();

    mEngine->CreateFileHandlerAsync(
        filePath,
        filePath,
        true,
        make_shared<FileHandlerObserver>(),
        handlerPromise);

    return handlerFuture.get();
}

void Action::SetLabel(const string& inputFilePath,
                      const string& outputFilePath,
                      const string& labelId)
{
    // Create handler for input file
    auto handler = CreateFileHandler(inputFilePath);

    // Apply label
    cout << "Applying label " << labelId << " to " << inputFilePath << endl;

    mip::LabelingOptions labelingOptions(mip::AssignmentMethod::PRIVILEGED);
    handler->SetLabel(mEngine->GetLabelById(labelId), labelingOptions, mip::ProtectionSettings());

    // Commit changes to output file
    auto commitPromise = make_shared<std::promise<bool>>();
    auto commitFuture = commitPromise->get_future();

    handler->CommitAsync(outputFilePath, commitPromise);

    if (commitFuture.get()) {
        cout << "Label committed to file: " << outputFilePath << endl;
    } else {
        std::cerr << "Failed to label: " << outputFilePath << endl;
    }
}

void Action::GetLabel(const string& filePath)
{
    auto handler = CreateFileHandler(filePath);

    cout << "Reading label from: " << filePath << endl;

    auto contentLabel = handler->GetLabel();
    if (contentLabel != nullptr) {
        auto label = contentLabel->GetLabel();
        cout << "  Name : " << label->GetName() << endl;
        cout << "  Id   : " << label->GetId() << endl;
    } else {
        cout << "  No label applied to this file." << endl;
    }
}

void Action::RemoveLabel(const string& inputFilePath,
                         const string& outputFilePath)
{
    auto handler = CreateFileHandler(inputFilePath);

    // Check if a label exists
    auto contentLabel = handler->GetLabel();
    if (contentLabel == nullptr) {
        cout << "No label on " << inputFilePath << " — nothing to remove." << endl;
        return;
    }

    auto label = contentLabel->GetLabel();
    cout << "Removing label '" << label->GetName() << "' from " << inputFilePath << endl;

    mip::LabelingOptions labelingOptions(mip::AssignmentMethod::PRIVILEGED);
    labelingOptions.SetDowngradeJustification(true, "Label removed via MIP SDK CLI tool");
    handler->DeleteLabel(labelingOptions);

    auto commitPromise = make_shared<std::promise<bool>>();
    auto commitFuture = commitPromise->get_future();

    handler->CommitAsync(outputFilePath, commitPromise);

    if (commitFuture.get()) {
        cout << "Label removed. Output: " << outputFilePath << endl;
    } else {
        std::cerr << "Failed to remove label from: " << outputFilePath << endl;
    }
}
