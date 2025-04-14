
# Table of Contents

-   [WhatWasiDoing](#whatwasidoing)
-   [Description](#description)
-   [Features](#features)
-   [Screenshots](#screenshots)
-   [Dependencies](#dependencies)
-   [Configuration](#configuration)
-   [Keyboard Controls](#keyboard_controls)
-   [Building](#building)
-   [Debug Mode](#debug_mode)
-   [System Requirements](#system_requirements)
-   [Limitations](#limitations)
-   [License](#license)



<a id="whatwasidoing"></a>

# WhatWasiDoing


<a id="description"></a>

# Description

Display TODO items from specified items in a floating window.


<a id="features"></a>

# Features

-   Floating window that stays on top of other windows.
-   Reads KEYWORD items from configured files.Do
-   Customizable background color.Do
-   Keyboard shortcuts for window management.Do
-   Configurable keywords and target files.Do


<a id="screenshots"></a>

# Screenshots

![img](./images/screenshot_20250414_165314.png)


<a id="dependencies"></a>

# Dependencies

-   [SDL2](https://github.com/libsdl-org/SDL/releases)
-   [SDL2\_ttf](https://github.com/libsdl-org/SDL_ttf/releases)


<a id="configuration"></a>

# Configuration

Create a `.currTasks.conf` file in your home folder with the following in it:

    file = "path/to/your/todo.org"
    keyword = "TODO"

You can specify multiple files and keywords

File listing order is relevant to which items are displayed on the first line

You can check where your `home` folder is using:

-   Powershell: `Write-Host $HOME`
-   CMD: `echo %userprofile%`
-   bash: `echo $HOME`


<a id="keyboard_controls"></a>

# Keyboard Controls

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Key</th>
<th scope="col" class="org-left">Function</th>
</tr>
</thead>
<tbody>
<tr>
<td class="org-left">r</td>
<td class="org-left">Increase red component</td>
</tr>

<tr>
<td class="org-left">g</td>
<td class="org-left">Increase green component</td>
</tr>

<tr>
<td class="org-left">b</td>
<td class="org-left">Increase blue component</td>
</tr>

<tr>
<td class="org-left">R</td>
<td class="org-left">Decrease red component</td>
</tr>

<tr>
<td class="org-left">G</td>
<td class="org-left">Decrease green component</td>
</tr>

<tr>
<td class="org-left">B</td>
<td class="org-left">Decrease blue component</td>
</tr>

<tr>
<td class="org-left">t</td>
<td class="org-left">Toggle window border</td>
</tr>

<tr>
<td class="org-left">z</td>
<td class="org-left">Reset window position/size</td>
</tr>

<tr>
<td class="org-left">p</td>
<td class="org-left">Toggle always-on-top</td>
</tr>
</tbody>
</table>


<a id="building"></a>

# Building

Do a [make](https://mirror.team-cymru.com/gnu/make) command from root

The resulting binary will be in a `build` folder


<a id="debug_mode"></a>

# Debug Mode

To enable debug output, compile with `-DDEBUG_MODE` using the Makefile


<a id="system_requirements"></a>

# System Requirements


## Windows

-   Windows 10 or later


## Linux

-   X11 or Wayland


<a id="limitations"></a>

# Limitations

-   Maximum of 90 target file paths
-   Maximum of 10 keywords
-   Maximum of 100 lines in config file
-   Maximum of 150 matching task lines displayed


<a id="license"></a>

# License

The MIT License (MIT)

Copyright (c) 2025 Eyoel Tesfu

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
&ldquo;Software&rdquo;), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED &ldquo;AS IS&rdquo;, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

