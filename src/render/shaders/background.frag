#version 450
layout(location=0) in vec2 v_uv;
layout(location=0) out vec4 o_color;
layout(set=0,binding=0) uniform sampler2D u_texture;
vec3 source(vec2 uv) {
#ifdef BLADES_OPENGL
    uv.y=1.0-uv.y;
#endif
    return texture(u_texture,uv).rgb;
}
void main() {
    // Reproduce the original endpoint mapping and 8-bit interpolation weights.
    // Point fetches keep each separable pass's integer rounding on the GPU.
    vec2 p=floor(clamp((v_uv*vec2(1280.0,720.0)-.5)/vec2(1279.0,719.0),0.0,1.0)*vec2(383.0,215.0)*256.0)/256.0;
    vec2 i=floor(p),f=fract(p),size=vec2(384.0,216.0);
    vec3 a=source((i+.5)/size);
    vec3 b=source((i+vec2(1.5,.5))/size);
    vec3 c=source((i+vec2(.5,1.5))/size);
    vec3 d=source((i+1.5)/size);
    vec3 top=floor(mix(a,b,f.x)*255.0+.001),bottom=floor(mix(c,d,f.x)*255.0+.001);
    o_color=vec4(floor(mix(top,bottom,f.y)+.001)/255.0,1.0);
}
