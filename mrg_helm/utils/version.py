from importlib.metadata import version

from mrg_helm._version import __version_tuple__

def get_version():
    return version('mrg-helm')

def get_short_version():
    major, minor, patch, dev, id = __version_tuple__
    dev = dev[-1:]
    id = id[:3]

    short_version = f'{major}.{minor}.{patch}+{dev}.{id}'
    return short_version