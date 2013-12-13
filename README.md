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
  

ScreenShots :

Without Global Illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/forward-and-fplus/base/res/withoutGI.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9mb3J3YXJkLWFuZC1mcGx1cy9iYXNlL3Jlcy93aXRob3V0R0kucG5nIiwiZXhwaXJlcyI6MTM4NzUxMjczN30%3D--fb97916302564e8ba4c1b9ed39a084d2c6f7d84b)

With Global Illumination (Note that the image is not tone mapped and hence appears blown out)
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/forward-and-fplus/base/res/withGI.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9mb3J3YXJkLWFuZC1mcGx1cy9iYXNlL3Jlcy93aXRoR0kucG5nIiwiZXhwaXJlcyI6MTM4NzUxMjgwM30%3D--a7aacaf087ed6412c4fc93c017c1d6d83c12c242)

Virtual point lights that contribute for the global illumination
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/forward-and-fplus/base/res/uniformVPLs.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9mb3J3YXJkLWFuZC1mcGx1cy9iYXNlL3Jlcy91bmlmb3JtVlBMcy5wbmciLCJleHBpcmVzIjoxMzg3NTEyODcyfQ%3D%3D--ed55376f6bb32573386eecf179d97edc1eeea4f2)

Color Bleeding 
![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/forward-and-fplus/base/res/ColorBleeding.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9mb3J3YXJkLWFuZC1mcGx1cy9iYXNlL3Jlcy9Db2xvckJsZWVkaW5nLnBuZyIsImV4cGlyZXMiOjEzODc1MTI5Mzd9--f64d567512d325fbcdffc536c82590024724761d)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/forward-and-fplus/base/res/ColorBleeding2.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9mb3J3YXJkLWFuZC1mcGx1cy9iYXNlL3Jlcy9Db2xvckJsZWVkaW5nMi5wbmciLCJleHBpcmVzIjoxMzg3NTEyOTY4fQ%3D%3D--83b71176fd47015dbd1f84721993ef1766d99846)




Comparisons with forward and deferred shading for different number of VPL's

Performance testing done in GForce GTX-Titan 

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1DellComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUxRGVsbENvbXBhcmlzaW9uLnBuZyIsImV4cGlyZXMiOjEzODc1MTAyMzJ9--669deeeaa1abcb557af89c97d52df5045578de2a)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2DellComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUyRGVsbENvbXBhcmlzaW9uLnBuZyIsImV4cGlyZXMiOjEzODc1MTAyOTF9--e02411174e3a04a0e3043c535d8d35205fbddd28)

Performance testing done in GT-740M

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table1HPComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUxSFBDb21wYXJpc2lvbi5wbmciLCJleHBpcmVzIjoxMzg3NTEwMzI1fQ%3D%3D--b4bb15be395ebd2e1890e34d86fd1e08e285bb5f)

![alt tag](https://raw.github.com/rohith10/ForwardPlus-InstantRadiosity/master/base/res/Table2HPComparision.png?token=5392763__eyJzY29wZSI6IlJhd0Jsb2I6cm9oaXRoMTAvRm9yd2FyZFBsdXMtSW5zdGFudFJhZGlvc2l0eS9tYXN0ZXIvYmFzZS9yZXMvVGFibGUySFBDb21wYXJpc2lvbi5wbmciLCJleHBpcmVzIjoxMzg3NTEwMzUwfQ%3D%3D--902af622e9453664f965cafcb725c106e13e0656)
