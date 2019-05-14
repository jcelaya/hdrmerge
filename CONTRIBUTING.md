## Thank You
Thank you for showing interest in contributing to HDRmerge. It is people such as yourself who make this program and project possible.

## Contributing as a Programmer
- Announce and discuss your plans in GitHub before starting work.
- Work in a new branch. Fork if necessary.
- Keep branches small.
- Use C++11.

### Naming and Comments
The naming isn't homogeneous throughout the code but here is a rough guideline:
- *Identifiers* (variables, functions, methods, keys, enums, etc.) should be clear and unambiguous. Make them as long as necessary to ensure that your code is understandable to others and to your future self.
- *Types* (classes, structs, enums, typedefs...) should be named with `UpperCamelCase`.
- *Functions* and *methods* should be named with `lowerCamelCase`.
- *Variables* should be named with `lower_underscores` to avoid conflicts.
- *Enum values* should be named with `UPPER_UNDERSCORES`.
- Add *comments* when needed. Not too little but not too much. Try to be reasonable and keep a good balance.
- Give an overview of the code block/algorithm. If the identifiers are clearly describing the intent, single lines don't need to be commented.
- Be consistent, even when not sticking to the rules.

### Refactoring Code
Don't shy away from refactoring code, it helps more than you think.
- Change the naming and add comments when they don't match with the guidelines above.
- C++11 gives many opportunities to simplify code (e.g. default initializing with = {} or default returning with return {};).
- When using `auto` think a bit about the reviewers reading diffs and guessing types. It's a double-edged sword.
- Help the compiler and reviewers by using `const` as default for almost anything.
- Reduce scopes. This also means moving private functions with little or no dependency to `this` to an anonymous namespace inside the implementation.
- Try to separate "code refactoring" commits from actual functionality changes.
