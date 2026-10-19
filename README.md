# In-Memory File System (LLD)

A low-level design exercise: an in-memory file system written in a single C++17
translation unit (`main.cpp`). It supports files and directories, where a
directory can hold both files and other directories, and every operation is
addressed by a Unix-style path such as `/docs/notes.txt` or `/docs/projects`.

## Authorship

**The architecture, every design decision, and the original implementation are
my own work.** I designed the class hierarchy, the ownership model, the path
traversal strategy, and wrote the code for `File`, `TextFile`, `Directory`, and
`FileSystem` myself.

[Claude](https://claude.com/claude-code) (an AI assistant) was used **only to
write the driver code**, the `main()` function that exercises the operations and
edge cases, and to help with documentation.

## Problem statement

Design an in-memory file system that supports:

- Creating a file at a given path
- Creating a directory at a given path
- Reading a file's content given its path
- Listing everything directly inside a given directory

Paths are assumed to be well-formed; malformed input handling is out of scope for
this first pass.

## Build and run

```sh
g++ -std=c++17 -Wall -Wextra main.cpp -o filesystem   # clang++ works too
./filesystem
```

There are no external dependencies, build system, or test framework. `main()`
is the driver: it runs every operation plus edge cases and prints traced output
that can be checked against the comments in `main()`.

## Architecture

The design has three layers:

```
FileSystem                      (public entry point, path handling)
  └── root : Directory
        ├── files          : unordered_map<string, unique_ptr<File>>
        │     └── TextFile (name, content)
        └── subDirectories : unordered_map<string, unique_ptr<Directory>>
              └── Directory ... (recursive)
```

### `File` (abstract) and `TextFile`

- `File` is an abstract interface: `getName`, `setName`, `readFile`,
  `setContent`, `getFileType`.
- `TextFile` is the concrete leaf. It stores a `name`, string `content`, and a
  `fileType` (`.txt`).
- `setContent` is **private** in `TextFile`: content is set once, through the
  constructor, and can't be changed from outside afterwards.
- Making `File` abstract leaves room for other file types (binary, image, and
  so on) without changing `Directory` or `FileSystem`.

### `Directory`

- A tree node with **two separate maps**:
  - `files`: `name -> unique_ptr<File>`
  - `subDirectories`: `name -> unique_ptr<Directory>`
- Keeping files and directories in separate maps keeps lookups O(1) and
  type-safe, with no runtime type checks or casts needed.
- A directory **owns its children** through `unique_ptr`. Destroying a
  directory frees its whole subtree, and nothing leaks.
- Operations:
  - `createFile(name, content)` / `readFile(name)` work on files in *this*
    directory.
  - `createDirectory(name)` / `addSubDirectory(name, dir)` add a child
    directory.
  - `getSubDirectory(name)` returns a raw, non-owning pointer to a child, or
    `NULL` if it doesn't exist.
  - `listContents(dirName)` looks **one level down** into the named child and
    prints its subdirectories (with a trailing `/`) followed by its files.

### `FileSystem`

- Owns the `root` directory, which is created in the constructor, and is the
  only public entry point.
- Path handling:
  - `transformToList(path)` splits `"/a/b/c.txt"` into `["a", "b", "c.txt"]`,
    skipping empty tokens.
  - `extractFileDirName(path)` returns the last segment, which is the target's
    own name.
  - `navigateToDirectory(path)` drops the last segment and walks the rest from
    `root`, returning the **parent** directory of the target.
- Public API:

  | Method | Behavior |
  | --- | --- |
  | `createFile(path, content)` | Navigates to the parent and creates the file there |
  | `createDirectory(path)` | Navigates to the parent and creates the directory there |
  | `readFile(path)` | Navigates to the parent and prints the file's content |
  | `listContents(path)` | Navigates to the parent and lists the target directory's immediate contents |
  | `getDirectoryName()` | Returns the root directory's name |

### Path traversal: auto-creating intermediate directories

`navigateToDirectory` **creates any missing intermediate directory** as it
walks, similar to `mkdir -p`. So a single call such as

```cpp
fileSystem.createFile("/docs/2024/report.txt", "Q1 numbers");
```

creates `docs` and `docs/2024` on the way if they don't already exist, then
places `report.txt` inside.

Because every path-based operation routes through `navigateToDirectory`,
`readFile` and `listContents` on a non-existent path also create the missing
intermediate directories before reporting that the target doesn't exist. This
is a known trade-off of the current design, and it's where to start if
not-found handling is made strict later.

## Design summary

| Decision | Rationale |
| --- | --- |
| Abstract `File` with a concrete `TextFile` | Extensible to new file types |
| Separate maps for files and directories | O(1) lookup, no casting, clear typing |
| `unique_ptr` ownership down the tree | Automatic, leak-free cleanup of subtrees |
| Raw non-owning pointers for traversal | Cheap navigation without transferring ownership |
| Path tokenized once, then walked | Simple, predictable traversal logic |
| Auto-create on traversal | Deep paths are created in a single call |
| Private `setContent` | File content is immutable after creation |
