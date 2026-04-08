import os
import subprocess
from timeit import default_timer as timer

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def compile_test(test: str) -> bool:
    if subprocess.run(["../build/heliox",  f"{test}.hx"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
        return False
    if subprocess.run(["nasm", "-felf64", f"{test}.asm", "-o", f"{test}.o"]).returncode != 0: 
        return False
    if subprocess.run(["gcc", "-no-pie", f"{test}.o", "-o", f"{test}"]).returncode != 0:
        return False
    return True

def main():
    os.chdir("tests/")
    tests = os.listdir(".")

    expected_values = {
        "operators1" : 23,
        "operators2": 29,
        "nested_calls1" : 12,
        "nested_calls2" : 9,
        "expression1" : 15,
        "function_arguments1" : 36,
        "print" : 1,
        "conditional1" : 9,
        "conditional2" : 0,
        "factorial": 0,
        "fibonacci": 0,
        "while1": 10,
        "command_line_args": 0,
        "modulo": 0,
        "bitwise": 0,
        "logical": 0,
        "multiple_operations": 30,
        }
    
    expected_outputs = {
        "print": "argc: 1\n\tthis is the number 10 -> 10",
        "operators1": "div: 14 / 3 = 4\nmul: 4 * 3 = 12\nsub: 12 - 3 = 9\nadd: 14 + 9 = 23\n",
        "conditional2" : "4 != 8\n4 < 8\n4 <= 8\n",
        "factorial": "479001600",
        "fibonacci": "75025",
        "while1": "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n",
        "command_line_args": "0: ./command_line_args\n1: first\n2: second\n3: third\n4: fourth\n",
        "modulo": "0\n3\n6\n9\n12\n15\n18\n",
        "bitwise": "1\n7\n6\n-6\n",
        "logical": "1: exec\n2: exec\n"
    }

    command_line_args = {
        "command_line_args": ["first", "second", "third", "fourth"]
    }
    
    total_compile_time = 0
    total_execution_time = 0
    max_compile_time = [0, "no_test"]
    max_execution_time = [0, "no_test"]
    
    passed_tests = []
    failed_tests = []

    for test_path in tests:
        test, ext = os.path.splitext(test_path)
        if ext != ".hx": continue
        compile_time_start = timer()
        if not compile_test(test):
            failed_tests.append(f"{bcolors.FAIL}TEST \"{test}\" FAILED{bcolors.ENDC} -- FAILED TO COMPILE TEST")
            continue
        compile_time_end = timer()
        compile_time = compile_time_end - compile_time_start 
        if compile_time > max_compile_time[0]:
            max_compile_time[0] = compile_time
            max_compile_time[1] = test
        total_compile_time += compile_time

        execution_time_start = timer()
        command = [f"./{test}"]
        if test in command_line_args:
            command.extend(command_line_args[test])

        output = subprocess.run(command, stdout=subprocess.PIPE)
        execution_time_end = timer()
        execution_time = execution_time_end - execution_time_start

        if execution_time > max_execution_time[0]:
            max_execution_time[0] = execution_time
            max_execution_time[1] = test

        total_execution_time += execution_time 

        if output.returncode != expected_values[test]:
            failed_tests.append(f"{bcolors.FAIL}TEST \"{test}\" FAILED{bcolors.ENDC} -- {output.returncode} did not equal {expected_values[test]}")
            continue 
        if test in expected_outputs: 
            ostr = ""
            try:
                ostr = output.stdout.decode()
            except (UnicodeDecodeError):
                ostr = str(output.stdout)
            if expected_outputs[test] != ostr:
                  failed_tests.append(f"{bcolors.FAIL}TEST \"{test}\" FAILED{bcolors.ENDC} \n\toutput: \n{ostr}\n\tdid not equal\n{expected_outputs[test]}")
        passed_tests.append(f"{bcolors.OKGREEN}TEST \"{test}\" PASSED{bcolors.ENDC}")
    
    for file in os.listdir("."):
        ext = os.path.splitext(file)[1]
        if ext != ".hx":
            os.remove(file)
    
    for passed in passed_tests:
        print(passed)
    print("")
    for failed in failed_tests:
        print(failed)

    avg_compile_time = round(total_compile_time / len(tests) * 1000.0, 0)
    avg_execution_time = round(total_execution_time / len(tests) * 1000.0, 0 )
    max_compile_time[0] = round(max_compile_time[0] * 1000.0, 0)
    max_execution_time[0] = round(max_execution_time[0] * 1000.0, 0)
    print(f"avg compile time: {avg_compile_time} ms\navg execution time: {avg_execution_time} ms")
    print(f"max compile time: {max_compile_time[0]} ms at \"{max_compile_time[1]}\"")
    print(f"max execution time: {max_execution_time[0]} ms at \"{max_execution_time[1]}\"")

if __name__ == '__main__':
    main()
