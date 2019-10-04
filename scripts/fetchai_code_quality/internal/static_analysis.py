import re
import shutil
import subprocess
import sys
import tempfile
import yaml
from os.path import abspath, commonprefix, isabs, isfile, join, relpath


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
        ['clang-tidy-6.0', '-list-checks']).decode()

    enabled_checks = parse_checks_list(enabled_checks_output)

    return sorted(enabled_checks)


def get_disabled_checks_list():
    all_checks_output = subprocess.check_output(
        ['clang-tidy-6.0', '-list-checks', '-checks=*']).decode()
    enabled_checks_output = subprocess.check_output(
        ['clang-tidy-6.0', '-list-checks']).decode()

    all_checks = parse_checks_list(all_checks_output)
    enabled_checks = parse_checks_list(enabled_checks_output)

    disabled_checks = list(set(all_checks).difference(set(enabled_checks)))

    return sorted(disabled_checks)


def static_analysis(project_root, build_root, fix, concurrency):
    output_file = abspath(join(build_root, 'clang_tidy_fixes.yaml'))
    runner_script_path = shutil.which('run-clang-tidy-6.0.py')
    assert runner_script_path is not None
    clang_tidy_path = shutil.which('clang-tidy-6.0')
    clang_apply_replacements_path = shutil.which(
        'clang-apply-replacements-6.0')
    if fix:
        assert clang_apply_replacements_path is not None, \
            'clang-apply-replacements-6.0 must be installed and ' \
            'found in the path (install clang-tools-6.0)'

    cmd = [
        'python3', '-u',
        runner_script_path,
        '-j{concurrency}'.format(concurrency=concurrency),
        '-p={path}'.format(path=abspath(build_root)),
        '-header-filter=.*(apps|libs).*\\.hpp$',
        '-quiet',
        '-clang-tidy-binary={clang_tidy_path}'.format(
            clang_tidy_path=clang_tidy_path),
        # Hacky workaround: we do not need clang-apply-replacements unless applying fixes
        # through run-clang-tidy-6.0.py (which we cannot do, as it would alter vendor
        # libraries). Unfortunately, run-clang-tidy will refuse to function unless it
        # thinks that clang-apply-replacements is installed on the system. We pass
        # a valid path to an arbitrary executable here to placate it.
        '-clang-apply-replacements-binary={clang_tidy_path}'.format(
            clang_tidy_path=clang_tidy_path),
        '-export-fixes={output_file}'.format(output_file=output_file)]

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
        exit_code = subprocess.call(cmd, cwd=project_root,
                                    stdout=f, stderr=f)
        if exit_code != 0:
            print('clang-tidy reported an error')
            f.seek(0)
            print('\n{text}\n'.format(text=f.read().decode()))
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
            print('Applying fixes...')
            subprocess.call(['clang-apply-replacements-6.0',
                             '-format', '-style=file', '.'], cwd=build_root)
            print(
                'Done. Note that the automated fix feature of clang-tidy is unreliable:')
            print('  * not all checks are supported,')
            print('  * applying fixes may result in invalid code.')

        sys.exit(1)
    else:
        print('No violations found.')
