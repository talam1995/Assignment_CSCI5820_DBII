# Homework 1: B+ Tree Index

All code is in a single file, `bplus.cpp`.
Each run starts with an empty tree, executes the commands read from standard input in order, and exits.
Nothing is stored on disk

## Requirements

A C++17 compiler: `g++` 7 or newer (Linux, macOS, or MinGW/MSYS2 on Windows).

## Compile

```
g++ -std=c++17 -O2 -o bplus bplus.cpp
```

## Run

```
./bplus init <d> < test.txt
```

`d` (an integer, 1 or larger) sets the capacity of every node.

- a leaf holds at most `2d` key-pointer pairs
- an internal node holds at most `2d` keys and `2d + 1` child pointers
- every node except the root is at least half full (at least `d` keys)

For example, with `d = 2`:

```
./bplus init 2 < test.txt
```

A missing or invalid `d` prints a usage message on standard error and exits.

### Running on Windows

The compiled program is `bplus.exe`.
In Command Prompt use `bplus init 2 < test.txt`.
PowerShell does not support `<` for input, so from a PowerShell terminal (the VS Code default) use:

```
cmd /c "bplus.exe init 2 < test.txt"
```

## Commands

| Command | Output |
|---|---|
| `INSERT <key> <pointer>` | `(10, 201340) inserted` or `(10, 201340) not inserted. 10 found.` |
| `SEARCH <key>` | `10 found, point is 201340` or `10 not found` |
| `DELETE <key>` | `10 deleted.` or `10 not found, not deleted.` |
| `RANGESEARCH <k1> <k2>` | `found` followed by one `(key, pointer)` line per record in [k1, k2], or `no records in the range [k1, k2]` |
| `PRINT` | The tree level by level, e.g. `Level 0: [(10)]` then `Level 1: [(5: 201451)] [(10: 201340) (20: 409678)]`. `[]` is one node; `(k)` is a key in an internal node, `(k: p)` a key-pointer pair in a leaf. |
| `PRINT STATISTICS` | `Tree Height: N`, `Total Nodes: N`, `Total Keys: N` |

Keys and pointers are 32-bit integers and keys are unique.
Commands are case-insensitive; blank lines are skipped.
A malformed line is reported onstandard error and skipped, so standard output contains only the command results above.

### Notes on the output

- Tree height counts levels: a tree that is a single leaf has height 1.
- `delete(int key)` operation is the method `remove(int key)` in `bplus.cpp`, because `delete` is a reserved word in C++.
- Pointers may be any integer, including -1: a stored pointer of -1 is still reported as found by `SEARCH`.
- Following the textbook, deleting a key does not change the separator keys in internal nodes unless a redistribution or merge happens, so `PRINT` can show an internal key that is no longer stored in a leaf. Searches are unaffected.

## Files

- `bplus.cpp`: source code
- `README.md`: this file
- `DesignDocument.pdf`: design document
- `test1.txt`: the example command sequence from the assignment (run with `init 1`)
- `test2.txt`: growth to three levels, then redistribution and merges (run with `init 1`)
- `test3.txt`: builds the tree of Figure 10.9 in the textbook and replays its
  insert and delete examples, Figures 10.13, 10.16 and 10.18 (run with `init 2`)