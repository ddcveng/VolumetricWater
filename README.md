# Volumetric Water

This repository contains a little demo scene showcasing a water shader I made as a school project. The water surface reflects as well as refracts light according to the Fresnel equations (using Schlick's approximation).

## Building and running

You need to have [glfw](https://www.glfw.org/) (v3) installed, point the linker to the `.lib` files and make the `.dll` available to the built binary. The easiest way to do this is to copy the glfw lib files into the `lib` directory and place the dll into the `bin` directory. Or you can point the project to wherever your glfw is installed.

Then just open the solution in visual studio and as long as you have some c++ build tools installed, everything should work. You can move about the scene using WASD and move the camera with the mouse when holding right click.

## Where's the sauce
The relevant code is in the `02-3dScene` directory. The shaders can be found in `shaders.h` and the code that draws the scene in `main.cpp`.