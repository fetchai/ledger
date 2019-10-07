import re
import shutil
import subprocess
import sys
import tempfile
import yaml
import glob
import os
import json
from os.path import abspath, commonprefix, isabs, isfile, join, relpath
from .toolchains import ClangToolchain


CLANG_TOOLCHAIN = ClangToolchain()


def convert_to_dependencies(
        changed_files_fullpath: set, build_path, verbose=False):
    """For each changed hpp file, find all cpp files that depend on it (clang-tidy requires cpp files only) and replace it in the set.

    To do this, find cmake generated depend.make files
    Example depend.make format:
    libs/metrics/CMakeFiles/fetch-metrics.dir/src/metrics.cpp.o: ../libs/vectorise/include/vectorise/math/exp.hpp
    """

    targets_to_substitute = set()
    targets_substituted = set()
    for filename in changed_files_fullpath:
        if '.hpp' in filename:
            targets_to_substitute.add(filename)

    if targets_to_substitute == 0:
        return changed_files_fullpath

    globbed_files = glob.glob(
        build_path + '/libs/**/depend.make', recursive=True)
    globbed_files.extend(
        glob.glob(build_path + '/apps/**/depend.make', recursive=True))

    # files with no content have this as their first line
    globbed_files = [
        x for x in globbed_files
        if 'Empty dependencies file' not in open(x).readline()]

    if len(globbed_files) == 0:
        print(
            "\nWARNING: No non-empty depend.make files found in {} . Did you make sure to build in this directory?".format(build_path))

    for filename in globbed_files:
        if verbose:
            print("reading file: {}".format(filename))

        for line in open(filename):

            dependency_file = "{}{}{}".format(
                build_path, '/', line.split(' ')[-1])

            if '.hpp' not in dependency_file:
                continue

            dependency_file = os.path.abspath(dependency_file).rstrip()

            target_cpp_file = line.split('.o:')[0]
            target_cpp_file = re.sub(
                r'CMakeFiles.*\.dir', '**', target_cpp_file.rstrip())

            if os.path.exists(
                    dependency_file) and '.cpp' in target_cpp_file and dependency_file in changed_files_fullpath:

                if verbose:
                    print("globbing for:")
                    print(target_cpp_file)

                target_cpp_file_save = target_cpp_file
                target_cpp_file = glob.glob(
                    "**" + target_cpp_file, recursive=True)

                if target_cpp_file is None or len(target_cpp_file) == 0:
                    print("Failed to find file/dependency: ")
                    print("File: {}".format(target_cpp_file_save))
                    print("Dependency: {}".format(dependency_file))
                    print("Your build directory may be old")
                    break

                if target_cpp_file is None or len(target_cpp_file) > 1:
                    print("Too many files found matching {}".format(
                        target_cpp_file))

                target_cpp_file = os.path.abspath(target_cpp_file[0])

                if verbose:
                    print("source file: {}".format(dependency_file))
                    print("target cpp file: {}".format(target_cpp_file))

                changed_files_fullpath.add(target_cpp_file)
                # changed_files_fullpath.remove(dependency_file)

                targets_substituted.add(dependency_file)

    if targets_to_substitute != targets_substituted:
        print(
            "\nWARNING: Failed to find dependencies for the following files:")
        print(targets_to_substitute.difference(targets_substituted))

    # Remove all hpp files
    changed_files_fullpath = [
        x for x in changed_files_fullpath if '.hpp' not in x]

    return changed_files_fullpath


def find_nth(text, substr, n):
    augtext = b'\n' + text
    occurrences = [i for i in range(
        0, len(augtext)) if augtext[i:].startswith(substr)]
    assert len(occurrences) > n
    return occurrences[n]


def excluded_diagnostic(d, absolute_path, project_root, relative_path):
    vendor_path = abspath(join(project_root, 'vendor'))
    EXCLUSIONS = ('libs/vm/include/vm/tokeniser.hpp',
                  'libs/vm/src/tokeniser.cpp')
    is_excluded_path = commonprefix([vendor_path, absolute_path]) == vendor_path or \
        commonprefix([project_root, absolute_path]) != project_root or \
        relative_path in EXCLUSIONS

    is_gtest_or_gbench_cert_err58_cpp_problem = \
        d['DiagnosticName'] == 'cert-err58-cpp' and \
        any([d['Message'].startswith('initialization of \'{}'.format(prefix))
             for prefix in ('gtest_', '_benchmark_', 'test_info_\'')])

    special_member_fns_pattern = re.compile(
        "^class '.+_Test' defines a copy constructor and a copy assignment operator but "
        "does not define a destructor, a move constructor or a move assignment operator$")
    is_gtest_special_member_functions_problem = \
        d['DiagnosticName'] in ('cppcoreguidelines-special-member-functions',
                                'hicpp-special-member-functions') and \
        special_member_fns_pattern.match(d['Message'])

    return is_excluded_path or \
        is_gtest_or_gbench_cert_err58_cpp_problem or \
        is_gtest_special_member_functions_problem


