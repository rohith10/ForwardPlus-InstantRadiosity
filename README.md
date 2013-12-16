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

The original scope of the project would have us implement additional rendering techniques, such as Forward+/Tiled forward and Tiled deferred rendering as part of the core project. However, in the course of implementation, we ran into multiple issues (not necessarily bugs) which took quite a lot of time to figure out how to get around. As a result, although we have implemented tiled forward shading, there are still issues that persist that need to be ironed out before we can get an acceptable result from it.

We're also working on making the code less messier. Ah, deadlines.. ^_^

##Screenshots :

###Without Global Illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/1.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvMS5wbmciLCJleHBpcmVzIjoxMzg3NzU4Mjk2fQ%3D%3D--2e264a9b09e0290368b87202da6c6a60e71585cb)

###With Global Illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/2.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvMi5wbmciLCJleHBpcmVzIjoxMzg3NzU4MzE3fQ%3D%3D--0a0063348469f2997740546f9f6744a7d56edef3)

###Virtual point lights that contribute for the global illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/uniformVPLsagain.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvdW5pZm9ybVZQTHNhZ2Fpbi5wbmciLCJleHBpcmVzIjoxMzg3NTE0NzUyfQ%3D%3D--ccdd2584eed3ecb37bd03fcac31bd81842d82a7f)

###Color Bleeding 
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/3.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvMy5wbmciLCJleHBpcmVzIjoxMzg3NzU4MzMzfQ%3D%3D--c47b7c35cc29b52e5ee2a7703ff4f5514b04d9d6)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/4.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvNC5wbmciLCJleHBpcmVzIjoxMzg3NzU4MzUwfQ%3D%3D--37b52674f1e1b73a1be76eb023bfbef40665ec16)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/5.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvNS5wbmciLCJleHBpcmVzIjoxMzg3NzU4NDYxfQ%3D%3D--d168b48016a79ce14ca770db0d7f1a031534cca1)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/6.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvNi5wbmciLCJleHBpcmVzIjoxMzg3NzU4NDE4fQ%3D%3D--bd92621b0d759030304a037bdbce5468d9c7caef)

###Shadow mapping (on Deferred rendering path)
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/shadowMap.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvc2hhZG93TWFwLnBuZyIsImV4cGlyZXMiOjEzODc1MTAzOTh9--ab178c15ef68255b247c3a0a6143ee768a183043)


##Performance Analysis
We measured performance on a desktop system having an Intel Xeon processor, 8 GB of RAM and a GeForce GTX Titan, comparing the framerates for both the forward and deferred rendering passes. The results we obtained are as given below:

###Comparisons with forward and deferred shading for different number of VPL's  

Performance results on Nvidia GeForce GTX Titan  
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2DellComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUyRGVsbENvbXBhcmlzaW9uLnBuZyIsImV4cGlyZXMiOjEzODc1MTAyOTF9--e02411174e3a04a0e3043c535d8d35205fbddd28)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1DellComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUxRGVsbENvbXBhcmlzaW9uLnBuZyIsImV4cGlyZXMiOjEzODc1MTAyMzJ9--669deeeaa1abcb557af89c97d52df5045578de2a)

Performance results on Nvidia GT 740M  
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2HPComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUySFBDb21wYXJpc2lvbi5wbmciLCJleHBpcmVzIjoxMzg3NTEwMzUwfQ%3D%3D--902af622e9453664f965cafcb725c106e13e0656)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1HPComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUxSFBDb21wYXJpc2lvbi5wbmciLCJleHBpcmVzIjoxMzg3NTEwMzI1fQ%3D%3D--b4bb15be395ebd2e1890e34d86fd1e08e285bb5f)
