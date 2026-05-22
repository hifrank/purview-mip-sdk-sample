"""
MIP Document Reader — MCP Server

Reads sensitivity-labeled / protected Office documents using the MIP SDK CLI.
Decrypted content lives only in memory; no unprotected file is ever written to disk.

Tools exposed:
  - read_protected_doc(file_path)  → extracted text
  - get_doc_label(file_path)       → label metadata
  - list_labels()                  → available sensitivity labels
"""

import io
import json
import logging
import os
import subprocess
import sys
from pathlib import Path

from docx import Document
from dotenv import load_dotenv
from mcp.server.fastmcp import FastMCP

# ── Configuration ───────────────────────────────────────────────────

# Resolve paths relative to *this* file so the server works
# regardless of cwd.
_SERVER_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _SERVER_DIR.parent

# .env lives at the project root (or wherever ENV_FILE points)
_env_path = Path(os.environ.get("ENV_FILE", _PROJECT_ROOT / ".env"))
if _env_path.exists():
    load_dotenv(_env_path)

# The pre-built CLI binary
CLI_BIN = os.environ.get(
    "MIP_CLI_BIN",
    str(_PROJECT_ROOT / "labeler" / "build" / "mip-sdk-cli"),
)

# Identity is locked to server config — never accepted from callers.
USER_EMAIL = os.environ.get("USER_EMAIL", "")

# The .env file the CLI should use (same one)
CLI_ENV_FLAG = ["--env", str(_env_path)] if _env_path.exists() else []

logging.basicConfig(level=logging.INFO, stream=sys.stderr)
log = logging.getLogger("mip-mcp")

# ── Helpers ─────────────────────────────────────────────────────────


def _run_cli(*args: str, binary: bool = False) -> subprocess.CompletedProcess:
    """Run mip-sdk-cli with the given arguments."""
    cmd = [CLI_BIN] + CLI_ENV_FLAG + list(args)
    log.info("Running: %s", " ".join(cmd))
    return subprocess.run(
        cmd,
        capture_output=True,
        timeout=120,
    )


def _extract_text_from_docx(data: bytes) -> str:
    """Parse .docx bytes in memory and return all paragraph text."""
    doc = Document(io.BytesIO(data))
    paragraphs = [p.text for p in doc.paragraphs if p.text.strip()]
    return "\n".join(paragraphs)


# ── MCP Server ──────────────────────────────────────────────────────

mcp = FastMCP(
    "MIP Document Reader",
)


@mcp.tool()
def read_protected_doc(file_path: str) -> str:
    """Read text content from a sensitivity-labeled or protected .docx file.

    The file is decrypted in memory only — no unprotected copy is ever
    written to disk.  The caller's identity is determined by server
    configuration, not by this parameter.

    Args:
        file_path: Absolute path to the .docx file.

    Returns:
        The extracted text content of the document.
    """
    path = Path(file_path).resolve()
    if not path.exists():
        return f"Error: File not found: {path}"

    suffix = path.suffix.lower()
    if suffix not in (".docx", ".doc", ".pdf", ".pptx", ".xlsx"):
        return f"Error: Unsupported file type: {suffix}"

    # Decrypt via CLI — output is raw bytes on stdout
    result = _run_cli("label", "decrypt", str(path), binary=True)

    if result.returncode != 0:
        stderr = result.stderr.decode(errors="replace")
        return f"Error decrypting file: {stderr}"

    raw_bytes = result.stdout

    if len(raw_bytes) == 0:
        return "Error: No data returned from decryption."

    # Parse the decrypted content
    if suffix == ".docx":
        try:
            return _extract_text_from_docx(raw_bytes)
        except Exception as e:
            return f"Error parsing .docx: {e}"
    else:
        return f"Text extraction for {suffix} is not yet implemented. Decrypted {len(raw_bytes)} bytes."


@mcp.tool()
def get_doc_label(file_path: str) -> str:
    """Get the sensitivity label applied to a file.

    Args:
        file_path: Absolute path to the file.

    Returns:
        Label name and ID, or a message if no label is applied.
    """
    path = Path(file_path).resolve()
    if not path.exists():
        return f"Error: File not found: {path}"

    result = _run_cli("label", "get", str(path))

    output = result.stdout.decode(errors="replace")
    stderr = result.stderr.decode(errors="replace")

    if result.returncode != 0:
        return f"Error: {stderr}"

    return output.strip()


@mcp.tool()
def list_labels() -> str:
    """List all sensitivity labels available in the organization.

    Returns:
        A formatted list of label names and IDs.
    """
    result = _run_cli("label", "list")

    output = result.stdout.decode(errors="replace")
    stderr = result.stderr.decode(errors="replace")

    if result.returncode != 0:
        return f"Error: {stderr}"

    return output.strip()


# ── Entry point ─────────────────────────────────────────────────────

if __name__ == "__main__":
    mcp.run(transport="stdio")
