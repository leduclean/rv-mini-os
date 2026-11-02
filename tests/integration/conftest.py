import os

import pexpect
import pytest


@pytest.fixture
def os_shell():
    cmd = (
        os.environ.get("OS_QEMU_CMD")
        or pytest.fail("run via `ninja integration` or export OS_QEMU_CMD")
    ).split(";")
    # NOTE: every test shares ${DISK_IMG}, copy it into tmp_path if tests start writing to disk
    qemu = pexpect.spawn(cmd[0], cmd[1:], encoding="utf-8", timeout=10)
    qemu.expect_exact("(shell) ")
    yield qemu
    qemu.close(force=True)
