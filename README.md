# botHub -> work in progress..

## Installing MOC for moc_irc_api

1. Ensure the Qt development package is installed:
   ```bash
   sudo apt-get update
   sudo apt-get install qtbase5-dev
   ```
2. Generate the moc file for your IRC API header:
   ```bash
   moc -o moc_irc_api.cpp includes/irc_api.h
   ```
3. Compile the generated moc_irc_api.cpp along with your project sources.

## Resolving Shared Library Issues

If you get an error similar to:
```
./github-bot: error while loading shared libraries: libpugixml.so.1: cannot open shared object file: No such file or directory
```
try the following:

1. Install the libpugixml package:
   ```bash
   sudo apt-get update
   sudo apt-get install libpugixml1
   ```
2. If the package is unavailable or you built pugixml from source, ensure the library path is correctly set. For example:
   ```bash
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   ```
3. Re-run your application:
   ```bash
   ./github-bot start
   ```
