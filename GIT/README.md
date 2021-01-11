## Retrigger hooks by running `commit --amend` on multiple commits
```Shell
git rebase -i --exec "git commit --amend --no-edit" <first-commit>~
```
