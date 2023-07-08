#!/usr/bin/env python3

import sys
import subprocess
import time
import os
from pathlib import Path
from typing import IO, Any, Callable, Dict, List, Optional, Text, Union, Iterator

from testsupport import run, subtest, assert_executable, find_project_executable

def fuse_mount(temp_path: Path, mt_pt: str) -> str:
    assert_executable("fusermount", "fusermount not found")
    mnt_path = gen_mnt_path(temp_path, mt_pt)
    
    with subtest("Mount filesystem"):
        exe_path = find_project_executable("memfs")
        try:
            proc = run_background([exe_path, mnt_path])
        except Exception as e:
            fuse_unmount(mnt_path)
            sys.exit(1)
    
    return mnt_path

def run_background(args: List[str]) -> subprocess.Popen:
    proc = subprocess.Popen(args)
    time.sleep(5)

    return proc

def fuse_unmount(mnt_path: str) -> "subprocess.CompletedProcess[Text]":
    if os.path.isdir(mnt_path):
        if os.path.ismount(mnt_path):
            run(["fusermount", "-u", str(mnt_path)])

def gen_mnt_path(temp_path: Path, mt_pt: str) -> str:
    mnt_path = temp_path.joinpath(mt_pt)
    os.mkdir(mnt_path)

    return mnt_path

def fuse_check_mnt(tmpdir: str, mnt_path: str) -> None:
    with open(f'{tmpdir}/stdout', 'w+') as stdout:
        run(
            ["mount"],
            stdout=stdout,
        )

    with open(f'{tmpdir}/stdout') as stdin:
        found = False

        for line in stdin.readlines():
            parts = line.split(" ")

            fs_spec = parts[0]            
            fs_path = Path(parts[2])

            if "memfs" in fs_spec and fs_path == mnt_path:
                found = True

        if not found:
            print(f"memfs not found at {mnt_path}")
            fuse_unmount(mnt_path)
            exit(1)