Before generating or modifying any code, read and follow the rules in [docs/contributing.md](../docs/contributing.md).

Unless prompted not to, any locally run agent must do its work in a dedicated git worktree. 
On Windows, create agent worktrees as visible sibling directories of the main checkout rather than under a hidden repo-local `.worktrees` folder so VS Code can recognize them. *ALWAYS* create a new worktree for new task, as existing ones might be used by other agents in the background.
