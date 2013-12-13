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



SHADOW MAP

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/shadowMap.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvc2hhZG93TWFwLnBuZyIsImV4cGlyZXMiOjEzODc1MTAzOTh9--ab178c15ef68255b247c3a0a6143ee768a183043)


RENDERING WITH SHADOWS

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/withshadows.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvd2l0aHNoYWRvd3MucG5nIiwiZXhwaXJlcyI6MTM4NzUxMDQzMn0%3D--c86d31c98cab8e140be0d6c6c43d10ad23540b78)


Comparisons with forward and deferred shading for different number of VPL's

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1DellComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUxRGVsbENvbXBhcmlzaW9uLnBuZyIsImV4cGlyZXMiOjEzODc1MTAyMzJ9--669deeeaa1abcb557af89c97d52df5045578de2a)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2DellComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUyRGVsbENvbXBhcmlzaW9uLnBuZyIsImV4cGlyZXMiOjEzODc1MTAyOTF9--e02411174e3a04a0e3043c535d8d35205fbddd28)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1HPComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUxSFBDb21wYXJpc2lvbi5wbmciLCJleHBpcmVzIjoxMzg3NTEwMzI1fQ%3D%3D--b4bb15be395ebd2e1890e34d86fd1e08e285bb5f)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2HPComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUySFBDb21wYXJpc2lvbi5wbmciLCJleHBpcmVzIjoxMzg3NTEwMzUwfQ%3D%3D--902af622e9453664f965cafcb725c106e13e0656)
