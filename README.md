##Instant Radiosity for real time global illumination
###CIS 565 Final Project by Rohith Chandran and [Vivek Reddy] (https://github.com/vivreddy)

Achieving real-time global illumination has been the holy grail of rendering in video games. Because 
accurate global illumination as computed by a path tracer or a photon mapper is computationally intense 
and time-consuming, most games eschew such techniques, opting instead for simple Phong shading with tricks 
to cheaply simulate indirect illumination. Such techniques include, but are not limited to, the use of 
lightmaps and local illumination with ambient occlusion.  
  
The Instant Radiosity method is an interesting method to achieve global illumination in real-time. This 
method works by creating a large number of point lights (called virtual point lights in the literature) 
at places in the scene where rays from the light source would intersect scene objects. Shading is then 
performed using all of the lights, including these virtual point lights.
  
We implemented the classic Instant Radiosity method, employing the Compute Shader to perform ray tracing and the creation of virtual point lights in the scene. The Compute Shader became part of the GLSL core specification with version 4.3, and our primary motivation for employing it was to use a solution that would work on graphics cards from all major vendors, as opposed to proprietary solutions like CUDA. OpenCL was not chosen even though it was a viable alternative because the ray tracing that we perform is fairly lightweight, and the Compute Shader was designed for such purposes. Moreover, it is more tightly integrated into the OpenGL workflow than OpenCL (through CL/GL introp) . We based our code on the Deferred Shader we already implemented, and added render paths to it.
  
##Future work

The original scope of the project would have us implement additional rendering techniques, such as Forward+/Tiled forward and Tiled deferred rendering as part of the core project. However, in the course of implementation, we ran into multiple issues (not necessarily bugs) which took quite a lot of time to figure out how to get around. As a result, although we have implemented tiled forward shading, there are still issues that persist that need to be ironed out before we can get an acceptable result from it. This is something that we're continuing to work on.

Shadows in the forward rendering path are messed up. It works on the deferred rendering path, although it's using the same shadow map texture and the same code to generate the said texture. This is something that we'll fix shortly. 

We're also working on making the code less messier. Ah, deadlines.. ^_^

##Screenshots :

###Without Global Illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/1.png)

###With Global Illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/2.png)

###Virtual point lights that contribute for the global illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/uniformVPLsagain.png)

###Color Bleeding 
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/3.png)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/4.png)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/5.png)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/6.png)


##Performance Analysis
We measured performance on a desktop system having an Intel Xeon processor, 8 GB of RAM and a GeForce GTX Titan, comparing the framerates for both the forward and deferred rendering passes. The results we obtained are as given below:

###Comparisons with forward and deferred shading for different number of VPL's  

Performance results on Nvidia GeForce GTX Titan  
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2DellComparision.png)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1DellComparision.png)

Performance results on Nvidia GT 740M  
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2HPComparision.png)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1HPComparision.png)
