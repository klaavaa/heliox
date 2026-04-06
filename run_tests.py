import os
import subprocess

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
        "nested_calls1" : 12,
        "nested_calls2" : 9,
        "expression1" : 15,
        "function_arguments1" : 36,
        "print" : 1
        }
    
    expected_outputs = {
        "print": "argc: 1\n\tthis is the number 10 -> 10",
        "operators1": "div: 14 / 3 = 42\nmul: 4 * 3 = 12\nsub: 12 - 3 = 9\nadd: 14 + 9 = 23\n"
    }
            
    
    for test_path in tests:
        test, ext = os.path.splitext(test_path)
        if ext != ".hx": continue
        if not compile_test(test):
            print(f"{bcolors.FAIL}TEST \"{test}\" FAILED{bcolors.ENDC} -- FAILED TO COMPILE TEST")
            continue

        output = subprocess.run([f"./{test}"], stdout=subprocess.PIPE)
        if output.returncode != expected_values[test]:
            print(f"{bcolors.FAIL}TEST \"{test}\" FAILED{bcolors.ENDC} -- {output.returncode} did not equal {expected_values[test]}")
            continue 
        if test in expected_outputs: 
              if expected_outputs[test] != output.stdout.decode():
                  print(f"{bcolors.FAIL}TEST \"{test}\" FAILED{bcolors.ENDC} \n\toutput: \n{output.stdout.decode()}\n\tdid not equal\n{expected_outputs[test]}")
        print(f"{bcolors.OKGREEN}TEST \"{test}\" PASSED{bcolors.ENDC}")
    
    for file in os.listdir("."):
        ext = os.path.splitext(file)[1]
        if ext != ".hx":
            os.remove(file)
    
if __name__ == '__main__':
    main()
