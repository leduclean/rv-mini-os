import re
from pathlib import Path

import pexpect
import pytest

OS_SOURCE: Path = Path(__file__).parents[2] / "src"
pattern = re.compile(r"X\((\w+_test)")
with open(OS_SOURCE / "user/include/cmd_registry.h") as f:
    content = f.read()
    tests: list[str] = pattern.findall(content)


@pytest.mark.parametrize("test_name", tests)
def test_cmd(os_shell: pexpect.spawn, test_name: str):
    os_shell.send(test_name + "\n")
    idx: int = os_shell.expect([r"\[TEST\] \w+ SUCCESS", r"\[TEST\] \w+ ERROR"])
    assert idx == 0, os_shell.before
