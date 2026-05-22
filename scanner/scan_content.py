#!/usr/bin/env python3
"""
Scan file content against Microsoft Purview data protection policies
using the Microsoft Graph processContent API.

Usage:
    python scan_content.py --file <path-to-file>
    python scan_content.py --file <path-to-file> --env ../.env
"""

import argparse
import base64
import json
import os
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path

import msal
import requests
from dotenv import load_dotenv


def acquire_token(tenant_id: str, client_id: str, client_secret: str) -> str:
    """Acquire an access token using MSAL client credentials flow."""
    authority = f"https://login.microsoftonline.com/{tenant_id}"

    try:
        app = msal.ConfidentialClientApplication(
            client_id,
            authority=authority,
            client_credential=client_secret,
        )
    except ValueError as e:
        print(f"Invalid tenant or authority configuration: {e}", file=sys.stderr)
        print("Check that TENANT_ID in your .env file is a valid GUID or domain.", file=sys.stderr)
        sys.exit(1)

    scopes = ["https://graph.microsoft.com/.default"]
    result = app.acquire_token_for_client(scopes=scopes)

    if "access_token" not in result:
        error = result.get("error_description", result.get("error", "Unknown error"))
        print(f"Failed to acquire token: {error}", file=sys.stderr)
        sys.exit(1)

    return result["access_token"]


# File extensions that should be sent as binary (base64-encoded)
BINARY_EXTENSIONS = {
    ".docx", ".xlsx", ".pptx", ".pdf",
    ".doc", ".xls", ".ppt",
    ".zip", ".pfile",
}


def is_binary_file(file_path: str) -> bool:
    """Check if a file should be sent as binary content."""
    return Path(file_path).suffix.lower() in BINARY_EXTENSIONS


def read_file_content(file_path: str) -> str:
    """Read file content as UTF-8 text."""
    path = Path(file_path)
    if not path.exists():
        print(f"File not found: {file_path}", file=sys.stderr)
        sys.exit(1)
    return path.read_text(encoding="utf-8")


def read_file_binary(file_path: str) -> bytes:
    """Read file content as raw bytes."""
    path = Path(file_path)
    if not path.exists():
        print(f"File not found: {file_path}", file=sys.stderr)
        sys.exit(1)
    return path.read_bytes()


def build_request_body(
    file_path: str,
    client_id: str,
    user_id: str,
    file_content: str = None,
    file_bytes: bytes = None,
) -> dict:
    """Build the processContent request body."""
    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S")
    file_name = Path(file_path).name
    correlation_id = str(uuid.uuid4())
    entry_id = str(uuid.uuid4())

    if file_bytes is not None:
        # Per MS docs Example 4: file content uses textContent with base64 data
        content_block = {
            "@odata.type": "microsoft.graph.textContent",
            "data": base64.b64encode(file_bytes).decode("ascii"),
        }
        file_length = len(file_bytes)
        activity = "uploadFile"
    else:
        content_block = {
            "@odata.type": "microsoft.graph.textContent",
            "data": file_content,
        }
        file_length = len(file_content.encode("utf-8"))
        activity = "uploadText"

    entry = {
        "@odata.type": "microsoft.graph.processFileMetadata",
        "identifier": entry_id,
        "content": content_block,
        "name": file_name,
        "correlationId": correlation_id,
        "createdDateTime": now,
        "modifiedDateTime": now,
        "length": file_length,
        "isTruncated": False,
        "ownerId": user_id,
    }

    return {
        "contentToProcess": {
            "contentEntries": [entry],
            "activityMetadata": {
                "activity": activity,
            },
            "deviceMetadata": {
                "deviceType": "Unmanaged",
                "operatingSystemSpecifications": {
                    "operatingSystemPlatform": "macOS",
                    "operatingSystemVersion": "15.0",
                },
                "ipAddress": "127.0.0.1",
            },
            "protectedAppMetadata": {
                "name": "MIP-ProcessContent-Scanner",
                "version": "1.0",
                "applicationLocation": {
                    "@odata.type": "microsoft.graph.policyLocationApplication",
                    "value": client_id,
                },
            },
            "integratedAppMetadata": {
                "name": "MIP-ProcessContent-Scanner",
                "version": "1.0",
            },
        }
    }


