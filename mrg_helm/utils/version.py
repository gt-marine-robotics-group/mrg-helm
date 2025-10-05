from importlib.metadata import version


def get_version_hash():
    version_string = version('mrg-helm')
    return version_string