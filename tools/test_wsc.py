#!/usr/bin/env python3
"""
Automated WonderSwan Color target execution and test runner.
Discovers runners in tools/runner/ or system PATH, launches .wsc binaries,
and performs automated execution tests.
"""

import sys
import os
import glob
import subprocess
import time
import argparse
import shutil

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUNNER_DIR = os.path.join(BASE_DIR, "tools", "runner")

def find_runners():
    runners = []
    # 1. Search tools/runner/ directly and 1 level deep
    candidates = glob.glob(os.path.join(RUNNER_DIR, "*.exe")) + \
                 glob.glob(os.path.join(RUNNER_DIR, "*", "*.exe"))
    for c in candidates:
        if os.path.isfile(c):
            runners.append(os.path.abspath(c))

    # 2. Search system PATH for standard names
    for name in ["mednafen", "mesen", "Mesen"]:
        which_path = shutil.which(name)
        if which_path and which_path not in runners:
            runners.append(which_path)

    return runners

def get_default_runner():
    runners = find_runners()
    if not runners:
        return None
    # Prefer mednafen or mesen if present
    for r in runners:
        lower = os.path.basename(r).lower()
        if "mednafen" in lower or "mesen" in lower:
            return r
    return runners[0]

def run_wsc(rom_path, runner_path=None, validate_timeout=None, extra_args=None):
    if not os.path.isfile(rom_path):
        print(f"Error: Target binary not found: {rom_path}")
        return False

    runner = runner_path or get_default_runner()
    if not runner:
        print("No executable runner found in tools/runner/ or system PATH.")
        print("Please place a WonderSwan executable (e.g. mednafen.exe, Mesen.exe) in tools/runner/.")
        return False

    cmd = [runner, os.path.abspath(rom_path)]
    if extra_args:
        cmd.extend(extra_args)

    print(f"Launching runner: {os.path.basename(runner)}")
    print(f"Target binary: {os.path.abspath(rom_path)}")

    if validate_timeout:
        print(f"Running automated validation test ({validate_timeout}s timeout)...")
        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            time.sleep(validate_timeout)
            poll = proc.poll()
            if poll is not None and poll != 0:
                stdout, stderr = proc.communicate()
                print(f"FAIL: Runner crashed with exit code {poll}")
                if stderr:
                    print("Error output:", stderr.decode(errors='ignore'))
                return False
            else:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
                print("PASS: Target binary initialized and ran successfully with no crashes.")
                return True
        except Exception as e:
            print(f"FAIL: Execution error: {e}")
            return False
    else:
        try:
            subprocess.run(cmd)
            return True
        except KeyboardInterrupt:
            return True
        except Exception as e:
            print(f"Execution error: {e}")
            return False

def main():
    parser = argparse.ArgumentParser(description="WonderSwan Color Automated Target Runner")
    parser.add_argument("rom", nargs="?", help="Path to .wsc binary file")
    parser.add_argument("--runner", help="Explicit path to runner executable")
    parser.add_argument("--validate", type=float, const=4.0, nargs="?", help="Automated validation mode with timeout in seconds (default: 4.0s)")
    parser.add_argument("--list", action="store_true", help="List discovered runners")

    args = parser.parse_args()

    if args.list:
        runners = find_runners()
        if runners:
            print("Discovered runners:")
            for r in runners:
                print(f"  - {r}")
        else:
            print("No runners found in tools/runner/ or system PATH.")
        return 0

    if not args.rom:
        parser.print_help()
        return 1

    success = run_wsc(args.rom, runner_path=args.runner, validate_timeout=args.validate)
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
