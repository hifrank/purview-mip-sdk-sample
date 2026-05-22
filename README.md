# Microsoft Purview MIP SDK Sample

A two-part sample demonstrating Microsoft Information Protection (MIP) capabilities:

1. **Labeler** (C++) — Apply a sensitivity label to a `.txt` file using the MIP File SDK
2. **Scanner** (Python) — Scan file content against Purview data protection policies using the Microsoft Graph `processContent` API

The two parts are connected: the scanner reads the labeled file output from the labeler and evaluates it against configured policies.

---

## Prerequisites

| Requirement | Details |
|---|---|
| **Microsoft 365** | E3 or E5 subscription with sensitivity labels configured |
| **MIP SDK** | [Download](https://aka.ms/mipsdkbins) the macOS C++ SDK and extract it |
| **CMake** | 3.16+ (`brew install cmake`) |
| **Xcode CLI tools** | `xcode-select --install` |
| **Python** | 3.9+ with pip |
| **OpenSSL / cURL** | Typically pre-installed on macOS |

## Microsoft Entra App Registration

Register an app in [Microsoft Entra ID](https://portal.azure.com/#blade/Microsoft_AAD_IAM/ActiveDirectoryMenuBlade/RegisteredApps):

1. **Supported account types**: Accounts in this organizational directory only
2. **Redirect URI**: Public client — `http://localhost`
3. **API Permissions** (all Delegated unless noted):
   - `Azure Rights Management Services` → `user_impersonation`
   - `Microsoft Information Protection Sync Service` → `UnifiedPolicy.User.Read`
   - `Microsoft Graph` → `Content.Process.User` or `Content.Process.All` (Application)
4. **Grant admin consent** for all permissions
5. Create a **Client Secret** (for the Python scanner)

Note the **Application (client) ID**, **Directory (tenant) ID**, and **Client Secret**.

---

## Setup

```bash
# Clone the repo
git clone <this-repo-url>
cd purview-mip-sdk-sample

# Create .env from template
cp .env.example .env
# Edit .env with your values: TENANT_ID, CLIENT_ID, CLIENT_SECRET, USER_EMAIL, USER_ID, LABEL_ID
```

---

## Part 1: C++ Labeler (MIP File SDK)

### Build

The MIP SDK ships Linux x86-64 `.so` libraries, so the labeler must be built and run via Docker (even on macOS/Apple Silicon):

```bash
cd purview-mip-sdk-sample

# Build the Docker image (--platform is required on Apple Silicon Macs)
docker build --platform linux/amd64 -t mip-labeler -f labeler/Dockerfile .
```

### Usage

The labeler reads credentials from a `.env` file automatically, so you don't need to pass them on every command. Create your `.env` file first:

```bash
cp labeler/.env.sample labeler/.env
# Edit labeler/.env with your real values
```

**`.env` file format:**
```env
# Microsoft Entra App Registration
CLIENT_ID=your-client-id
TENANT_ID=your-tenant-id
CLIENT_SECRET=your-client-secret

# User identity
USER_EMAIL=user@tenant.com
```

The labeler looks for `.env` in the current directory by default. Use `--env <path>` to specify a different location. CLI arguments always override `.env` values.

Run the labeler via Docker, mounting your `.env` file and any input/output files:

```bash
# List available sensitivity labels
docker run --rm --platform linux/amd64 \
    -v "$(pwd)/labeler/.env:/app/.env:ro" \
    mip-labeler --list-labels

# Apply a sensitivity label to a text file
docker run --rm --platform linux/amd64 \
    -v "$(pwd)/labeler/.env:/app/.env:ro" \
    -v "$(pwd)/sample_files:/data" \
    mip-labeler --input /data/test_input.txt \
    --output /data/test_labeled.txt \
    --label-id <label-guid>

# Read the label from a file
docker run --rm --platform linux/amd64 \
    -v "$(pwd)/labeler/.env:/app/.env:ro" \
    -v "$(pwd)/sample_files:/data" \
    mip-labeler --get-label /data/test_labeled.txt

# Remove a label from a file
docker run --rm --platform linux/amd64 \
    -v "$(pwd)/labeler/.env:/app/.env:ro" \
    -v "$(pwd)/sample_files:/data" \
    mip-labeler --remove-label /data/test_labeled.txt \
    --output /data/test_unlabeled.txt
```

When prompted, generate an OAuth2 access token using the displayed authority/resource values. You can use the MSAL PowerShell module or any MSAL-based tool:

```powershell
# PowerShell example
$authority = "<authority-url>"
$resourceUrl = "<resource-url>"
$clientId = "<client-id>"
$result = Get-MsalToken -ClientId $clientId -Authority $authority -Scopes "$resourceUrl/.default" -Interactive
$result.AccessToken | Set-Clipboard
```

---

## Part 2: Python Scanner (Graph processContent API)

### Setup

```bash
cd scanner
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Usage

```bash
# Scan the labeled file against Purview DLP policies
python scan_content.py --file ../sample_files/test_labeled.txt --env ../.env
```

### What it does

1. Reads the file content as UTF-8 text
2. Acquires a token via MSAL client credentials flow
3. Calls `POST /users/{userId}/dataSecurityAndGovernance/processContent` with the file content
4. Displays policy actions (block, audit, etc.) returned by Purview

### Example Output

```
Reading file: ../sample_files/test_labeled.txt
  File size: 423 bytes
Acquiring access token...
  Token acquired successfully.
Sending processContent request to Microsoft Graph...

============================================================
  processContent Scan Results
  File: ../sample_files/test_labeled.txt
============================================================

  Protection Scope State: modified

  Policy Actions (1):
    - restrictAccessAction: restrictAccess -> block

  Processing Errors: None

============================================================
```

---

## End-to-End Walkthrough

```bash
# 0. Set up your credentials
cp labeler/.env.sample labeler/.env
# Edit labeler/.env with your CLIENT_ID, TENANT_ID, CLIENT_SECRET, USER_EMAIL

# 1. Build the Docker image
docker build --platform linux/amd64 -t mip-labeler -f labeler/Dockerfile .

# 2. Label the test file
docker run --rm --platform linux/amd64 \
    -v "$(pwd)/labeler/.env:/app/.env:ro" \
    -v "$(pwd)/sample_files:/data" \
    mip-labeler --input /data/test_input.txt \
    --output /data/test_labeled.txt \
    --label-id <label-guid>

# 3. Scan the labeled file with processContent
cd scanner
source .venv/bin/activate
python scan_content.py --file ../sample_files/test_labeled.txt --env ../.env
```

---

## Project Structure

```
purview-mip-sdk-sample/
├── .env.example                          # Config template
├── .gitignore
├── README.md
├── labeler/                              # C++ MIP SDK labeler
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp                      # CLI entry point
│       ├── action.h / action.cpp         # MipContext → Profile → Engine → Handler
│       ├── auth_delegate.h / .cpp        # OAuth2 token acquisition (stdin prompt)
│       ├── consent_delegate.h / .cpp     # Consent delegate (auto-accept)
│       ├── profile_observer.h / .cpp     # FileProfile async observer
│       └── file_handler_observer.h / .cpp# FileHandler async observer
├── scanner/                              # Python Graph API scanner
│   ├── requirements.txt
│   └── scan_content.py                   # processContent API caller
└── sample_files/
    └── test_input.txt                    # Sample file with sensitive data patterns
```

## References

- [MIP SDK Setup & Configuration](https://learn.microsoft.com/en-us/information-protection/develop/setup-configure-mip)
- [MIP SDK File Handler Concepts (C++)](https://learn.microsoft.com/en-us/information-protection/develop/concept-handler-file-cpp)
- [Quickstart: Set and Get a Sensitivity Label (C++)](https://learn.microsoft.com/en-us/information-protection/develop/quick-file-set-get-label-cpp)
- [Graph API: processContent](https://learn.microsoft.com/en-us/graph/api/userdatasecurityandgovernance-processcontent?view=graph-rest-1.0)
- [Azure-Samples/MipSDK-File-Cpp-Basic](https://github.com/Azure-Samples/MipSDK-File-Cpp-Basic)
