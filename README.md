# arcdps killproof.me plugin
A Plugin for arcdps, that is loading killproof.me info and displaying it ingame.

## Installation
Make sure arcdps is installed. If arcdps is not installed, this plugin is simply not loaded and does nothing.  
Download the latest version from [github releases](https://github.com/knoxfighter/arcdps-killproof.me-plugin/releases/latest).  
Then put the .dll file into the same folder as arcdps (normally `<Guild Wars 2>/bin64`).  
To disable, just remove the .dll or move it to a different folder.

Alternative, install it with the [Guild Wars 2 Addon loader](https://github.com/fmmmlee/GW2-Addon-Manager/), like multiple other plugins.

## Usage
Don't be a dick and give people a chance!

This plugin will load the killproof.me profiles, from everybody in your group, when they are on the same map, than you are. Those information, can be accessed in the killproof.me windows. No information is shown, when the individual has no killproof.me account or it is on private.

Two way to open the window:  
- Open the arcdps options panel (Alt+Shift+T by default) and enable the "Kproofs" checkbox. It can be found in the "Killproof.me" category.
- Use the hotkey Alt+Shift+K. This can be adjusted in the Settings menu (opened also in the arcdps options panel, in the "Killproof.me" category.

To close the window, just press the X on the top right, press Escape or use the hotkey Alt+Shift+K again.

In the settings you can choose, what will be listed for each user.

The UI is updated to a table version, as soon, as arcdps is updating to the new ImGui version 1.80 (will be around christmas).

![Ingame screenshot](screenshot.png)

## Contact
For any errors, feature requests and question, simply open a new issue here.  

## Development

This plugin was developed with visual studio 2019 and vcpkg. It has only one dependency, that has to be installed in vcpkg.

Setup vcpk and install cpr for 64-bit windows programs:
```
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install cpr:x64-windows
```

Then download this project and open it with Visual Studio. Everything should be set up and you should be able to compile it.  
The default output directory is `C:\Program Files\Guild Wars 2\bin64\`, you can change that in `Properties -> Configuration Properties -> General -> Output Directory`. Also, running the Local Windows Debugger, will start `C:\Program Files\Guild Wars 2\Gw2-64.exe` and attaches the debugger to it. Make sure to NOT use your normal account, cause you could get banned for it!

If things fail that ways, i have more setups done, than i remember, just open an issue and i will sort them out.

## LICENSE

This project is licensed with the MIT License.

### Dear ImGui
[Dear ImGui](https://github.com/ocornut/imgui) is also licensed with the MIT License and included as git submodule to this project.

### json
[json](https://github.com/nlohmann/json) is a json library created by nlohmann and licensed with the MIT License. It is included into this project with a single file [json.hpp](/json.hpp).

### cpr
[cpr](https://github.com/whoshuu/cpr) is a http library, that is licensed with the MIT License. It is included with vcpkg and itself has dependencies.