def read_static_analysis_yaml(project_root, build_root, yaml_file_path):
    out = {}

    with open(yaml_file_path, 'r') as f:
        document = yaml.safe_load(f)

    if document is not None:
        new_diagnostics = []
        for d in document['Diagnostics']:
            file_path = d['FilePath']

            # ignore extries that refer to empty file paths (this is usually a waning that clang
            # have stopped processing a file because too many warnings have been generated from that
            # file
            if len(file_path) == 0:
                continue

            absolute_path = file_path if isabs(
                file_path) else abspath(join(build_root, file_path))
            assert isfile(absolute_path), "Not a file: {}".format(
                absolute_path)
            relative_path = relpath(absolute_path, project_root)

            if not excluded_diagnostic(d, absolute_path, project_root, relative_path):
                new_diagnostics.append(d)
                if relative_path not in out:
                    out[relative_path] = {
                        'absolute_path': absolute_path, 'diagnostics': []}
                out[relative_path]['diagnostics'] += [{
                    'offset': d['FileOffset'],
                    'check': d['DiagnosticName'],
                    'message': d['Message']}]

        document['Diagnostics'] = new_diagnostics
        with open(yaml_file_path, 'w') as f:
            yaml.dump(document, f)

    for relative, diagnostic in out.items():
        # deduplicate
        out[relative]['diagnostics'] = [dict(t) for t in {tuple(
            d.items()) for d in out[relative]['diagnostics']}]
        # sort by offset
        out[relative]['diagnostics'] = sorted(
            out[relative]['diagnostics'], key=lambda d: d['offset'])

        # convert byte offsets to human-readable data
        with open(out[relative]['absolute_path'], 'rb') as cplusplus:
            text = cplusplus.read()
            for d in out[relative]['diagnostics']:
                d['line_number'] = text[0:d['offset']].decode().count('\n') + 1
                line1 = find_nth(text, b'\n', d['line_number'] - 1)
                line2 = find_nth(text, b'\n', d['line_number']) - 1

                d['char_number'] = len(text[line1:d['offset']].decode()) + 1
                d['context'] = [text[line1:line2].decode(), ' ' *
                                (d['char_number'] + 1) + '^']

    return out


def parse_checks_list(output):
    lines = output.split('\n')
    checks = []
    for line in lines:
        stripped = line.strip()
        if '-' in stripped:
            checks.append(stripped)

    return checks


def get_enabled_checks_list():
    enabled_checks_output = subprocess.check_output(
        [CLANG_TOOLCHAIN.clang_tidy_path, '-list-checks']).decode()

    enabled_checks = parse_checks_list(enabled_checks_output)

    return sorted(enabled_checks)


def get_disabled_checks_list():
    all_checks_output = subprocess.check_output(
        [CLANG_TOOLCHAIN.clang_tidy_path, '-list-checks', '-checks=*']).decode()
    enabled_checks_output = subprocess.check_output(
        [CLANG_TOOLCHAIN.clang_tidy_path, '-list-checks']).decode()

    all_checks = parse_checks_list(all_checks_output)
    enabled_checks = parse_checks_list(enabled_checks_output)

    disabled_checks = list(set(all_checks).difference(set(enabled_checks)))

    return sorted(disabled_checks)


def get_changed_paths_from_git(project_root, commit):
    raw_relative_paths = subprocess.check_output(['git', 'diff', '--name-only', commit]) \
        .strip() \
        .decode('utf-8') \
        .split('\n')

    relative_paths = [rel_path.strip() for rel_path in raw_relative_paths]

    return [abspath(join(project_root, rel_path)) for rel_path in relative_paths]


def filter_non_matching(file_list, words):
    return [x for x in file_list if any(file_ending in x for file_ending in words)]


def filter_compile_commands(project_root, input_location, target_files, output_location):
    filtered_commands = []

    # read the input file and filter the entries
    with open(input_location, 'r') as input_file:
        commands = json.load(input_file)

        for command in commands:
            command_file_path = command['file']
            relative_file_path = os.path.relpath(
                command_file_path, project_root)

            if relative_file_path.startswith('vendor'):
                continue

            if relative_file_path == 'libs/vm/src/tokeniser.cpp':
                continue

            if target_files is not None:
                match = False
                for target_file in target_files:
                    if target_file in command_file_path:
                        match = True
                        break

                if not match:
                    continue

            # add the command to the filtered set
            filtered_commands.append(command)

    # flush the output file
    with open(output_location, 'w') as output_file:
        json.dump(filtered_commands, output_file)