def process_content(
    access_token: str,
    user_id: str,
    request_body: dict,
) -> dict:
    """Call the processContent Graph API."""
    url = f"https://graph.microsoft.com/v1.0/users/{user_id}/dataSecurityAndGovernance/processContent"

    headers = {
        "Authorization": f"Bearer {access_token}",
        "Content-Type": "application/json",
        "Client-Request-Id": str(uuid.uuid4()),
    }

    response = requests.post(url, headers=headers, json=request_body, timeout=30)

    if response.status_code == 200:
        result = response.json()
        print(f"\n  [DEBUG] Full API response:\n{json.dumps(result, indent=2)}\n")
        return result
    elif response.status_code == 202:
        print("Request accepted (202) — processing asynchronously.")
        return {"status": "accepted"}
    elif response.status_code == 204:
        print("No content (204) — no policy actions apply.")
        return {"status": "no_content"}
    else:
        print(f"API error {response.status_code}:", file=sys.stderr)
        try:
            error_body = response.json()
            print(json.dumps(error_body, indent=2), file=sys.stderr)
        except ValueError:
            print(response.text, file=sys.stderr)
        sys.exit(1)


def print_results(result: dict, file_path: str):
    """Print a human-readable summary of the processContent response."""
    print(f"\n{'='*60}")
    print(f"  processContent Scan Results")
    print(f"  File: {file_path}")
    print(f"{'='*60}")

    scope_state = result.get("protectionScopeState", "unknown")
    print(f"\n  Protection Scope State: {scope_state}")

    actions = result.get("policyActions", [])
    if actions:
        print(f"\n  Policy Actions ({len(actions)}):")
        for action in actions:
            action_type = action.get("@odata.type", "unknown").split(".")[-1]
            action_name = action.get("action", "unknown")
            restriction = action.get("restrictionAction", "")
            detail = f" -> {restriction}" if restriction else ""
            print(f"    - {action_type}: {action_name}{detail}")
    else:
        print("\n  Policy Actions: None (no policy violations detected)")

    errors = result.get("processingErrors", [])
    if errors:
        print(f"\n  Processing Errors ({len(errors)}):")
        for err in errors:
            print(f"    - {err}")
    else:
        print("  Processing Errors: None")

    print(f"\n{'='*60}\n")


def main():
    parser = argparse.ArgumentParser(
        description="Scan file content against Purview data protection policies via Graph processContent API."
    )
    parser.add_argument("--file", required=True, help="Path to the file to scan")
    parser.add_argument("--env", default="../.env", help="Path to .env file (default: ../.env)")
    args = parser.parse_args()

    # Load environment variables
    env_path = Path(args.env)
    if env_path.exists():
        load_dotenv(env_path)
    else:
        load_dotenv()

    tenant_id = os.getenv("TENANT_ID")
    client_id = os.getenv("CLIENT_ID")
    client_secret = os.getenv("CLIENT_SECRET")
    user_id = os.getenv("USER_ID")

    missing = []
    if not tenant_id:
        missing.append("TENANT_ID")
    if not client_id:
        missing.append("CLIENT_ID")
    if not client_secret:
        missing.append("CLIENT_SECRET")
    if not user_id:
        missing.append("USER_ID")

    if missing:
        print(f"Missing required environment variables: {', '.join(missing)}", file=sys.stderr)
        print("Set them in your .env file or as environment variables.", file=sys.stderr)
        sys.exit(1)

    # 1. Read the file content
    print(f"Reading file: {args.file}")
    binary = is_binary_file(args.file)
    if binary:
        file_bytes = read_file_binary(args.file)
        print(f"  File size: {len(file_bytes)} bytes (binary)")
    else:
        file_content = read_file_content(args.file)
        file_bytes = None
        print(f"  File size: {len(file_content.encode('utf-8'))} bytes (text)")

    # 2. Acquire access token
    print("Acquiring access token...")
    access_token = acquire_token(tenant_id, client_id, client_secret)
    print("  Token acquired successfully.")

    # 3. Build and send processContent request
    print("Sending processContent request to Microsoft Graph...")
    if binary:
        request_body = build_request_body(args.file, client_id, user_id, file_bytes=file_bytes)
    else:
        request_body = build_request_body(args.file, client_id, user_id, file_content=file_content)
    result = process_content(access_token, user_id, request_body)

    # 4. Display results
    print_results(result, args.file)


if __name__ == "__main__":
    main()
