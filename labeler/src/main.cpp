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

// ── Usage helpers ──────────────────────────────────────────────────

void printUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " [global-options] <command> [command-options]\n\n"
              << "Commands:\n"
              << "  label       Manage sensitivity labels on files\n"
              << "  policy      Evaluate label policies\n\n"
              << "Global options:\n"
              << "  --env <path>             Path to .env file (default: .env)\n"
              << "  --app-id <id>            Microsoft Entra application (client) ID\n"
              << "  --user <email>           User email for MIP engine identity\n"
              << "  --tenant-id <id>         Tenant ID (for client credentials auth)\n"
              << "  --client-secret <secret> Client secret (for client credentials auth)\n"
              << "  --help                   Show this help message\n\n"
              << "Run '" << prog << " <command> --help' for command-specific options.\n";
}

void printLabelUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " label <action> [options]\n\n"
              << "Actions:\n"
              << "  list                     List available sensitivity labels\n"
              << "  get <file>               Read and display the label on a file\n"
              << "  set [options]            Apply a sensitivity label to a file\n"
              << "  remove [options]         Remove the sensitivity label from a file\n"
              << "  decrypt <file>           Decrypt a protected file to stdout (in-memory)\n\n"
              << "Options for 'set':\n"
              << "  --input <file>           Input file path\n"
              << "  --output <file>          Output file path\n"
              << "  --label-id <id>          Sensitivity label ID to apply\n\n"
              << "Options for 'remove':\n"
              << "  --input <file>           Input file path\n"
              << "  --output <file>          Output file path\n\n"
              << "Examples:\n"
              << "  " << prog << " label list\n"
              << "  " << prog << " label get test_labeled.docx\n"
              << "  " << prog << " label set --input test.docx --output labeled.docx --label-id <guid>\n"
              << "  " << prog << " label remove --input labeled.docx --output clean.docx\n"
              << "  " << prog << " label decrypt protected.docx > decrypted.docx\n";
}

void printPolicyUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " policy <action>\n\n"
              << "Actions:\n"
              << "  list                     List label policies and their settings\n\n"
              << "Examples:\n"
              << "  " << prog << " policy list\n";
}

// ── Subcommand handlers ────────────────────────────────────────────

int cmdLabel(Action& action, int argc, char* argv[], const char* prog)
{
    if (argc < 1) {
        printLabelUsage(prog);
        return 1;
    }

    std::string labelAction = argv[0];

    if (labelAction == "list") {
        action.ListLabels();
        return 0;
    }

    if (labelAction == "get") {
        if (argc < 2) {
            std::cerr << "Error: 'label get' requires a file path.\n\n";
            printLabelUsage(prog);
            return 1;
        }
        action.GetLabel(argv[1]);
        return 0;
    }

    if (labelAction == "set") {
        std::string inputFile, outputFile, labelId;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
                inputFile = argv[++i];
            } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
                outputFile = argv[++i];
            } else if (std::strcmp(argv[i], "--label-id") == 0 && i + 1 < argc) {
                labelId = argv[++i];
            }
        }
        if (inputFile.empty() || outputFile.empty() || labelId.empty()) {
            std::cerr << "Error: 'label set' requires --input, --output, and --label-id.\n\n";
            printLabelUsage(prog);
            return 1;
        }
        action.SetLabel(inputFile, outputFile, labelId);

        std::cout << "\nVerifying label on output file...\n";
        action.GetLabel(outputFile);
        return 0;
    }

    if (labelAction == "decrypt") {
        if (argc < 2) {
            std::cerr << "Error: 'label decrypt' requires a file path.\n\n";
            printLabelUsage(prog);
            return 1;
        }
        action.DecryptToStdout(argv[1]);
        return 0;
    }

    if (labelAction == "remove") {
        std::string inputFile, outputFile;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
                inputFile = argv[++i];
            } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
                outputFile = argv[++i];
            }
        }
        if (inputFile.empty() || outputFile.empty()) {
            std::cerr << "Error: 'label remove' requires --input and --output.\n\n";
            printLabelUsage(prog);
            return 1;
        }
        action.RemoveLabel(inputFile, outputFile);
        return 0;
    }

    if (labelAction == "--help") {
        printLabelUsage(prog);
        return 0;
    }

    std::cerr << "Unknown label action: " << labelAction << "\n\n";
    printLabelUsage(prog);
    return 1;
}

int cmdPolicy(Action& action, int argc, char* argv[], const char* prog)
{
    if (argc < 1) {
        printPolicyUsage(prog);
        return 1;
    }

    std::string policyAction = argv[0];

    if (policyAction == "list") {
        action.ListPolicies();
        return 0;
    }

    if (policyAction == "--help") {
        printPolicyUsage(prog);
        return 0;
    }

    std::cerr << "Unknown policy action: " << policyAction << "\n\n";
    printPolicyUsage(prog);
    return 1;
}

// ── Main ───────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    std::string appId;
    std::string userName;
    std::string tenantId;
    std::string clientSecret;
    std::string envFile = ".env";
    std::string command;

    // Parse global options (everything before the subcommand)
    int i = 1;
    for (; i < argc; ++i) {
        if (std::strcmp(argv[i], "--app-id") == 0 && i + 1 < argc) {
            appId = argv[++i];
        } else if (std::strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            userName = argv[++i];
        } else if (std::strcmp(argv[i], "--tenant-id") == 0 && i + 1 < argc) {
            tenantId = argv[++i];
        } else if (std::strcmp(argv[i], "--client-secret") == 0 && i + 1 < argc) {
            clientSecret = argv[++i];
        } else if (std::strcmp(argv[i], "--env") == 0 && i + 1 < argc) {
            envFile = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            // First non-option argument is the subcommand
            command = argv[i];
            ++i;
            break;
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

    if (command.empty()) {
        std::cerr << "Error: No command specified.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    // Validate required args
    if (appId.empty() || userName.empty()) {
        std::cerr << "Error: --app-id and --user are required (or set CLIENT_ID and USER_EMAIL in .env).\n\n";
        printUsage(argv[0]);
        return 1;
    }

    try {
        Action action(appId, "MIP-SDK-CLI", "1.0.0", userName, tenantId, clientSecret);

        // Remaining args after the subcommand
        int subArgc = argc - i;
        char** subArgv = argv + i;

        if (command == "label") {
            return cmdLabel(action, subArgc, subArgv, argv[0]);
        } else if (command == "policy") {
            return cmdPolicy(action, subArgc, subArgv, argv[0]);
        } else {
            std::cerr << "Unknown command: " << command << "\n\n";
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