def static_analysis(project_root, build_root, fix, concurrency, commit, verbose):

    # ensure the static analysis root is correct
    static_analysis_root = os.path.join(build_root, 'static_analysis')
    os.makedirs(static_analysis_root, exist_ok=True)

    target_files = None

    # In the case we are linting a subset of all files
    if commit:
        target_files = get_changed_paths_from_git(project_root, *commit)
        target_files = filter_non_matching(target_files, [".hpp", ".cpp"])

        if verbose:
            print("Relevant files that differ: {}".format(target_files))

        target_files = convert_to_dependencies(
            set(target_files),
            abspath(build_root),
            verbose
        )

        if verbose:
            print("Relevant files that differ (after dependency conversion): {}".format(
                clang_apply_replacements_path))

        # exit if there is nothing that needs to be done
        if len(target_files) == 0:
            print("\n\nThere doesn't appear to be any relevant files to lint. 👍.\n\n")
            return

    # filter the compile commands
    filter_compile_commands(
        project_root,
        os.path.join(build_root, 'compile_commands.json'),
        target_files,
        os.path.join(static_analysis_root, 'compile_commands.json')
    )

    output_file = abspath(join(static_analysis_root, 'clang_tidy_fixes.yaml'))

    cmd = [
        'python3', '-u',
        CLANG_TOOLCHAIN.run_clang_tidy_path,
        '-j{}'.format(concurrency),
        '-p={}'.format(static_analysis_root),
        '-header-filter=.*(apps|libs).*\\.hpp$',
        '-clang-tidy-binary={}'.format(CLANG_TOOLCHAIN.clang_tidy_path),
        # Hacky workaround: we do not need clang-apply-replacements unless applying fixes
        # through run-clang-tidy-6.0.py (which we cannot do, as it would alter vendor
        # libraries). Unfortunately, run-clang-tidy will refuse to function unless it
        # thinks that clang-apply-replacements is installed on the system. We pass
        # a valid path to an arbitrary executable here to placate it.
        '-clang-apply-replacements-binary={}'.format(
            CLANG_TOOLCHAIN.clang_tidy_path),
        '-export-fixes={}'.format(output_file)
    ]

    if fix:
        cmd += ['-fix']

    print('\nPerform static analysis')

    enabled_checks = get_enabled_checks_list()
    print('\nEnabled checks ({}):'.format(len(enabled_checks)))
    print('  ' + '\n  '.join(enabled_checks))

    disabled_checks = get_disabled_checks_list()
    print('\nDisabled checks ({}):'.format(len(disabled_checks)))
    print('  ' + '\n  '.join(disabled_checks))

    print('\nCommand:')
    print('  {cmd}\n'.format(cmd=" ".join(cmd)))

    print('Working...')
    with tempfile.SpooledTemporaryFile() as f:

        # execute the run-clang-tidy "helper" script
        exit_code = subprocess.call(cmd, cwd=project_root, stdout=f, stderr=f)

        if exit_code != 0:
            print('clang-tidy reported an error')
            f.seek(0)
            print('\n{}\n'.format(f.read().decode()))
            sys.exit(1)

    print('Done.')

    print('Processing results...')
    files_diagnostics = read_static_analysis_yaml(
        project_root, build_root, output_file)

    if len(files_diagnostics):
        for file, data in files_diagnostics.items():
            print('\n' + (10 * '-'))
            print(
                'Static analysis check(s) failed in:\n  {file}\n'.format(file=file))
            for d in data['diagnostics']:
                print('  Line: {line_number} @ {char_number}'.format(line_number=d["line_number"],
                                                                     char_number=d["char_number"]))
                print('  Message: {message}'.format(message=d["message"]))
                print('  Check: {check}'.format(check=d["check"]))
                print('  Context:')
                print('    {context}\n'.format(
                    context='\n  '.join(d["context"])))

        print(
            '\nStatic analysis found {total_violations} violation(s) '
            'in {total_files} file(s)'.format(
                total_violations=sum([len(d["diagnostics"])
                                      for f, d in files_diagnostics.items()]),
                total_files=len(files_diagnostics)))

        if fix:
            if CLANG_TOOLCHAIN.clang_apply_replacements_path is None:
                raise RuntimeError(
                    'Unable to detect `clang-apply-replacements`. This is required for applying fixes')

            print('Applying fixes...')
            subprocess.call([CLANG_TOOLCHAIN.clang_apply_replacements_path,
                             '-format', '-style=file', '.'], cwd=build_root)
            print(
                'Done. Note that the automated fix feature of clang-tidy is unreliable:')
            print('  * not all checks are supported,')
            print('  * applying fixes may result in invalid code.')

        sys.exit(1)
    else:
        print('No violations found.')
