import os
import subprocess
import unittest
import definitions as defs

ASM_BINARY = os.path.join(defs.INSTALL_PREFIX, 'asm')
SIM_BINARY = os.path.join(defs.INSTALL_PREFIX, 'sim')

class Tests(unittest.TestCase):

    def run_program(self, filename, expected_output):
        test_path = os.path.join(defs.TEST_SRC_PREFIX, filename)
        subprocess.run([ASM_BINARY, test_path, '-o', 'a.bin'])
        output = subprocess.check_output([SIM_BINARY, 'a.bin'])
        self.assertTrue(output.decode('utf-8') == expected_output)

    def setUp(self):
        pass

    def test_hello(self):
        self.run_program('hello.S', 'hello\n')

    def test_hello_procedure(self):
        self.run_program('hello_procedure.S', 'hello\n')


if __name__ == '__main__':
    unittest.main()