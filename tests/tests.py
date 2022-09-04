import os
import subprocess
import unittest
import definitions as defs

ASM_BINARY = os.path.join(defs.INSTALL_PREFIX, 'hexasm')
SIM_BINARY = os.path.join(defs.INSTALL_PREFIX, 'hexsim')
VTB_BINARY = os.path.join(defs.INSTALL_PREFIX, 'hextb')
CMP_BINARY = os.path.join(defs.INSTALL_PREFIX, 'xcmp')
RUN_BINARY = os.path.join(defs.INSTALL_PREFIX, 'xrun')

class Tests(unittest.TestCase):

    def run_asm_program_simulator(self, filename, expected_output):
        test_path = os.path.join(defs.ASM_TEST_SRC_PREFIX, filename)
        subprocess.run([ASM_BINARY, test_path, '-o', 'a.bin'])
        output = subprocess.run([SIM_BINARY, 'a.bin'], capture_output=True)
        self.assertTrue(output.stdout.decode('utf-8') == expected_output)

    def run_asm_program_verilator(self, filename, expected_output):
        if (defs.USE_VERILATOR):
            test_path = os.path.join(defs.ASM_TEST_SRC_PREFIX, filename)
            subprocess.run([ASM_BINARY, test_path, '-o', 'a.bin'])
            output = subprocess.run([VTB_BINARY, 'a.bin'], capture_output=True)
            self.assertTrue(output.stdout.decode('utf-8').endswith(expected_output))
        else:
            pass

    def run_x_program_simulator(self, filename, expected_output):
        with open(os.path.join(defs.X_TEST_SRC_PREFIX, filename), 'rb') as infile:
            subprocess.run([SIM_BINARY, 'xhexb.bin'], input=infile.read())
        output = subprocess.run([SIM_BINARY, 'simout2'], capture_output=True)
        self.assertTrue(output.stdout.decode('utf-8') == expected_output)

    def run_x_program_verilator(self, filename, expected_output):
        if (defs.USE_VERILATOR):
            with open(os.path.join(defs.X_TEST_SRC_PREFIX, filename), 'rb') as infile:
                subprocess.run([SIM_BINARY, 'xhexb.bin'], input=infile.read())
            output = subprocess.run([VTB_BINARY, 'simout2'], capture_output=True)
            self.assertTrue(output.stdout.decode('utf-8').endswith(expected_output))
        else:
            pass

    def setUp(self):
        # Create a xhexb compiler binary.
        subprocess.run([ASM_BINARY, defs.XHEXB_SRC, '-o', 'xhexb.bin'])

    def test_x_hello_putval(self):
        self.run_x_program_simulator('hello_putval.x', 'hello world\n')
        self.run_x_program_verilator('hello_putval.x', 'hello world\n')

    def test_x_hello_prints(self):
        self.run_x_program_simulator('hello_prints.x', 'hello world\n')
        self.run_x_program_verilator('hello_prints.x', 'hello world\n')

    def test_x_echo_char(self):
        # Test that stdin and stdout work with a program that echos a single character
        subprocess.run([CMP_BINARY, os.path.join(defs.X_TEST_SRC_PREFIX, 'echo_char.x'), '-o', 'a.out'])
        output = subprocess.run([SIM_BINARY, 'a.out'], input=bytes('x', encoding='utf-8'), capture_output=True)
        self.assertTrue(output.stdout.decode('utf-8') == 'x')

    def test_x_compiler_sim(self):
        # Compile xhexb.x with xhexb.bin on simulator.
        with open(os.path.join(defs.X_TEST_SRC_PREFIX, 'xhexb.x'), 'rb') as infile:
            output = subprocess.run([SIM_BINARY, 'xhexb.bin'], input=infile.read(), capture_output=True)
            self.assertTrue(output.stdout.decode('utf-8') == 'tree size: 18631\nprogram size: 17101\nsize: 177105\n')

    def test_x_compiler_verilator(self):
        # Compile xhexb.x with xhexb.bin on hex RTL.
        if (defs.USE_VERILATOR):
            with open(os.path.join(defs.X_TEST_SRC_PREFIX, 'xhexb.x'), 'rb') as infile:
                output = subprocess.run([VTB_BINARY, 'xhexb.bin'], input=infile.read(), capture_output=True)
                self.assertTrue(output.stdout.decode('utf-8').endswith('tree size: 18631\nprogram size: 17101\nsize: 177105\n'))
        else:
            pass

if __name__ == '__main__':
    unittest.main()
