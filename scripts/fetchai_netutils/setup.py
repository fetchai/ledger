from setuptools import setup, find_packages

setup(
    name='fetchai-netutils',
    version='0.0.6a2',
    description='Tools and utilities for interacting with the fetch network',
    url='https://github.com/fetchai/',
    author='Ed FitzGerald',
    author_email='edward.fitzgerald@fetch.ai',
    packages=find_packages(exclude=['contrib', 'docs', 'tests']),  # Required
    install_requires=[],
    extras_require={
        'dev': ['check-manifest'],
        'test': ['coverage'],
    },
    entry_points={
        'console_scripts': [
            'swarm=fetch.cluster.apps.swarm:main',
        ],
    },
)
