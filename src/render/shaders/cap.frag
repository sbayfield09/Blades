#version 450
// Alpha-aware vertical box filtering for the caps' small overshoot shrink.
// The original CPU scaler includes every source row intersecting the pixel.
layout(location=0) in vec2 v_uv;
layout(location=0) out vec4 o_color;
layout(set=0,binding=0) uniform sampler2D u_texture;
layout(push_constant) uniform Push {vec4 size;} pc;
void main() {
    float ratio=pc.size.y/pc.size.z;
    float position=v_uv.y*pc.size.y-.5;
    float y0=floor(position+.0001),y1=ceil(position+ratio-.0001);
    float count=y1-y0;
    vec4 a=texture(u_texture,vec2(v_uv.x,(y0+.5)/pc.size.y));
    vec4 b=texture(u_texture,vec2(v_uv.x,(y0+1.5)/pc.size.y));
    vec4 c=texture(u_texture,vec2(v_uv.x,(y0+2.5)/pc.size.y));
    float wb=count>1.0?1.0:0.0,wc=count>2.0?1.0:0.0;
    float alpha=a.a+b.a*wb+c.a*wc;
    vec3 rgb=(a.rgb*a.a+b.rgb*b.a*wb+c.rgb*c.a*wc)/max(alpha,.000001);
    o_color=vec4(floor(rgb*255.0+.001)/255.0,floor(alpha*255.0/count+.001)/255.0);
}
