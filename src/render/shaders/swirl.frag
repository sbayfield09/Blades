#version 450
// Motion and lighting from zivvoz / DashX360 BladeAmbientTexture.cs, pinned
// 0c8adb2cef4ad7dff2bf889351efa1f1c244aef2. See DASHX360-NOTICE.txt.
layout(location=0) in vec2 v_uv;
layout(location=0) out vec4 o_color;
layout(push_constant) uniform Push {vec4 tint;vec4 motion;} pc;
void main() {
    // This pass is 384x216. UV locates the original source grid exactly.
    vec2 grid=(v_uv*vec2(384.0,216.0)-.5)/vec2(383.0,215.0);
    float u=(grid.x-.51)*2.0,v=(grid.y-.49)*1.7,phase=pc.motion.x;
    float a=u+.14*sin(v*3.0+phase),b=v+.11*cos(phase*.73);
    float radius=sqrt(a*a*.83+b*b);
    float band=(radius-.65-.12*sin((grid.x-.51)*6.2+phase*.6))*4.2;
    float ribbon=exp(-band*band);
    float wave=sin(u*3.4)*cos(v*2.8+phase*.8)+cos(u*3.4)*sin(v*2.8+phase*.8);
    float light=.69+ribbon*.57+wave*.08+sin(radius*9.0-phase)*.045;
    vec3 color=floor(clamp(pc.tint.rgb*light+ribbon*vec3(18.0,25.0,5.0),0.0,255.0));
    o_color=vec4(color/255.0,1.0);
}
