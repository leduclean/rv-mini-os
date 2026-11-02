import pytest
import pexpect


@pytest.fixture
def os_shell():
    qemu = pexpect.spawn()
