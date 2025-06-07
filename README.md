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

---

## Creating Custom Commands

**Yes!** You can define new commands by modifying the `pmp_config.json` file. By default, it looks like this:

```json
{
    "project_name": "",
    "tasks": [],
    "description": "This is a configuration file for the PMP project management tool.",
    "version": "1.0",
    "env_file": ".env",
    "functions": {
        "*": "python main.py"
    },
    "dependencies": {},
    "dependencies_secundary": {}
}
```

Suppose you install `pytest` and want to create a shortcut to run tests with:

```bash
pmp run test
```

You would update `pmp_config.json` like this:

```json
{
    "project_name": "",
    "tasks": [],
    "description": "This is a configuration file for the PMP project management tool.",
    "version": "1.0",
    "env_file": ".env",
    "functions": {
        "*": "python main.py",
        "test": "pytest"
    },
    "dependencies": {},
    "dependencies_secundary": {}
}
```

# environment variables
You can set environment variables making a file with the name ".env" (you can change this changing the file pmp_config.json in the parte "env_file") this you can define your variables and if you want to use the variable, you must use the sintaxis {name_variables}

## examples
file .env
```.env
HOST_SERVER = 127.0.0.1
PORT_SERVER = 7777
```

file pmp_config.json
```json
{
    "project_name": "",
    "tasks": [],
    "description": "This is a configuration file for the PMP project management tool.",
    "version": "1.0",
    "env_file": ".env",
    "functions": {
        "*": "uvicorn main:app --host {HOST_SERVER} --port {PORT_SERVER} --reload"
    },
    "dependencies": {
        "fastapi": {
            "version": "0.115.12"
        },
        "uvicorn": {
            "version": "0.34.3"
        }
    },
    "dependencies_secundary": {...}
}
```

---

## Bugs:

-   in "dependencies_secondary" there can be a dependency twice.
-   the sintaxis with "[]" it doesn't work properly.(example: "fastapi[standart]")
