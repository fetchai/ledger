# Git branching model

## Master branch

The default branch for a project is the master branch. This branch must always be in a stable state: the build must be successful, all tests must pass, style must follow the [style guide](cplusplus-style-guide.md) and static analysis should not give warnings or errors.

Commits representing a release will be tagged as described in the [release strategy](release-strategy.md#tagging-and-version-numbers).


## Development forks

In order to keep the main repo clear from development branches, developers must create a fork of any project they are planning on contributing to. In addition, for projects which are or will be open sourced this is the easiest way to begin developing.
The only scenario when it may be currently necessary to push branches directly to the repository is where the branch changes the Jenkinsfile. These should not be picked up from a fork for security reasons.

Once the fork has been created, it is expected that the user would create a branch to track each new feature, bugfix, etc,  being made. If any of these tasks are complex, they can be addressed in multiple increments that are individually reviewed and merged. See [Integration (pull requests)](#integration-pull-requests) for more information about merging development branches.

A task is only considered done once all the branches associated with it have been merged into the master branch; this encourages tasks to be well defined so they can be assessed and merged quickly, with the minimum amount of code to be integrated every time.

Long-lived development branches are strongly discouraged but if necessary, its creation must be discussed to try to find alternatives. Long-lived branches tend to strongly diverge from the master branch, making the review and integration complicated and tedious.

To further aid clarity we advise using the following prefixes based on the nature of the change being made:

 * `feature`: New feature
 * `bugfix`: Fixing a bug in the code
 * `refactor`: Not a change in functionality, but a general improvement to code health
 * `backport`: Porting a fix from the [master branch](#master-branch) to an existing [release branch](#release-branches)

Jira ticket numbers should be present in the branch names, to allow the Jira <-> GitHub plugin to link them automatically. But they must not be used as the whole branch name, because they are not descriptive enough out of Jira's context. In addition to branches, pull requests can also be linked as described in [Integration (pull requests)](#integration-pull-requests).

**Examples**

 * `feature/ldgr-111-transaction-fees` - A branch for the implementation of transaction-based fees
 * `bugfix/ldgr-222-http-empty-buffer` - A branch for fixing a specific issue
 * `refactor/ldgr-333-update-state-machine` - A branch for updating a state machine


## Release branches

Release branches will follow the prefix `release` and are created at the point of a release from the master branch. It is expected that after this point, no feature updates will be made to these branches, and any further updates will be only bug fixes.

**Examples**

 * `release/v0.1.x`
 * `release/v0.2.x`
 * ...
 * `release/v1.0.x`


## Integration (pull requests)

At the point when a change has been made and wants to be reintegrated, it is expected that the developer would push all the updates to their personal fork and then create a pull request to merge all the changes into the code base. The pull request title must contain the Jira ticket number(s) it is associated with, to allow the Jira <-> GitHub plugin to link them automatically.

At this point, there is a chance for fellow developers on the project to review these changes and suggest modifications and/or improvements. This is also the usual point to run integrated CI checks against the changes.

At the conclusion of this process, when code reviews and CI checks have been completed the developer can then merge their changes into the master branch.

**Note**: This process works best when the changes being generated are relatively small, are limited to a single operational change. One comment from the review process might be that this integration should be split into a number of parts to aid review and testing.
