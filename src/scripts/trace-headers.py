#! /usr/bin/python3

import os
import shlex
import subprocess
import sys

from typing import List, Iterable, Set


def get_deps_for_single_file(filename: str, extra_flags: List[str]) -> Iterable[str]:
    command = ["g++", "-Igame", "-Iengine", "-Ishared", "-I/usr/include/SDL2"]

    # append additional compiler flags
    command += extra_flags

    # generate make-style dependency lists
    command.append("-M")

    command.append(filename)

    print(" ".join([shlex.quote(i) for i in command]), file=sys.stderr)

    out = subprocess.check_output(command).decode()

    if os.environ.get("DEBUG", None):
        print(out, file=sys.stderr)

    for line in out.splitlines():
        parts = line.split(" ")

        # handles indentation and other annoying whitespace
        try:
            parts.remove("")
        except ValueError:
            pass

        # we don't need the escaping at the line ends
        try:
            parts.remove("\\")
        except ValueError:
            pass

        for part in parts:
            # ignore own object file
            if part.endswith(".o:"):
                continue

            # we don't need code dependencies
            if part.endswith(".cpp"):
                continue

            # same goes for system-wide includes
            if part.startswith("/usr"):
                continue

            yield part


def recursive_trace(filename: str, extra_flags: List[str]):
    # note: since files can include each other in this weird codebase, it's important to terminate once no more new
    # includes are found

    def get_deps(f: str, deps: Set[str] = None):
        if deps is None:
            deps = set()

        new_deps = set(get_deps_for_single_file(f, extra_flags))

        # remove all existing includes as well as the own file (unless it's source code) from the list before adding
        # them to the set to be able to make sure the for loop only runs on new files
        if not f.endswith(".cpp"):
            deps.add(f)

        for dep in deps:
            try:
                new_deps.remove(dep)
            except KeyError:
                pass

        deps = deps.union(new_deps)

        # if there's any more includes, we can recurse down, otherwise this function will terminate
        for dep in new_deps:
            deps = deps.union(get_deps(dep))

        return deps

    return get_deps(filename)


def main():
    deps = set()

    files = []
    flags = []

    for i in sys.argv[1:]:
        if i.startswith("-"):
            flags.append(i)
        else:
            files.append(i)

    if not files:
        print("Usage:", sys.argv[0], "file [file...] [-mycompilerflag]", file=sys.stderr)
        print("Note: this tool is supposed to run from Blue Nebula src/ directory")
        return 1

    for file in files:
        deps = deps.union(recursive_trace(file, flags))

    deps = list(deps)
    deps.sort()
    print("\n".join(deps))


if __name__ == "__main__":
    sys.exit(main())
