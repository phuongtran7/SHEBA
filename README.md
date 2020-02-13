# Near-Future Observation Lens "SHEBA"

`SHEBA` is a simple project that gathers and displays traffic information from user's GitHub account. `SHEBA` uses [cpprestsdk](https://github.com/Microsoft/cpprestsdk), [RapidJSON](https://github.com/Tencent/rapidjson), [tabulate](https://github.com/p-ranav/tabulate) and [toml11](https://github.com/ToruNiina/toml11).

## Building
### Windows

If you don't want to compile the program by yourself, you can head over the [releases](https://github.com/phuongtran7/SHEBA/releases) tab a get a pre-compiled version.

1. Install cpprestsdk with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
    * `vcpkg install cpprestsdk:x64-windows`
2. Clone the project with submodules: `git clone --recurse-submodules https://github.com/phuongtran7/SHEBA.git`.
3. Build the project.

## Usage
1. Generate personal access token for GitHub account ([https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line)).
2. Prepare a `Secrets.toml` file with content:
```
User = "GitHub username"
Token = "Personal token"
```
and then put it next to the compiled executable.

4. Start the executable.