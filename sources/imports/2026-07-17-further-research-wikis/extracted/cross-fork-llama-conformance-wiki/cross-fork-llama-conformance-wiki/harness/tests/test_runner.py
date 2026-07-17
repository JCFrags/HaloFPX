from pathlib import Path
import sys
from llama_conformance.runner import run_command

def test_runner_success(tmp_path: Path):
    result = run_command(
        [sys.executable, "-c", "print('ok')"],
        output_dir=tmp_path / "success",
        timeout_seconds=5,
    )
    assert result.status == "pass"
    assert result.exit_code == 0
    assert "ok" in Path(result.stdout_path).read_text()

def test_runner_timeout(tmp_path: Path):
    result = run_command(
        [sys.executable, "-c", "import time; time.sleep(5)"],
        output_dir=tmp_path / "timeout",
        timeout_seconds=0.05,
    )
    assert result.status == "timeout"
    assert result.exit_code is None
