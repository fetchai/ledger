import os
import sys


def output(*args):
    text = ' '.join(map(str, args))
    if text != '':
        sys.stdout.write(text)
        sys.stdout.write('\n')
        sys.stdout.flush()


def verify_file(filename):
    if not os.path.isfile(filename):
        output("Couldn't find expected file: {}".format(filename))
        sys.exit(1)


def yaml_extract(test, key, expected=True, expect_type=None, default=None):
    """
    Convenience function to remove an item from a YAML string, specifying the type you expect to find
    """
    if key in test:
        result = test[key]

        if expect_type is not None and not isinstance(result, expect_type):
            output(
                "Failed to get expected type from YAML! Key: {} YAML: {}".format(
                    key, test))
            output("Note: expected type: {} got: {}".format(
                expect_type, type(result)))
            sys.exit(1)

        return result
    else:
        if expected:
            output(
                "Failed to find key in YAML! \nKey: {} \nYAML: {}".format(
                    key, test))
            sys.exit(1)
        else:
            return default
