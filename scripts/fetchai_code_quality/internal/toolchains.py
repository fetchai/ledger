import os
import platform
import shutil


class ClangToolchain:
    def __init__(self):

        # init. internal values
        self._clang_tidy_path = None
        self._run_clang_tidy_path = None
        self._clang_apply_replacements_path = None

        # trigger platform specific toolchain detection
        platform_name = platform.system()
        if platform_name == 'Darwin':
            self.__detect_macos()
        elif platform_name == 'Linux':
            self.__detect_linux()
        else:
            raise RuntimeError('Unsupported system: {}'.format(platform_name))

        # validate all the paths
        self._clang_tidy_path = self.__validate_path(self._clang_tidy_path)
        self._run_clang_tidy_path = self.__validate_path(self._run_clang_tidy_path)
        self._clang_apply_replacements_path = self.__validate_path(self._clang_apply_replacements_path)

        # check that the detection has been completed
        if any([self._clang_tidy_path is None, self._run_clang_tidy_path is None, self._clang_apply_replacements_path is None]):
            raise RuntimeError('Unable to detect compatible clang toolchain')

        print('Successfully detected clang toolchain: ', self._clang_tidy_path)

    @property
    def clang_tidy_path(self):
        return self._clang_tidy_path

    @property
    def run_clang_tidy_path(self):
        return self._run_clang_tidy_path

    @property
    def clang_apply_replacements_path(self):
        return self._clang_apply_replacements_path

    def __detect_macos(self):
        BREW_TOOLCHAIN_PATH = '/usr/local/Cellar/llvm@6'

        if not os.path.isdir(BREW_TOOLCHAIN_PATH):
            print('Unable to detect brew compatible toolchain on your system. Consider installing with `brew install llvm@6`')
            return

        # to make this a little tolerant to minor update in the LLVM 6 backend do not hard code
        # the exact version but simple assume that it is the first directory underneath. In the case
        # of more than one (or empty) panic!
        contents = os.listdir(BREW_TOOLCHAIN_PATH)
        if len(contents) == 1:
            toolchain_base_path = os.path.join(BREW_TOOLCHAIN_PATH, contents[0])

            # detect and paths
            self._clang_tidy_path = os.path.join(toolchain_base_path, 'bin', 'clang-tidy')
            self._run_clang_tidy_path = os.path.join(toolchain_base_path, 'share', 'clang', 'run-clang-tidy.py')
            self._clang_apply_replacements_path = os.path.join(toolchain_base_path, 'bin', 'clang-apply-replacements')

    def __detect_linux(self):
        self._clang_tidy_path = shutil.which('clang-tidy-6.0')
        self._run_clang_tidy_path = shutil.which('run-clang-tidy-6.0.py')
        self._clang_apply_replacements_path = shutil.which('clang-apply-replacements-6.0')

    @staticmethod
    def __validate_path(path):
        if path is not None and os.path.isfile(path):
            return path
