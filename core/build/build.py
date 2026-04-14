# Copyright 2017-2023 Valve Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import re
import subprocess
import sys
import shutil

def parse_command_line():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--platform', help="Target operating system.", choices=['windows'], type=str.lower, default='windows')
    parser.add_argument('-t', '--toolchain', help="Compiler toolchain.", choices=['vs2019', 'vs2022'], type=str.lower, default='vs2022')
    parser.add_argument('-a', '--architecture', help="CPU architecture.", choices=['x86', 'x64', 'arm64'], type=str.lower, default='x64')
    parser.add_argument('-c', '--configuration', help="Build configuration.", choices=['debug', 'release'], type=str.lower, default='release')
    parser.add_argument('-o', '--operation', help="CMake operation.", choices=['generate', 'build', 'install', 'default', 'ci_build'], type=str.lower, default='default')
    parser.add_argument('--unity', action='store_true', help='Configure a unity build.')
    parser.add_argument('--parallel', type=int, default=0, help='Number of threads to use when building.')
    args = parser.parse_args()
    return args

def build_subdir(args):
    return "-".join([args.platform, args.toolchain, args.architecture])

def root_dir():
    return os.path.normpath(os.getcwd() + '/../..')

def generator_name(args):
    generators = {
        'vs2019': 'Visual Studio 16 2019',
        'vs2022': 'Visual Studio 17 2022',
    }
    return generators.get(args.toolchain, 'Visual Studio 16 2019')

def config_name(args):
    return "Debug" if args.configuration == 'debug' else "Release"

def run_cmake(program_name, args):
    env = os.environ.copy()
    if os.getenv('STEAMAUDIO_OVERRIDE_PYTHONPATH') is not None:
        env['PYTHONPATH'] = ''
    subprocess.check_call([program_name] + args, env=env)

def cmake_generate(args):
    cmake_args = ['-G', generator_name(args)]

    if args.architecture == 'x86':
        cmake_args += ['-A', 'Win32']
    elif args.architecture == 'x64':
        cmake_args += ['-A', 'x64']
    elif args.architecture == 'arm64':
        cmake_args += ['-A', 'ARM64']

    cmake_args += ['-DCMAKE_INSTALL_PREFIX=' + root_dir() + '/bin']

    if args.unity:
        cmake_args += ['-DCMAKE_UNITY_BUILD=TRUE']

    cmake_args += ['../..']

    run_cmake('cmake', cmake_args)

def cmake_build(args):
    cmake_args = ['--build', '.', '--config', config_name(args)]

    if args.parallel > 0:
        cmake_args += ['--parallel', str(args.parallel)]

    run_cmake('cmake', cmake_args)

def cmake_install(args):
    run_cmake('cmake', ['--install', '.', '--config', config_name(args)])

def find_tool(name, dir_regex, min_version):
    matches = {}

    tools_dirs = [os.path.normpath(root_dir() + '/../../tools'), os.path.normpath(root_dir() + '/../tools')]
    for tools_dir in tools_dirs:
        if not os.path.exists(tools_dir):
            continue

        dir_contents = os.listdir(tools_dir)
        for item in dir_contents:
            dir_path = os.path.join(tools_dir, item)
            if not os.path.isdir(dir_path):
                continue

            match = re.match(dir_regex, os.path.basename(dir_path))
            if match is None:
                continue

            matches[dir_path] = match

    num_version_components = len(min_version)
    latest_version = [0] * num_version_components
    latest_version_path = None

    for path, match in matches.items():
        version = [int(match.group(i)) for i in range(1, num_version_components + 1)]

        newer_version_found = False
        for i in range(num_version_components):
            if version[i] < min_version[i]:
                break
            if version[i] < latest_version[i]:
                break
            newer_version_found = True

        if newer_version_found:
            latest_version = version
            latest_version_path = path

    if latest_version_path is not None:
        latest_version_path = os.path.join(latest_version_path, 'windows-x64')

    return latest_version_path


# Main script
args = parse_command_line()

try:
    os.makedirs(build_subdir(args))
except Exception:
    pass

olddir = os.getcwd()
os.chdir(build_subdir(args))

cmake_path = find_tool('cmake', r'cmake-(\d+)\.(\d+)\.?(\d+)?', [3, 17])
if cmake_path is not None:
    os.environ['PATH'] = os.path.normpath(os.path.join(cmake_path, 'bin')) + os.pathsep + os.environ['PATH']

if args.operation == 'ci_build':
    args.unity = True
    args.parallel = 4

if args.operation == 'generate':
    cmake_generate(args)
elif args.operation == 'build':
    cmake_build(args)
elif args.operation == 'install':
    cmake_install(args)
elif args.operation == 'default':
    cmake_generate(args)
    cmake_build(args)
elif args.operation == 'ci_build':
    cmake_generate(args)
    cmake_build(args)
    cmake_install(args)

os.chdir(olddir)
