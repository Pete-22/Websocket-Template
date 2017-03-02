##Sending data using websockets

###Project goals
Simple template that can be used in a project or as a stand-alone program that sends and recieved messages using websokets and Json.

#####Devices used in my test
- Windows 10 laptop
- Raspberry Pi 3

#### Software / Libraries used
- Raspbian GNU / Linux 8 (Jessie)
- [Boost](http://www.boost.org/)
- [Websocket++](https://github.com/zaphoyd/websocketpp)
- [Nlohmann Json](https://github.com/nlohmann/json)
- g++ 4.9 or higher

####Initial configuration (Linux / Raspberry Pi)
We start by updating the OS with the following commands:

- `sudo apt-get update`
- `sudo apt-get upgrade`
- `sudo reboot`

We update the Raspberry Pi firmware:

- `sudo apt-get install rpi-update`
- `sudo rpi-update`
- `sudo reboot`

After that we will install our libraries by following the installation instructions online:

- [Websocket++](https://github.com/zaphoyd/websocketpp/wiki/Build-on-debian)
- Boost: `sudo apt-get install libboost-all-dev` (Also good to have)
- Download the websocket++ repo from github and extract it to `/home/pi/`
- [Nlohmann Json](https://github.com/nlohmann/json), download project, place the json.hpp file into our project folder:
- `cd ~/Downloads`
- `git clone https://github.com/nlohmann/json`

Link the libraries, add the following lines to `/etc/ld.so.conf.d/libc.conf`:

- `sudo nano /etc/ld.so.conf.d/libc.conf`
- `/usr/local/boost_1_61_0/libs` (If newer verions was installed, change name accordingly)
- `/usr/local/boost_1_61_0/libbin` (If newer verions was installed, change name accordingly)

Add the following line to `/etc/ld.so.conf.d/local.conf`

- `sudo nano /etc/ld.so.conf.d/local.conf`
- `/usr/local/boost_1_61_0/libbin/lib` (If newer verions was installed, change name accordingly)
- run `sudo ldconfig`

After all the libraries that we need have been installed, we are ready to build our code.

- `cd ~/Downloads/PSMove_Websocket`
- Use the makefile to compile the code (The makefile might need to be edited according to the username of your system!).
- `make`
- The compiling can take a couple of minutes on a Raspberry Pi, seconds on a desktop computer.
- Once the program has been compiled we can run it with the command:
- `sudo ./psmove_send_poll_data`

####Initial configuration (Windows / Visual Studio), Project Compiles

Download the libraries from Websocket++ and Boost for windows, follow online instructions
- [Websocket++](https://github.com/zaphoyd/websocketpp/wiki)
- [Boost](http://www.boost.org/doc/libs/1_62_0/more/getting_started/windows.html)
- [Nlohmann Json](https://github.com/nlohmann/json), download project, place the json.hpp file into project

Linkoing the libraries in Visual Studio 2015 (Mine look like this)
- D:\Includes\json-develop;D:\Includes\websocketpp-master;D:\Includes\boost_1_61_0;%(AdditionalIncludeDirectories)
- D:\Includes\boost_1_61_0\stage\lib;%(AdditionalLibraryDirectories)

In Visual Studio Project properties I also have the following:
- Preprocessor Definitions: WIN32;_DEBUG;_CONSOLE;_SCL_SECURE_NO_WARNINGS;%(PreprocessorDefinitions)

####Using the program
- Input the IP address and port according to the example on screen and then press _"**enter**"_.
- Next the program will connect to the server, send an initial message and then wait for the start message to be received from the server.
- After receiving the start message the program will start to poll.
- To pause the program, you can send a _"**pause**"_ command through websockets as Json.