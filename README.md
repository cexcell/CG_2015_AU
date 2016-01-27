# CG_2015_AU
Computer Graphics 2015 SpbAU ( OpenGL )

To build it via 2013 or 2015 visual studio you should declare platform toolset for each project in Properties->General and then open commons.props and change additional library dir to vc120 or vc140 for vs2013 and vs2015 respectively.

## Task 1 - Fractal
Visualize Mandelbrot set and map 1D texture on it.
Iterations number and r^2 can be controlled from two sliders

## Task 2 - Cubemap
Draws box with cubemap and bunny with enviroment map texture.

* Camera controls - WASD + mouse.
* Draw wireframes - V.

## Task 3 - Projection
Draw a simple textured scene with nanosuit soldier from crysis and projets a smiling texture on it

* Camera controls - WASD + mouse. 
* Switch texture filtering:
  * 1 - Nearest
  * 2 - Linear
  * 3 - Linear mipmap
* Switch texture from smiling to dynamic texture from virtual camera - G.
* Hold space + camera controls to move projector

## Task 4 - Deferred shading
Draw nanosuit soldier from crysis using deferred shading. Supports directional and point lights

* Camera controls - WASD + mouse
* Draw wireframes - V
* To switch between attachments(position, normal, diffuse, specular, final render) use keys 1, 2, 3, 4, 5 respectively.
