import shutil
import subprocess
import sys
import tempfile
from os.path import abspath, commonprefix, isabs, isfile, join, relpath

import yaml


def read_static_analysis_yaml(project_root, build_root, yaml_file_path):
    out = {}
    vendor_path = abspath(join(project_root, 'vendor'))
    EXCLUSIONS = ('libs/vm/include/vm/tokeniser.hpp',
                  'libs/vm/src/tokeniser.cpp')

    with open(yaml_file_path, 'r') as f:
        document = yaml.safe_load(f)
        new_diagnostics = []
        for d in document['Diagnostics']:
            file_path = d['FilePath']
            absolute_path = file_path if isabs(
                file_path) else abspath(join(build_root, file_path))
            assert isfile(absolute_path)

            relative_path = relpath(absolute_path, project_root)
            if not commonprefix([vendor_path, absolute_path]) == vendor_path and relative_path not in EXCLUSIONS:
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

        # convert offsets to line numbers
        with open(out[relative]['absolute_path'], 'rb') as cplusplus:
            text = cplusplus.read()
            for d in out[relative]['diagnostics']:
                d['line'] = text[0:d['offset']].decode().count('\n') + 1

    return out


def static_analysis(project_root, build_root, fix, concurrency):
    output_file = abspath(join(build_root, 'clang_tidy_fixes.yaml'))
    runner_script_path = shutil.which('run-clang-tidy-6.0.py')
    assert runner_script_path is not None
    clang_tidy_path = shutil.which('clang-tidy-6.0')
    clang_apply_replacements_path = shutil.which(
        'clang-apply-replacements-6.0')
    if fix:
        assert clang_apply_replacements_path is not None, \
            'clang-apply-replacements-6.0 must be installed and found in the path (install clang-tools-6.0)'

    cmd = [
        'python3', '-u',
        runner_script_path,
        f'-j{concurrency}',
        f'-p={abspath(build_root)}',
        '-header-filter=.*(apps|libs).*\\.hpp$',
        '-quiet',
        f'-clang-tidy-binary={clang_tidy_path}',
        # Hacky workaround: we do not need clang-apply-replacements unless applying fixes
        # through run-clang-tidy-6.0.py (which we cannot do, as it would alter vendor
        # libraries). Unfortunately, run-clang-tidy will refuse to function unless it
        # thinks that clang-apply-replacements is installed on the system. We pass
        # a valid path to an arbitrary executable here to placate it.
        f'-clang-apply-replacements-binary={clang_tidy_path}',
        f'-export-fixes={output_file}',
        '.']

    print('\nPerform static analysis')
    print(subprocess.check_output(['clang-tidy-6.0', '-list-checks']).decode())
    print('\nCommand:')
    print(f'  {" ".join(cmd)}\n')

    print('Working...')
    with tempfile.SpooledTemporaryFile() as f:
        exit_code = subprocess.call(cmd, cwd=project_root,
                                    stdout=f, stderr=f)
        if exit_code != 0:
            print('clang-tidy reported an error')
            f.seek(0)
            print(f'\n{f.read().decode()}\n')
            sys.exit(1)

    print('Done.')

    print('Processing results...')
    files_diagnostics = read_static_analysis_yaml(
        project_root, build_root, output_file)

    if len(files_diagnostics):
        for file, data in files_diagnostics.items():
            print('\n' + (10 * '-'))
            print(f'Static analysis check(s) failed in:\n  {file}\n')
            for d in data['diagnostics']:
                print(f'  Line: {d["line"]}')
                print(f'  Message: {d["message"]}')
                print(f'  Check: {d["check"]}')

        print(
            f'\nStatic analysis found {sum([len(d["diagnostics"]) for f, d in files_diagnostics.items()])} violation(s) in {len(files_diagnostics)} file(s)')

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
