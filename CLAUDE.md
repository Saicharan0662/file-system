# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A low-level design (LLD) practice exercise: an in-memory file system in a single
C++ translation unit. `prompt.txt` is the problem statement and defines scope —
paths are assumed well-formed, and only four operations are in scope: create file
at a path, create directory at a path, read a file's content, list a directory's
immediate contents. There is no build system, no test framework, and no external
dependencies.

## Build and run

```sh
g++ -std=c++17 -Wall -Wextra main.cpp -o filesystem   # clang++ works too
./filesystem
```

`main()` is the driver and the only verification: it exercises every operation
plus edge cases (deep auto-creation, reading a missing file, listing a missing
directory) and prints heavily traced output. To "test" a change, recompile and
eyeball the run against the commented expectations in `main()`. The `filesystem`
binary is a build artifact and is gitignored.

## Architecture

Three layers, all in `main.cpp`:

- **`File` (abstract) / `TextFile`** — a leaf holding `name` + string `content`.
  `setContent` is private; content is set once via the constructor.
- **`Directory`** — a tree node with *two separate* `unordered_map`s:
  `name -> unique_ptr<File>` and `name -> unique_ptr<Directory>`. It owns its
  children. Note the asymmetry: `Directory`'s own `readFile`/`createFile` operate
  on files in *this* directory, but `listContents(dirName)` and
  `getSubDirectory` look one level *down* into a named child.
- **`FileSystem`** — owns `root` and is the only public entry point. It converts
  `"/a/b/c.txt"` into a token list, then `navigateToDirectory` walks it.

### The key behavior to understand before changing traversal

`navigateToDirectory(path)` **pops the last path segment** (treated as the
target's own name, extracted separately by `extractFileDirName`) and walks the
rest from `root`. Crucially, **it creates any missing intermediate directory as
it walks** — it never fails on a missing path. Consequences:

- `createFile("/docs/2024/report.txt", ...)` auto-creates `docs` and `docs/2024`.
- Because *every* path-based call routes through `navigateToDirectory`,
  `readFile` and `listContents` on a non-existent path will silently mutate the
  tree (create directories) before hitting their "does not exist" branch. Any fix
  to not-found handling has to account for this.

## Git workflow for this repo

Commit after every file modification with a descriptive message (what changed and
why — not "update"). Do not `git push` (no remote is configured) unless asked.
