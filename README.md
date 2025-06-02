# Python Manage Package (PMP)

**PMP** is a tool for developers to manage Python packages easily, using commands similar to those of `npm`.

---

## Installation

If you just want to use this tool, run the following command:

```bash
make install
```

If you want to build this project locally on your PC, use the following command:

```bash
sudo apt install nlohmann-json3-dev
```

This installs a required package to build the project. Then you can build it using:

```bash
make build     # Builds the project
make install   # Replaces the previous 'pmp' file with the new build and installs it
```

---

## How to Use PMP

### Initialize a New Project

To start a new Python project with PMP, navigate to an empty folder and run:

```bash
pmp init
```

This command initializes the project by creating a virtual environment, necessary configuration files, and an example.

To try the example, run:

```bash
pmp run

# Output:
Welcome to the PMP Project Management Tool!
```

This message is printed by `main.py`.

---

### Installing a Package

To install a new package in your project:

```bash
pmp install <package>

# Example:
pmp install flask
```

This installs the `flask` package for the project and makes it available for use.

To uninstall a package:

```bash
pmp uninstall <package>
```

> **Note**: Currently, this only uninstalls the specified package. Dependencies of that package are **not** removed yet.

---

## Creating Custom Commands

**Yes!** You can define new commands by modifying the `pmp_config.json` file. By default, it looks like this:

```json
{
    "project_name": "",
    "tasks": [],
    "description": "This is a configuration file for the PMP project management tool.",
    "version": "1.0",
    "functions": {
        "*": "python main.py"
    },
    "dependencies": []
}
```

Suppose you install `pytest` and want to create a shortcut to run tests with:

```bash
pmp run test
```

You would update `pmp_config.json` like this:

```json
{
    "dependencies": [
        "pytest==8.3.5"
    ],
    "description": "This is a configuration file for the PMP project management tool.",
    "functions": {
        "*": "python main.py",
        "test": "pytest"
    },
    "project_name": "",
    "tasks": [],
    "version": "1.0"
}
```

---

## Future Goals

- Automatically install the dependencies of installed packages and list them in `pmp_config.json`.
- Automatically remove unused dependencies when a package is uninstalled.
