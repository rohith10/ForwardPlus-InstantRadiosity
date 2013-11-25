##Instant Radiosity using a Forward+ shader
###CIS 565 Final Project by Rohith Chandran and [Vivek Reddy] (https://github.com/vivreddy)

Achieving real-time global illumination has been the holy grail of rendering in video games. Because 
accurate global illumination as computed by a path tracer or a photon mapper is computationally intense 
and time-consuming, most games eschew such techniques, opting instead for simple Phong shading with tricks 
to cheaply simulate indirect illumination. Such techniques include, but are not limited to, the use of 
lightmaps and local illumination with ambient occlusion.  
  
The Instant Radiosity method is an interesting method to achieve global illumination in real-time. This 
method works by creating a large number of point lights (called virtual point lights in the literature) 
at places in the scene where rays from the light source would intersect scene objects. Shading is then 
performed using all of the lights, including these virtual point lights. Because this method would result 
in the creation of a large number of point lights, it is a prime candidate for being rendered using deferred shading.  
  
We plan to implement the classic Instant Radiosity method with two modifications. The first deals with 
the placement of virtual point lights in the scene. The Instant Radiosity method prescribes the placement 
of virtual point lights at each point in the world corresponding to every pixel location in the light’s 
rendering of the scene. We envision doing this step dynamically, by using the compute shader to trace 
rays from the light, finding the intersection of these rays with scene objects, and placing virtual 
point lights at the intersection points. We understand that tracing rays through the scene at real time 
for every frame might not be a good idea, so we limit ourselves to using this technique for static lights 
only and aim to perform the required computation during loading of the scene.  
  
The second has to deal with the choice of shading technique. Deferred shading would be **much** faster than 
forward shading for this method, but we happened to chance upon a technique called Forward+ shading, 
which was used in the AMD Leo Demo for the Radeon HD 7000 series. This technique is basically an application 
of deferred shading principles to forward rendering so as to make the latter more efficient. It looks 
promising since it claims to provide a speedup over regular deferred shading. We look forward to 
implementing it in our project.


DEPTH MAP

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/depthMap.png)

SHADOW MAP

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/shadowMap.png)


RENDERING WITH SHADOWS

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/withshadows.png)
