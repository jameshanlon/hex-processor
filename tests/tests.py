import os
import subprocess
import unittest
import definitions as defs

ASM_BINARY = os.path.join(defs.INSTALL_PREFIX, 'hexasm')
SIM_BINARY = os.path.join(defs.INSTALL_PREFIX, 'hexsim')
VTB_BINARY = os.path.join(defs.INSTALL_PREFIX, 'hextb')
CMP_BINARY = os.path.join(defs.INSTALL_PREFIX, 'xcmp')

class Tests(unittest.TestCase):

    def run_asm_program(self, filename, expected_output):
        test_path = os.path.join(defs.ASM_TEST_SRC_PREFIX, filename)
        subprocess.run([ASM_BINARY, test_path, '-o', 'a.bin'])
        # Sim
        output = subprocess.check_output([SIM_BINARY, 'a.bin'])
        self.assertTrue(output.decode('utf-8') == expected_output)
        # Verilator
        if (defs.USE_VERILATOR):
            output = subprocess.check_output([VTB_BINARY, 'a.bin'])
            self.assertTrue(output.decode('utf-8').endswith(expected_output))

    def run_x_program(self, filename, expected_output):
        # Compiler
        infile = open(os.path.join(defs.X_TEST_SRC_PREFIX, filename), 'rb')
        subprocess.run([SIM_BINARY, 'xhexb.bin'], input=infile.read())
        output = subprocess.check_output([SIM_BINARY, 'simout2'])
        self.assertTrue(output.decode('utf-8') == expected_output)
        # Verilator
        if (defs.USE_VERILATOR):
            output = subprocess.check_output([VTB_BINARY, 'simout2'])
            self.assertTrue(output.decode('utf-8').endswith(expected_output))
        infile.close()

    def setUp(self):
        # Create a xhexb compiler binary.
        subprocess.run([ASM_BINARY, defs.XHEXB_SRC, '-o', 'xhexb.bin'])

    def test_x_hello(self):
        self.run_x_program('hello.x', 'hello\n')

    def test_x_compiler_sim(self):
        # Compile xhexb.x with xhexb.bin on simulator.
        infile = open(os.path.join(defs.X_TEST_SRC_PREFIX, 'xhexb.x'), 'rb')
        output = subprocess.run([SIM_BINARY, 'xhexb.bin'], input=infile.read(), capture_output=True)
        self.assertTrue(output.stdout.decode('utf-8') == 'tree size: 18631\nprogram size: 17101\nsize: 177105\n')
        infile.close()

    def test_x_compiler_verilator(self):
        # Compile xhexb.x with xhexb.bin on hex RTL.
        if (defs.USE_VERILATOR):
            infile = open(os.path.join(defs.X_TEST_SRC_PREFIX, 'xhexb.x'), 'rb')
            output = subprocess.run([VTB_BINARY, 'xhexb.bin'], input=infile.read(), capture_output=True)
            self.assertTrue(output.stdout.decode('utf-8').endswith('tree size: 18631\nprogram size: 17101\nsize: 177105\n'))
            infile.close()

if __name__ == '__main__':
    unittest.main()
