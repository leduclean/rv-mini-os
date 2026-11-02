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
    os_shell.sendline(test_name)
    os_shell.expect(r"\[TEST\] \w+ (SUCCESS|ERROR) ([^\r\n]*)\r\n")
    status: str = os_shell.match.group(1)
    err_msg: str = os_shell.match.group(2).strip()
    assert status == "SUCCESS", f"Test '{test_name}' has failed with: {err_msg}"
