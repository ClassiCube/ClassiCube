Compiling with Carbide C++

- File > Import... > Symbian OS > Symbian OS Bld.inf file
- Locate `misc\symbian\bld.inf`, select `SBSv2` builder, Next.
- Under `Nokia_Symbian3_SDK_v1.0`, select targets with `armv5_urel_gcce` and `armv5_urel` (second only if you have RVCT), Next.
- Select All, Next, Finish.
- Go to project properties, Carbide.c++>Build Configurations, select correct configuration
(e.g: `Phone Release (armv5_urel_gcce) [Nokia_Symbian3_SDK_v1.0]`).
- In SIS Builder tab, click Add.
- In PKG File drop down list, select `misc\symbian\ClassiCube.pkg`.
- Check `Don't sign sis file`, click OK.
- Apply and close properties.
- Project > Build

(Based off comments from https://github.com/ClassiCube/ClassiCube/pull/1360)