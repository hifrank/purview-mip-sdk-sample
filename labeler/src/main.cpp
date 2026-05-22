#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <map>

#include "action.h"

std::map<std::string, std::string> loadEnvFile(const std::string& path)
{
    std::map<std::string, std::string> env;
    std::ifstream file(path);
    if (!file.is_open()) return env;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Strip surrounding quotes from value
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        env[key] = value;
    }
    return env;
}

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [options]\n\n"
              << "Options:\n"
              << "  --app-id <id>            Microsoft Entra application (client) ID  [required]\n"
              << "  --user <email>           User email for MIP engine identity       [required]\n"
              << "  --env <path>             Path to .env file (default: .env)\n"
              << "  --tenant-id <id>         Tenant ID (for client credentials auth)\n"
              << "  --client-secret <secret> Client secret (for client credentials auth)\n"
              << "  --list-labels            List available sensitivity labels\n"
              << "  --input <file>           Input file path to label\n"
              << "  --output <file>          Output file path for labeled file\n"
              << "  --label-id <id>          Sensitivity label ID to apply\n"
              << "  --get-label <file>       Read and display the label on a file\n"
              << "  --remove-label <file>    Remove the sensitivity label from a file\n"
              << "  --help                   Show this help message\n\n"
              << "Examples:\n"
              << "  # List labels\n"
              << "  " << programName << " --app-id <id> --user user@tenant.com --list-labels\n\n"
              << "  # Apply a label (with client credentials)\n"
              << "  " << programName << " --app-id <id> --tenant-id <tid> --client-secret <s> \\\n"
              << "      --user user@tenant.com --input test.txt --output test_labeled.txt --label-id <label-guid>\n\n"
              << "  # Read a label\n"
              << "  " << programName << " --app-id <id> --user user@tenant.com --get-label test_labeled.txt\n\n"
              << "  # Remove a label\n"
              << "  " << programName << " --app-id <id> --tenant-id <tid> --client-secret <s> \\\n"
              << "      --user user@tenant.com --remove-label test_labeled.txt --output test_unlabeled.txt\n";
}

int main(int argc, char* argv[])
{
    std::string appId;
    std::string userName;
    std::string tenantId;
    std::string clientSecret;
    std::string inputFile;
    std::string outputFile;
    std::string labelId;
    std::string getLabelFile;
    std::string removeLabelFile;
    std::string envFile = ".env";
    bool listLabels = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--app-id") == 0 && i + 1 < argc) {
            appId = argv[++i];
        } else if (std::strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            userName = argv[++i];
        } else if (std::strcmp(argv[i], "--tenant-id") == 0 && i + 1 < argc) {
            tenantId = argv[++i];
        } else if (std::strcmp(argv[i], "--client-secret") == 0 && i + 1 < argc) {
            clientSecret = argv[++i];
        } else if (std::strcmp(argv[i], "--list-labels") == 0) {
            listLabels = true;
        } else if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            inputFile = argv[++i];
        } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (std::strcmp(argv[i], "--label-id") == 0 && i + 1 < argc) {
            labelId = argv[++i];
        } else if (std::strcmp(argv[i], "--env") == 0 && i + 1 < argc) {
            envFile = argv[++i];
        } else if (std::strcmp(argv[i], "--get-label") == 0 && i + 1 < argc) {
            getLabelFile = argv[++i];
        } else if (std::strcmp(argv[i], "--remove-label") == 0 && i + 1 < argc) {
            removeLabelFile = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // Load .env file — values are used only where CLI args were not provided
    auto envVars = loadEnvFile(envFile);
    if (appId.empty()) {
        if (envVars.count("CLIENT_ID"))      appId = envVars["CLIENT_ID"];
        else if (envVars.count("APP_ID"))    appId = envVars["APP_ID"];
    }
    if (userName.empty() && envVars.count("USER_EMAIL"))         userName = envVars["USER_EMAIL"];
    if (tenantId.empty() && envVars.count("TENANT_ID"))          tenantId = envVars["TENANT_ID"];
    if (clientSecret.empty() && envVars.count("CLIENT_SECRET"))  clientSecret = envVars["CLIENT_SECRET"];

    // Validate required args
    if (appId.empty() || userName.empty()) {
        std::cerr << "Error: --app-id and --user are required.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    bool doSetLabel = !inputFile.empty() && !outputFile.empty() && !labelId.empty();
    bool doRemoveLabel = !removeLabelFile.empty() && !outputFile.empty();

    if (!listLabels && !doSetLabel && !doRemoveLabel && getLabelFile.empty()) {
        std::cerr << "Error: Specify --list-labels, --get-label <file>, --remove-label <file> --output <file>, or --input/--output/--label-id.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    try {
        Action action(appId, "MIP-File-Labeler", "1.0.0", userName, tenantId, clientSecret);

        if (listLabels) {
            action.ListLabels();
        }

        if (doSetLabel) {
            action.SetLabel(inputFile, outputFile, labelId);

            // Verify by reading the label back
            std::cout << "\nVerifying label on output file...\n";
            action.GetLabel(outputFile);
        }

        if (doRemoveLabel) {
            action.RemoveLabel(removeLabelFile, outputFile);
        }

        if (!getLabelFile.empty()) {
            action.GetLabel(getLabelFile);
        }

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
