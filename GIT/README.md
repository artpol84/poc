## Retrigger hooks by running `commit --amend` on multiple commits
```Shell
git rebase -i --exec "git commit --amend --no-edit" <first-commit>~
```

## Change the author info
```Shell
git rebase -i --exec 'git commit --amend --author="Author Name <author@email.com>" --no-edit' <first-commit>~
```
