#!/usr/bin/env python3
"""Offline syntax/context check for the optional llama.cpp MPTCP patch.

The fixture contains the exact old-line contexts reviewed in transport.cpp blob
`a728152421f7dac44baefc582d713540398dabe4` on 2026-07-17. This validates that
our packaged patch is a structurally valid unified diff against that source
shape; it is not a substitute for rebasing against a later llama.cpp revision.
"""
from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATCH = ROOT / "patches/0001-ggml-rpc-optional-native-mptcp.patch"


def write_fixture(path: Path) -> None:
    lines: list[str] = []
    # Place the first hunk at its reviewed location.
    lines.extend(f"// fixture filler {i}\n" for i in range(1, 38))
    lines.extend(
        [
            "#endif\n",
            "\n",
            'static const char * RPC_DEBUG = std::getenv("GGML_RPC_DEBUG");\n',
            "\n",
            "#define LOG_DBG(...) \\\n",
            "    do { if (RPC_DEBUG) GGML_LOG_DEBUG(__VA_ARGS__); } while (0)\n",
        ]
    )
    while len(lines) < 600:
        lines.append(f"// fixture filler {len(lines) + 1}\n")
    lines.extend(
        [
            "}\n",
            "\n",
            "socket_ptr socket_t::create_server(const char * host, int port) {\n",
            "    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);\n",
            "    if (!is_valid_fd(sockfd)) {\n",
            "        return nullptr;\n",
            "    }\n",
        ]
    )
    while len(lines) < 627:
        lines.append(f"// fixture filler {len(lines) + 1}\n")
    lines.extend(
        [
            "}\n",
            "\n",
            "socket_ptr socket_t::connect(const char * host, int port) {\n",
            "    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);\n",
            "    if (!is_valid_fd(sockfd)) {\n",
            "        return nullptr;\n",
            "    }\n",
        ]
    )
    path.write_text("".join(lines), encoding="utf-8")


def main() -> int:
    if shutil.which("git") is None:
        raise SystemExit("git is required for patch validation")
    if not PATCH.is_file():
        raise SystemExit(f"missing patch: {PATCH}")

    # Parse the diff before the context test. A malformed hunk fails here.
    subprocess.run(["git", "apply", "--numstat", str(PATCH)], check=True, capture_output=True, text=True)

    with tempfile.TemporaryDirectory(prefix="llama-mptcp-patch-") as td:
        work = Path(td)
        source = work / "ggml/src/ggml-rpc/transport.cpp"
        source.parent.mkdir(parents=True)
        write_fixture(source)
        subprocess.run(["git", "apply", "--check", str(PATCH)], cwd=work, check=True)
        subprocess.run(["git", "apply", str(PATCH)], cwd=work, check=True)
        text = source.read_text(encoding="utf-8")
        assert "static int rpc_stream_protocol()" in text
        assert text.count("SOCK_STREAM, rpc_stream_protocol()") == 2

    print("llama.cpp MPTCP patch parses and applies to the reviewed source contexts")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
