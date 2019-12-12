# Release strategy

## Tagging and version numbers

A tag indicates that a commit on the master or a release branch is a public release.

In order to imbue the version numbers with extra information it is important to follow [semantic versioning](https://semver.org/). In general version numbers will have the form below:

```
v<major>.<minor>.<patch>-[pre release]-[git info]
```

Where:

* Major, minor and patch are decimal numbers
* Pre-release is one of the following labels: alpha, beta, rc (release candidate)

Where possible, we should include the commit based information into versions that are being generated. The strategy is to use the git command with tags directly as shown below:

```bash
$ git describe --always --dirty=-wip
v0.9.0-alpha2-17-g2c43d712
```

## Packaging

TODO

## Release notes

TODO
